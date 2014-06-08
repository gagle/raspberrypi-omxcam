#include "omxcam.h"
#include "internal.h"

typedef struct {
  void (*buffer_callback)(uint8_t* buffer, uint32_t length);
  omxcam__component_t* fill_component;
} omxcam__thread_arg_t;

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
static omxcam__thread_arg_t thread_arg;

static int omxcam__video_change_state (omxcam__state state, int use_encoder){
  if (omxcam__component_change_state (&omxcam__ctx.camera, state)){
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (omxcam__component_change_state (&omxcam__ctx.null_sink, state)){
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.null_sink, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (!use_encoder) return 0;
  
  if (omxcam__component_change_state (&omxcam__ctx.video_encode, state)){
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.video_encode, OMXCAM_EVENT_STATE_SET,
      0)){
    return -1;
  }
  if (state == OMXCAM_STATE_EXECUTING &&
      omxcam__event_wait (&omxcam__ctx.video_encode,
          OMXCAM_EVENT_PORT_SETTINGS_CHANGED, 0)){
    return -1;
  }
  
  return 0;
}

static int omxcam__init_omx (omxcam_video_settings_t* settings){
  omxcam__trace ("initializing video");
  
  OMX_COLOR_FORMATTYPE color_format;
  omxcam__component_t* fill_component;
  OMX_ERRORTYPE error;
  
  OMX_U32 width_rounded = omxcam_round (settings->camera.width, 32);
  OMX_U32 height_rounded = omxcam_round (settings->camera.height, 16);
  OMX_U32 width = width_rounded;
  OMX_U32 height = height_rounded;
  OMX_U32 stride = width_rounded;
  
  //Stride is byte-per-pixel*width
  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
  
  switch (settings->format){
    case OMXCAM_FORMAT_RGB888:
      omxcam__ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format24bitRGB888;
      stride = stride*3;
      fill_component = &omxcam__ctx.camera;
      break;
    case OMXCAM_FORMAT_RGBA8888:
      omxcam__ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format32bitABGR8888;
      stride = stride*4;
      fill_component = &omxcam__ctx.camera;
      break;
    case OMXCAM_FORMAT_YUV420:
      omxcam__ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      fill_component = &omxcam__ctx.camera;
      break;
    case OMXCAM_FORMAT_H264:
      omxcam__ctx.video_encode.buffer_callback = settings->buffer_callback;
      use_encoder = 1;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      width = settings->camera.width;
      height = settings->camera.height;
      fill_component = &omxcam__ctx.video_encode;
      break;
    default:
      omxcam__set_last_error (OMXCAM_ERROR_FORMAT);
      return -1;
  }
  
  omxcam__trace ("%dx%d @%dfps", settings->camera.width,
      settings->camera.height, settings->camera.framerate);
  
  thread_arg.buffer_callback = settings->buffer_callback;
  thread_arg.fill_component = fill_component;
  
  if (omxcam__component_init (&omxcam__ctx.camera)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_CAMERA);
    return -1;
  }
  if (omxcam__component_init (&omxcam__ctx.null_sink)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam__component_init (&omxcam__ctx.video_encode)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_VIDEO_ENCODER);
    return -1;
  }
  
  if (omxcam__camera_load_drivers ()){
    omxcam__set_last_error (OMXCAM_ERROR_DRIVERS);
    return -1;
  }
  
  //Configure camera port definition
  omxcam__trace ("configuring '%s' port definition", omxcam__ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE port_st;
  omxcam__omx_struct_init (port_st);
  port_st.nPortIndex = 71;
  if ((error = OMX_GetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  port_st.format.video.nFrameWidth = width;
  port_st.format.video.nFrameHeight = height;
  port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  port_st.format.video.eColorFormat = color_format;
  port_st.format.video.xFramerate = settings->camera.framerate << 16;
  port_st.format.video.nStride = stride;
  if ((error = OMX_SetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (error ==  OMX_ErrorBadParameter
        ? OMXCAM_ERROR_BAD_PARAMETER
        : OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //The preview port must be configured with the same settings as the video port
  port_st.nPortIndex = 70;
  if ((error = OMX_SetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //The framerate must be configured (again) with its own structure in the video
  //and preview ports
  omxcam__trace ("Configuring %s framerate", omxcam__ctx.camera.name);
  
  OMX_CONFIG_FRAMERATETYPE framerate_st;
  omxcam__omx_struct_init (framerate_st);
  framerate_st.nPortIndex = 71;
  framerate_st.xEncodeFramerate = port_st.format.video.xFramerate;
  if ((error = OMX_SetConfig (omxcam__ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate_st))){
    omxcam__error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Preview port
  framerate_st.nPortIndex = 70;
  if ((error = OMX_SetConfig (omxcam__ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate_st))){
    omxcam__error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Configure camera settings
  if (omxcam__camera_configure_omx (&settings->camera, 1)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    omxcam__trace ("configuring '%s' port definition",
        omxcam__ctx.video_encode.name);
    
    omxcam__omx_struct_init (port_st);
    port_st.nPortIndex = 201;
    if ((error = OMX_GetParameter (omxcam__ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam__error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    port_st.format.video.nFrameWidth = settings->camera.width;
    port_st.format.video.nFrameHeight = settings->camera.height;
    port_st.format.video.xFramerate = settings->camera.framerate << 16;
    port_st.format.video.nStride = stride;
    //Despite being configured later, these two fields need to be set now
    port_st.format.video.nBitrate = settings->h264.bitrate;
    port_st.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    if ((error = OMX_SetParameter (omxcam__ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    //Configure H264 settings
    if (omxcam__h264_configure_omx (&settings->h264)){
      omxcam__set_last_error (OMXCAM_ERROR_H264);
      return -1;
    }
    
    //Setup tunnel: camera (video) -> video_encode
    omxcam__trace ("Configuring tunnel '%s' -> '%s'", omxcam__ctx.camera.name,
        omxcam__ctx.video_encode.name);
    
    if ((error = OMX_SetupTunnel (omxcam__ctx.camera.handle, 71,
        omxcam__ctx.video_encode.handle, 200))){
      omxcam__error ("OMX_SetupTunnel: %s", omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Setup tunnel: camera (preview) -> null_sink
  omxcam__trace ("configuring tunnel '%s' -> '%s'", omxcam__ctx.camera.name,
      omxcam__ctx.null_sink.name);
  
  if ((error = OMX_SetupTunnel (omxcam__ctx.camera.handle, 70,
      omxcam__ctx.null_sink.handle, 240))){
    omxcam__error ("OMX_SetupTunnel: %s", omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Change to Idle
  if (omxcam__video_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Enable the ports
  if (omxcam__component_port_enable (&omxcam__ctx.camera, 71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (!use_encoder && omxcam__buffer_alloc (&omxcam__ctx.camera, 71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__component_port_enable (&omxcam__ctx.camera, 70)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__component_port_enable (&omxcam__ctx.null_sink, 240)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.null_sink, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam__component_port_enable (&omxcam__ctx.video_encode, 200)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.video_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__component_port_enable (&omxcam__ctx.video_encode, 201)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__buffer_alloc (&omxcam__ctx.video_encode, 201)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.video_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Change to Executing
  if (omxcam__video_change_state (OMXCAM_STATE_EXECUTING, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_EXECUTING);
    return -1;
  }
  
  //Set camera capture port
  if (omxcam__camera_capture_port_set (71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  return 0;
}

static int omxcam__deinit_omx (){
  omxcam__trace ("deinitializing video");
  
  running = 0;
  
  if (pthread_mutex_destroy (&mutex)){
    omxcam__error ("pthread_mutex_destroy");
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }

  //Reset camera capture port
  if (omxcam__camera_capture_port_reset (71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Change to Idle
  if (omxcam__video_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Disable the ports
  if (omxcam__component_port_disable (&omxcam__ctx.camera, 71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (!use_encoder && omxcam__buffer_free (&omxcam__ctx.camera, 71)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__component_port_disable (&omxcam__ctx.camera, 70)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__component_port_disable (&omxcam__ctx.null_sink, 240)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.null_sink, OMXCAM_EVENT_PORT_DISABLE,
      0)){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam__component_port_disable (&omxcam__ctx.video_encode, 200)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.video_encode,
        OMXCAM_EVENT_PORT_DISABLE, 0)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__component_port_disable (&omxcam__ctx.video_encode, 201)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__buffer_free (&omxcam__ctx.video_encode, 201)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.video_encode,
        OMXCAM_EVENT_PORT_DISABLE, 0)){
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  //Change to Loaded
  if (omxcam__video_change_state (OMXCAM_STATE_LOADED, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_LOADED);
    return -1;
  }
  
  if (omxcam__component_deinit (&omxcam__ctx.camera)){
    omxcam__set_last_error (OMXCAM_ERROR_DEINIT_CAMERA);
    return -1;
  }
  if (omxcam__component_deinit (&omxcam__ctx.null_sink)){
    omxcam__set_last_error (OMXCAM_ERROR_DEINIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam__component_deinit (&omxcam__ctx.video_encode)){
    omxcam__set_last_error (OMXCAM_ERROR_DEINIT_VIDEO_ENCODER);
    return -1;
  }
  
  return 0;
}

static int omxcam__thread_sleep (uint32_t ms){
  omxcam__trace ("sleeping for %d ms", ms);
  
  if (pthread_mutex_init (&mutex_cond, 0)){
    omxcam__error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    omxcam__error ("pthread_cond_init");
    return -1;
  }
  
  //Lock the main thread for a given time using a timed cond variable
  
  struct timespec time;
  struct timeval now;

  gettimeofday (&now, 0);
  uint64_t end = now.tv_sec*1e6 + now.tv_usec + ms*1000;
  time.tv_sec = (time_t)(end/1e6);
  time.tv_nsec = (end%(uint64_t)1e6)*1000;
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  sleeping = 1;
  int error = 0;
  
  //Spurious wakeup guard
  while (sleeping){
    if ((error = pthread_cond_timedwait (&cond, &mutex_cond, &time))){
      sleeping = 0;
      if (error == ETIMEDOUT){
        error = 0;
      }else{
        omxcam__error ("pthread_cond_timedwait");
        error = 1;
      }
    }
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam__error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (error) return -1;
  
  if (pthread_mutex_destroy (&mutex_cond)){
    omxcam__error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    omxcam__error ("pthread_cond_destroy");
    return -1;
  }
  
  omxcam__trace ("main thread woken up");
  
  return 0;
}

static int omxcam__thread_wake (){
  omxcam__trace ("waking up from sleep");
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  sleeping = 0;
  
  if (pthread_cond_signal (&cond)){
    omxcam__error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam__error ("pthread_mutex_unlock");
    return -1;
  }

  return 0;
}

static int omxcam__thread_lock (){
  omxcam__trace ("locking main thread");
  
  if (pthread_mutex_init (&mutex_cond, 0)){
    omxcam__error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    omxcam__error ("pthread_cond_init");
    return -1;
  }
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  locked = 1;
  
  //Spurious wakeup guard
  while (locked){
    if (pthread_cond_wait (&cond, &mutex_cond)){
      omxcam__error ("pthread_cond_wait");
      locked = 0;
      return -1;
    }
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam__error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (pthread_mutex_destroy (&mutex_cond)){
    omxcam__error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    omxcam__error ("pthread_cond_destroy");
    return -1;
  }
  
  omxcam__trace ("main thread unlocked");
  
  return 0;
}

static int omxcam__thread_unlock (){
  omxcam__trace ("unlocking main thread");
  
  if (pthread_mutex_lock (&mutex_cond)){
    omxcam__error ("pthread_mutex_lock");
    return -1;
  }
  
  locked = 0;
  
  if (pthread_cond_signal (&cond)){
    omxcam__error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&mutex_cond)){
    omxcam__error ("pthread_mutex_unlock");
    return -1;
  }
  
  return 0;
}

static void omxcam__thread_handle_error (){
  omxcam__trace ("error while capturing");
  bg_error = -1;
  //Ignore the error
  omxcam_video_stop ();
  
  //Save the error after the video deinitialization in order to overwrite
  //a possible error during this task
  omxcam__set_last_error (OMXCAM_ERROR_CAPTURE);
}

static void* omxcam__video_capture (void* thread_arg){
  //The return value is not needed

  omxcam__thread_arg_t* arg = (omxcam__thread_arg_t*)thread_arg;
  int stop = 0;
  OMX_ERRORTYPE error;
  
  running_safe = 1;
  
  while (running_safe){
    //Critical section, this loop needs to be as fast as possible
  
    if (pthread_mutex_lock (&mutex)){
      omxcam__error ("pthread_mutex_lock");
      omxcam__thread_handle_error ();
      return (void*)0;
    }
    
    stop = !running;
    
    if (pthread_mutex_unlock (&mutex)){
      omxcam__error ("pthread_mutex_unlock");
      omxcam__thread_handle_error ();
      return (void*)0;
    }
    
    if (stop) break;
    
    if ((error = OMX_FillThisBuffer (arg->fill_component->handle,
        omxcam__ctx.output_buffer))){
      omxcam__error ("OMX_FillThisBuffer: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__thread_handle_error ();
      return (void*)0;
    }
    
    //Wait until it's filled
    if (omxcam__event_wait (arg->fill_component, OMXCAM_EVENT_FILL_BUFFER_DONE,
        0)){
      omxcam__thread_handle_error ();
      return (void*)0;
    }
    
    //Emit the buffer
    arg->buffer_callback (omxcam__ctx.output_buffer->pBuffer,
        omxcam__ctx.output_buffer->nFilledLen);
  }
  
  omxcam__trace ("exit thread");
  
  return (void*)0;
}

void omxcam_video_init (omxcam_video_settings_t* settings){
  omxcam__camera_init (&settings->camera, 1920, 1080);
  settings->format = OMXCAM_FORMAT_H264;
  omxcam__h264_init (&settings->h264);
  settings->buffer_callback = 0;
}

int omxcam_video_start (
    omxcam_video_settings_t* settings,
    uint32_t ms){
  omxcam__trace ("starting video capture");
  
  omxcam__set_last_error (OMXCAM_ERROR_NONE);
  
  if (!settings->buffer_callback){
    omxcam__error ("the 'buffer_callback' field must be defined");
    omxcam__set_last_error (OMXCAM_ERROR_BAD_PARAMETER);
    return -1;
  }
  if (running){
    omxcam__error ("video capture is already running");
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  bg_error = 0;
  
  if (omxcam__init ()){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (omxcam__init_omx (settings)) return -1;
  
  //Start the background thread
  omxcam__trace ("creating background thread");
  
  if (pthread_mutex_init (&mutex, 0)){
    omxcam__error ("pthread_mutex_init");
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  running = 1;
  
  if (pthread_create (&bg_thread, 0, omxcam__video_capture, &thread_arg)){
    omxcam__error ("pthread_create");
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Block the main thread
  if (ms == OMXCAM_CAPTURE_FOREVER){
    if (omxcam__thread_lock ()){
      omxcam__set_last_error (OMXCAM_ERROR_LOCK);
      return -1;
    }
  }else{
    if (omxcam__thread_sleep (ms)){
      omxcam__set_last_error (OMXCAM_ERROR_SLEEP);
      return -1;
    }
  }
  
  if (bg_error){
    //The video was already stopped due to an error
    omxcam__trace ("video stopped due to an error");
    
    int error = bg_error;
    bg_error = 0;
    
    //At this point the background thread could be still alive executing
    //deinitialization tasks, so wait until it finishes
    
    if (pthread_join (bg_thread, 0)){
      omxcam__error ("pthread_join");
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    return error;
  }
  
  if (!running){
    //The video was already stopped by the user
    omxcam__trace ("video already stopped by the user");
    
    if (pthread_join (bg_thread, 0)){
      omxcam__error ("pthread_join");
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    return 0;
  }
  
  return omxcam_video_stop ();
}

int omxcam_video_stop (){
  omxcam__trace ("stopping video capture");
  
  omxcam__set_last_error (OMXCAM_ERROR_NONE);
  
  if (!running){
    omxcam__error ("video capture is not running");
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  if (pthread_equal (pthread_self (), bg_thread)){
    //Background thread
    omxcam__trace ("stopping from background thread");
    
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
    omxcam__trace ("stopping from main thread");
    
    if (pthread_mutex_lock (&mutex)){
      omxcam__error ("pthread_mutex_lock");
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    running = 0;
    
    if (pthread_mutex_unlock (&mutex)){
      omxcam__error ("pthread_mutex_unlock");
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
    
    if (pthread_join (bg_thread, 0)){
      omxcam__error ("pthread_join");
      omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
      return -1;
    }
  }
  
  int error = omxcam__deinit_omx ();
  
  if (sleeping){
    if (omxcam__thread_wake ()){
      if (error){
        omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
        return error;
      }
      omxcam__set_last_error (OMXCAM_ERROR_WAKE);
      return -1;
    }
  }else if (locked){
    if (omxcam__thread_unlock ()){
      if (error){
        omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
        return error;
      }
      omxcam__set_last_error (OMXCAM_ERROR_UNLOCK);
      return -1;
    }
  }
  
  if (omxcam__deinit ()){
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  return 0;
}