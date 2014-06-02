#include "omxcam.h"

typedef struct {
  void (*buffer_callback)(uint8_t* buffer, uint32_t length);
  omxcam_component_t* fill_component;
} omxcam_thread_arg_t;

static int use_encoder;
static int running_safe;
static int running = 0;
static int sleeping = 0;
static int locked = 0;
static int bg_error = 0;
static pthread_t bg_thread;
static pthread_mutex_t mutex;
static pthread_mutex_t mutex_cond;
static pthread_cond_t cond;
static omxcam_thread_arg_t thread_arg;

static int omxcam_video_change_state (omxcam_state state, int use_encoder){
  if (omxcam_component_change_state (&omxcam_ctx.camera, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (omxcam_component_change_state (&omxcam_ctx.null_sink, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (!use_encoder) return 0;
  
  if (omxcam_component_change_state (&omxcam_ctx.video_encode, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.video_encode, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  if (state == OMXCAM_STATE_EXECUTING &&
      omxcam_event_wait (&omxcam_ctx.video_encode,
          OMXCAM_EVENT_PORT_SETTINGS_CHANGED, 0)){
    return -1;
  }
  
  return 0;
}

static int omxcam_init_omx (omxcam_video_settings_t* settings){
  omxcam_trace ("initializing video");
  
  OMX_COLOR_FORMATTYPE color_format;
  OMX_U32 stride;
  omxcam_component_t* fill_component;
  OMX_ERRORTYPE error;
  
  OMX_U32 width = omxcam_round (settings->camera.width, 32);
  OMX_U32 height = omxcam_round (settings->camera.height, 16);
  OMX_U32 slice_height = omxcam_round (settings->slice_height, 16);
  
  //Stride is byte-per-pixel*width
  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
  
  switch (settings->format){
    case OMXCAM_FORMAT_RGB888:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format24bitRGB888;
      stride = width*3;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_RGBA8888:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format32bitABGR8888;
      stride = width*4;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_YUV420:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = width;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_H264:
      omxcam_ctx.video_encode.buffer_callback = settings->buffer_callback;
      use_encoder = 1;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = width;
      fill_component = &omxcam_ctx.video_encode;
      break;
    default:
      omxcam_set_last_error (OMXCAM_ERROR_FORMAT);
      return -1;
  }
  
  omxcam_trace ("%dx%d @%dfps", width, height,
      settings->camera.framerate);
  
  thread_arg.buffer_callback = settings->buffer_callback;
  thread_arg.fill_component = fill_component;
  
  if (omxcam_component_init (&omxcam_ctx.camera)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_CAMERA);
    return -1;
  }
  if (omxcam_component_init (&omxcam_ctx.null_sink)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam_component_init (&omxcam_ctx.video_encode)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_VIDEO_ENCODER);
    return -1;
  }
  
  if (omxcam_camera_load_drivers ()){
    omxcam_set_last_error (OMXCAM_ERROR_DRIVERS);
    return -1;
  }
  
  //Configure camera port definition
  omxcam_trace ("configuring '%s' port definition", omxcam_ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE port_st;
  OMXCAM_INIT_STRUCTURE (port_st);
  port_st.nPortIndex = 71;
  if ((error = OMX_GetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  port_st.format.video.nFrameWidth = width;
  port_st.format.video.nFrameHeight = height;
  port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  port_st.format.video.eColorFormat = color_format;
  port_st.format.video.xFramerate = settings->camera.framerate << 16;
  port_st.format.video.nStride = stride;
  //It should be read-only parameter as stated in the OpenMAX IL specification,
  //but it can be configured. With this parameter the size of the buffer payload
  //can be controlled. It must be multiple of 16. Default is 16
  port_st.format.video.nSliceHeight = slice_height;
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (error ==  OMX_ErrorBadParameter
        ? OMXCAM_ERROR_BAD_PARAMETER
        : OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  OMX_PARAM_PORTDEFINITIONTYPE asd;
  OMXCAM_INIT_STRUCTURE (asd);
  asd.nPortIndex = 71;
  if ((error = OMX_GetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &asd))){
    omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //The preview port must be configured with the same settings as the video port
  port_st.nPortIndex = 70;
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //The framerate must be configured (again) with its own structure in the video
  //and preview ports
  omxcam_trace ("Configuring %s framerate", omxcam_ctx.camera.name);
  
  OMX_CONFIG_FRAMERATETYPE framerate_st;
  OMXCAM_INIT_STRUCTURE (framerate_st);
  framerate_st.nPortIndex = 71;
  framerate_st.xEncodeFramerate = port_st.format.video.xFramerate;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Preview port
  framerate_st.nPortIndex = 70;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Configure camera settings
  if (omxcam_camera_configure_omx (&settings->camera, 1)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    omxcam_trace ("configuring '%s' port definition",
        omxcam_ctx.video_encode.name);
    
    OMXCAM_INIT_STRUCTURE (port_st);
    port_st.nPortIndex = 201;
    if ((error = OMX_GetParameter (omxcam_ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    port_st.format.video.nFrameWidth = width;
    port_st.format.video.nFrameHeight = height;
    port_st.format.video.xFramerate = settings->camera.framerate << 16;
    port_st.format.video.nStride = stride;
    port_st.format.video.nSliceHeight = height;
    
    //Despite being configured later, these two fields need to be set now
    port_st.format.video.nBitrate = settings->h264.bitrate;
    port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    
    if ((error = OMX_SetParameter (omxcam_ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    //Configure H264 settings
    if (omxcam_h264_configure_omx (&settings->h264)){
      omxcam_set_last_error (OMXCAM_ERROR_H264);
      return -1;
    }
    
    //Setup tunnel: camera (video) -> video_encode
    omxcam_trace ("Configuring tunnel '%s' -> '%s'", omxcam_ctx.camera.name,
        omxcam_ctx.video_encode.name);
    
    if ((error = OMX_SetupTunnel (omxcam_ctx.camera.handle, 71,
        omxcam_ctx.video_encode.handle, 200))){
      omxcam_error ("OMX_SetupTunnel: %s", omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Setup tunnel: camera (preview) -> null_sink
  omxcam_trace ("configuring tunnel '%s' -> '%s'", omxcam_ctx.camera.name,
      omxcam_ctx.null_sink.name);
  
  if ((error = OMX_SetupTunnel (omxcam_ctx.camera.handle, 70,
      omxcam_ctx.null_sink.handle, 240))){
    omxcam_error ("OMX_SetupTunnel: %s", omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Change to Idle
  if (omxcam_video_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Enable the ports
  if (omxcam_component_port_enable (&omxcam_ctx.camera, 71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (!use_encoder && omxcam_buffer_alloc (&omxcam_ctx.camera, 71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_component_port_enable (&omxcam_ctx.camera, 70)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_component_port_enable (&omxcam_ctx.null_sink, 240)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam_component_port_enable (&omxcam_ctx.video_encode, 200)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.video_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_component_port_enable (&omxcam_ctx.video_encode, 201)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_buffer_alloc (&omxcam_ctx.video_encode, 201)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.video_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Change to Executing
  if (omxcam_video_change_state (OMXCAM_STATE_EXECUTING, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_EXECUTING);
    return -1;
  }
  
  //Set camera capture port
  if (omxcam_camera_capture_port_set (71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  return 0;
}

static int omxcam_deinit_omx (){
  omxcam_trace ("deinitializing video");
  
  running = 0;
  
  if (pthread_mutex_destroy (&mutex)){
    omxcam_error ("pthread_mutex_destroy");
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }

  //Reset camera capture port
  if (omxcam_camera_capture_port_reset (71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Change to Idle
  if (omxcam_video_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Disable the ports
  if (omxcam_component_port_disable (&omxcam_ctx.camera, 71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (!use_encoder && omxcam_buffer_free (&omxcam_ctx.camera, 71)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_component_port_disable (&omxcam_ctx.camera, 70)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_component_port_disable (&omxcam_ctx.null_sink, 240)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam_component_port_disable (&omxcam_ctx.video_encode, 200)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.video_encode, OMXCAM_EVENT_PORT_DISABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_component_port_disable (&omxcam_ctx.video_encode, 201)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_buffer_free (&omxcam_ctx.video_encode, 201)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.video_encode, OMXCAM_EVENT_PORT_DISABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Change to Loaded
  if (omxcam_video_change_state (OMXCAM_STATE_LOADED, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_LOADED);
    return -1;
  }
  
  if (omxcam_component_deinit (&omxcam_ctx.camera)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_CAMERA);
    return -1;
  }
  if (omxcam_component_deinit (&omxcam_ctx.null_sink)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam_component_deinit (&omxcam_ctx.video_encode)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_VIDEO_ENCODER);
    return -1;
  }
  
  return 0;
}

static int omxcam_thread_sleep (uint32_t ms){
  omxcam_trace ("sleeping for %d ms", ms);
  
  if (pthread_mutex_init (&mutex_cond, 0)){
    omxcam_error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    omxcam_error ("pthread_cond_init");
    return -1;
  }
  
  //Lock the main thread for a given time using a timed cond variable
  
  struct timespec time;
  struct timeval now;

  gettimeofday (&now, 0);
  uint64_t end = now.tv_sec*1e6 + now.tv_usec + ms*1000;
  time.tv_sec = (time_t)(end/1e6);
  time.tv_nsec = (end%(uint64_t)1e6)*1000;
  
  int error;
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam_error ("pthread_mutex_lock");
    return -1;
  }
  
  sleeping = 1;
  
  if ((error = pthread_cond_timedwait (&cond, &mutex_cond, &time)) &&
      error != ETIMEDOUT){
    omxcam_error ("pthread_cond_timedwait");
    sleeping = 0;
    error = 1;
  }else{
    error = 0;
  }
  
  sleeping = 0;
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam_error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (error) return -1;
  
  if (pthread_mutex_destroy (&mutex_cond)){
    omxcam_error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    omxcam_error ("pthread_cond_destroy");
    return -1;
  }
  
  omxcam_trace ("main thread woken up");
  
  return 0;
}

static int omxcam_thread_wake (){
  omxcam_trace ("waking up from sleep");
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam_error ("pthread_mutex_lock");
    return -1;
  }
  
  if (pthread_cond_signal (&cond)){
    omxcam_error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam_error ("pthread_mutex_unlock");
    return -1;
  }

  return 0;
}

static int omxcam_thread_lock (){
  omxcam_trace ("locking main thread");
  
  if (pthread_mutex_init (&mutex_cond, 0)){
    omxcam_error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    omxcam_error ("pthread_cond_init");
    return -1;
  }
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam_error ("pthread_mutex_lock");
    return -1;
  }
  
  locked = 1;
  
  if (pthread_cond_wait (&cond, &mutex_cond)){
    omxcam_error ("pthread_cond_wait");
    locked = 0;
    return -1;
  }
  
  locked = 0;
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam_error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (pthread_mutex_destroy (&mutex_cond)){
    omxcam_error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    omxcam_error ("pthread_cond_destroy");
    return -1;
  }
  
  omxcam_trace ("main thread unlocked");
  
  return 0;
}

static int omxcam_thread_unlock (){
  omxcam_trace ("unlocking main thread");
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam_error ("pthread_mutex_lock");
    return -1;
  }
  
  if (pthread_cond_signal (&cond)){
    omxcam_error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam_error ("pthread_mutex_unlock");
    return -1;
  }
  
  return 0;
}

static void omxcam_thread_handle_error (){
  omxcam_trace ("error while capturing");
  bg_error = -1;
  //Ignore the error
  omxcam_video_stop ();
  
  //Save the error after the video deinitialization in order to overwrite
  //a possible error during this task
  omxcam_set_last_error (OMXCAM_ERROR_CAPTURE);
}

static void* omxcam_video_capture (void* thread_arg){
  //The return value is not needed

  omxcam_thread_arg_t* arg = (omxcam_thread_arg_t*)thread_arg;
  int stop = 0;
  OMX_ERRORTYPE error;
  
  running_safe = 1;
  
  while (running_safe){
    //Critical section, this loop needs to be as fast as possible
  
    if (pthread_mutex_lock (&mutex)){
      omxcam_error ("pthread_mutex_lock");
      omxcam_thread_handle_error ();
      return (void*)0;
    }
    
    stop = !running;
    
    if (pthread_mutex_unlock (&mutex)){
      omxcam_error ("pthread_mutex_unlock");
      omxcam_thread_handle_error ();
      return (void*)0;
    }
    
    if (stop) break;
    
    if ((error = OMX_FillThisBuffer (arg->fill_component->handle,
        omxcam_ctx.output_buffer))){
      omxcam_error ("OMX_FillThisBuffer: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_thread_handle_error ();
      return (void*)0;
    }
    
    //Wait until it's filled
    if (omxcam_event_wait (arg->fill_component, OMXCAM_EVENT_FILL_BUFFER_DONE,
        0)){
      omxcam_thread_handle_error ();
      return (void*)0;
    }
    
    //Emit the buffer
    arg->buffer_callback (omxcam_ctx.output_buffer->pBuffer,
        omxcam_ctx.output_buffer->nFilledLen);
  }
  
  omxcam_trace ("exit thread");
  
  return (void*)0;
}

void omxcam_video_init (omxcam_video_settings_t* settings){
  omxcam_camera_init (&settings->camera, 1920, 1080);
  settings->format = OMXCAM_FORMAT_H264;
  omxcam_h264_init (&settings->h264);
  settings->buffer_callback = 0;
  settings->slice_height = 16;
}

int omxcam_video_start (
    omxcam_video_settings_t* settings,
    uint32_t ms){
  omxcam_trace ("starting video capture");
  
  omxcam_set_last_error (OMXCAM_ERROR_NONE);
  
  if (!settings->buffer_callback){
    omxcam_error ("the 'buffer_callback' field must be defined");
    omxcam_set_last_error (OMXCAM_ERROR_BAD_PARAMETER);
    return -1;
  }
  if (running){
    omxcam_error ("video capture is already running");
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  bg_error = 0;
  
  if (omxcam_init ()){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (omxcam_init_omx (settings)) return -1;
  
  //Start the background thread
  omxcam_trace ("creating background thread");
  
  if (pthread_mutex_init (&mutex, 0)){
    omxcam_error ("pthread_mutex_init");
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  running = 1;
  
  if (pthread_create (&bg_thread, 0, omxcam_video_capture, &thread_arg)){
    omxcam_error ("pthread_create");
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Block the main thread
  if (ms == OMXCAM_CAPTURE_FOREVER){
    if (omxcam_thread_lock ()){
      omxcam_set_last_error (OMXCAM_ERROR_LOCK);
      return -1;
    }
  }else{
    if (omxcam_thread_sleep (ms)){
      omxcam_set_last_error (OMXCAM_ERROR_SLEEP);
      return -1;
    }
  }
  
  if (bg_error){
    //The video was already stopped due to an error
    omxcam_trace ("video stopped due to an error");
    
    int error = bg_error;
    bg_error = 0;
    
    //At this point the background thread could be still alive executing
    //deinitialization tasks, so wait until it finishes
    
    if (pthread_join (bg_thread, 0)){
      omxcam_error ("pthread_join");
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    return error;
  }
  
  if (!running){
    //The video was already stopped by the user
    omxcam_trace ("video already stopped by the user");
    
    if (pthread_join (bg_thread, 0)){
      omxcam_error ("pthread_join");
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    return 0;
  }
  
  return omxcam_video_stop ();
}

int omxcam_video_stop (){
  omxcam_trace ("stopping video capture");
  
  omxcam_set_last_error (OMXCAM_ERROR_NONE);
  
  if (!running){
    omxcam_error ("video capture is not running");
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (pthread_equal (pthread_self (), bg_thread)){
    //Background thread
    omxcam_trace ("stopping from background thread");
    
    //If stop() is called from inside the background thread (from the
    //buffer_callback or due to an error), there's no need to use mutexes and
    //join(), just set running to false and the thread will die naturally
    running = 0;
    
    //This var is used to prevent calling mutex_lock() after mutex_destroy()
    running_safe = 0;
  }else{
    //Main thread
    //This case also applies when the video is stopped from another random
    //thread different than the main thread
    omxcam_trace ("stopping from main thread");
    
    if (pthread_mutex_lock (&mutex)){
      omxcam_error ("pthread_mutex_lock");
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    running = 0;
    
    if (pthread_mutex_unlock (&mutex)){
      omxcam_error ("pthread_mutex_unlock");
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    if (pthread_join (bg_thread, 0)){
      omxcam_error ("pthread_join");
      omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  int error = omxcam_deinit_omx ();
  
  if (sleeping){
    if (omxcam_thread_wake ()){
      if (error){
        omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
        return error;
      }
      omxcam_set_last_error (OMXCAM_ERROR_WAKE);
      return -1;
    }
  }else if (locked){
    if (omxcam_thread_unlock ()){
      if (error){
        omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
        return error;
      }
      omxcam_set_last_error (OMXCAM_ERROR_UNLOCK);
      return -1;
    }
  }
  
  if (omxcam_deinit ()){
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  return 0;
}