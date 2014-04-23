#include "omxcam_video.h"

typedef struct {
  uint32_t (*bufferCallback)(uint8_t* buffer, uint32_t length);
  void (*errorCallback)(OMXCAM_ERROR error);
  OMXCAM_COMPONENT* fillComponent;
} OMXCAM_THREAD_ARG;

static int useEncoder;
static int running = 0;
static int safeRunning;
static int bgError;
static pthread_t mainThread;
static pthread_t bgThread;
static pthread_mutex_t mutex;
static OMXCAM_THREAD_ARG threadArg;

int changeVideoState (OMX_STATETYPE state, int useEncoder){
  if (OMXCAM_changeState (&OMXCAM_ctx.camera, state)){
    return -1;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventStateSet, 0)){
    return -1;
  }
  
  if (OMXCAM_changeState (&OMXCAM_ctx.null_sink, state)){
    return -1;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.null_sink, OMXCAM_EventStateSet, 0)){
    return -1;
  }
  
  if (!useEncoder) return 0;
  
  if (OMXCAM_changeState (&OMXCAM_ctx.video_encode, state)){
    return -1;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventStateSet, 0)){
    return -1;
  }
  if (state == OMX_StateExecuting &&
      OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortSettingsChanged,
      0)){
    return -1;
  }
  
  return 0;
}

static OMXCAM_ERROR deinit (){
  OMXCAM_trace ("deinitializing video\n");

  if (pthread_mutex_destroy (&mutex)){
    OMXCAM_setError ("%s: pthread_mutex_init", __func__);
    return OMXCAM_ErrorVideo;
  }

  //Reset camera capture port
  if (OMXCAM_resetCapturePort (71)){
    return OMXCAM_ErrorVideo;
  }
  
  //Change to Idle
  if (changeVideoState (OMX_StateIdle, useEncoder)){
    return OMXCAM_ErrorIdle;
  }
  
  //Disable the ports
  if (OMXCAM_disablePort (&OMXCAM_ctx.camera, 71)){
    return OMXCAM_ErrorStill;
  }
  if (!useEncoder && OMXCAM_freeBuffer (&OMXCAM_ctx.camera, 71)){
    return OMXCAM_ErrorStill;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortDisable, 0)){
    return OMXCAM_ErrorStill;
  }
  if (OMXCAM_disablePort (&OMXCAM_ctx.camera, 70)){
    return OMXCAM_ErrorStill;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortDisable, 0)){
    return OMXCAM_ErrorStill;
  }
  
  if (useEncoder){
    if (OMXCAM_disablePort (&OMXCAM_ctx.video_encode, 200)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_disablePort (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_freeBuffer (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorStill;
    }
  }
  
  //Change to Loaded
  if (changeVideoState (OMX_StateLoaded, useEncoder)){
    return OMXCAM_ErrorLoaded;
  }
  
  if (OMXCAM_deinitComponent (&OMXCAM_ctx.camera)){
    return OMXCAM_ErrorDeinitCamera;
  }
  if (OMXCAM_deinitComponent (&OMXCAM_ctx.null_sink)){
    return OMXCAM_ErrorDeinitNullSink;
  }
  if (useEncoder && OMXCAM_deinitComponent (&OMXCAM_ctx.video_encode)){
    return OMXCAM_ErrorDeinitVideoEncoder;
  }
  
  return OMXCAM_ErrorNone;
}

void OMXCAM_initVideoSettings (OMXCAM_VIDEO_SETTINGS* settings){
  OMXCAM_initCameraSettings (1920, 1080, &settings->camera);
  settings->format = OMXCAM_FormatH264;
  OMXCAM_initH264Settings (&settings->h264);
  settings->bufferCallback = 0;
  settings->errorCallback = 0;
}

void* capture (void* threadArg){
  //The return value is not needed

  OMXCAM_THREAD_ARG* arg = (OMXCAM_THREAD_ARG*)threadArg;
  int stop = 0;
  OMX_ERRORTYPE error;
  OMXCAM_ERROR err;
  
  safeRunning = 1;
  
  while (safeRunning){
    if (pthread_mutex_lock (&mutex)){
      OMXCAM_setError ("%s: pthread_mutex_lock", __func__);
      running = 0;
      bgError = 1;
      err = deinit ();
      if (arg->errorCallback){
        arg->errorCallback (err ? err : OMXCAM_ErrorVideoThread);
      }
      return (void*)0;
    }
    
    stop = !running;
    
    if (pthread_mutex_unlock (&mutex)){
      OMXCAM_setError ("%s: pthread_mutex_unlock", __func__);
      running = 0;
      bgError = 1;
      err = deinit ();
      if (arg->errorCallback){
        arg->errorCallback (err ? err : OMXCAM_ErrorVideoThread);
      }
      return (void*)0;
    }
    
    if (stop) break;
    
    if ((error = OMX_FillThisBuffer (arg->fillComponent->handle,
        OMXCAM_ctx.outputBuffer))){
      OMXCAM_setError ("%s: OMX_FillThisBuffer: %s", __func__,
          OMXCAM_dump_OMX_ERRORTYPE (error));
      running = 0;
      bgError = 1;
      err = deinit ();
      if (arg->errorCallback){
        arg->errorCallback (err ? err : OMXCAM_ErrorVideoThread);
      }
      return (void*)0;
    }
    
    //Wait until it's filled
    if (OMXCAM_wait (arg->fillComponent, OMXCAM_EventFillBufferDone, 0)){
      running = 0;
      bgError = 1;
      err = deinit ();
      if (arg->errorCallback){
        arg->errorCallback (err ? err : OMXCAM_ErrorVideoThread);
      }
      return (void*)0;
    }
    
    //Emit the buffer
    if (arg->bufferCallback &&
        arg->bufferCallback (OMXCAM_ctx.outputBuffer->pBuffer,
        OMXCAM_ctx.outputBuffer->nFilledLen)){
      running = 0;
      bgError = 1;
      return (void*)OMXCAM_ErrorVideo;
    }
  }
  
  return (void*)0;
}

OMXCAM_ERROR OMXCAM_startVideo (OMXCAM_VIDEO_SETTINGS* settings){
  OMXCAM_trace ("Starting video\n");
  
  if (running){
    OMXCAM_setError ("%s: Video capture is already running", __func__);
    return OMXCAM_ErrorVideo;
  }
  
  running = 1;
  bgError = 0;
  
  OMX_COLOR_FORMATTYPE colorFormat;
  OMX_U32 stride;
  OMXCAM_COMPONENT* fillComponent;
  OMX_ERRORTYPE error;
  
  //Stride is byte-per-pixel*width
  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
  
  switch (settings->format){
    case OMXCAM_FormatRGB888:
      OMXCAM_ctx.camera.bufferCallback = settings->bufferCallback;
      useEncoder = 0;
      colorFormat = OMX_COLOR_Format24bitRGB888;
      stride = settings->camera.width*3;
      fillComponent = &OMXCAM_ctx.camera;
      break;
    case OMXCAM_FormatYUV420:
      OMXCAM_ctx.camera.bufferCallback = settings->bufferCallback;
      useEncoder = 0;
      colorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fillComponent = &OMXCAM_ctx.camera;
      break;
    case OMXCAM_FormatH264:
      OMXCAM_ctx.video_encode.bufferCallback = settings->bufferCallback;
      useEncoder = 1;
      colorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fillComponent = &OMXCAM_ctx.video_encode;
      break;
    default:
      running = 0;
      return OMXCAM_ErrorFormat;
  }
  
  if (OMXCAM_initComponent (&OMXCAM_ctx.camera)){
    running = 0;
    return OMXCAM_ErrorInitCamera;
  }
  if (OMXCAM_initComponent (&OMXCAM_ctx.null_sink)){
    running = 0;
    return OMXCAM_ErrorInitNullSink;
  }
  if (useEncoder && OMXCAM_initComponent (&OMXCAM_ctx.video_encode)){
    running = 0;
    return OMXCAM_ErrorInitVideoEncoder;
  }
  
  if (OMXCAM_loadCameraDrivers ()){
    running = 0;
    return OMXCAM_ErrorInitCamera;
  }
  
  //Configure camera port definition
  OMXCAM_trace ("Configuring '%s' port definition\n", OMXCAM_ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE portDefinition;
  OMX_INIT_STRUCTURE (portDefinition);
  portDefinition.nPortIndex = 71;
  if ((error = OMX_GetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &portDefinition))){
    OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  
  portDefinition.format.video.nFrameWidth = settings->camera.width;
  portDefinition.format.video.nFrameHeight = settings->camera.height;
  portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
  portDefinition.format.video.eColorFormat = colorFormat;
  portDefinition.format.video.xFramerate = settings->camera.framerate << 16;
  
  //TODO: stride, sliceHeight? http://www.raspberrypi.org/phpBB3/viewtopic.php?f=28&t=22019
  portDefinition.format.video.nStride = stride;
  if ((error = OMX_SetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &portDefinition))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorBadParameter;
  }
  
  //The preview port must be configured with the same settings as the video port
  portDefinition.nPortIndex = 70;
  if ((error = OMX_SetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &portDefinition))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorBadParameter;
  }
  
  //The framerate must be configured (again) with its own structure in the video
  //and preview ports
  OMXCAM_trace ("Configuring %s framerate\n", OMXCAM_ctx.camera.name);
  
  OMX_CONFIG_FRAMERATETYPE framerate;
  OMX_INIT_STRUCTURE (framerate);
  framerate.nPortIndex = 71;
  framerate.xEncodeFramerate = portDefinition.format.video.xFramerate;
  if ((error = OMX_SetConfig (OMXCAM_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate))){
    OMXCAM_setError ("%s: OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s\n",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorBadParameter;
  }
  
  //Preview port
  framerate.nPortIndex = 70;
  if ((error = OMX_SetConfig (OMXCAM_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate))){
    OMXCAM_setError ("%s: OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s\n",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorBadParameter;
  }
  
  //Configure camera settings
  if (OMXCAM_setCameraSettings (&settings->camera)){
    running = 0;
    return OMXCAM_ErrorBadParameter;
  }
  
  if (useEncoder){
    OMXCAM_trace ("Configuring '%s' port definition\n",
        OMXCAM_ctx.video_encode.name);
    
    OMX_INIT_STRUCTURE (portDefinition);
    portDefinition.nPortIndex = 201;
    if ((error = OMX_GetParameter (OMXCAM_ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &portDefinition))){
      OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamPortDefinition: "
          "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
      running = 0;
      return OMXCAM_ErrorStill;
    }
    
    portDefinition.format.video.nFrameWidth = settings->camera.width;
    portDefinition.format.video.nFrameHeight = settings->camera.height;
    portDefinition.format.video.xFramerate = settings->camera.framerate << 16;
    portDefinition.format.video.nStride = stride;
    
    //Despite being configured later, these two fields need to be set
    portDefinition.format.video.nBitrate = settings->h264.bitrate;
    portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    
    if ((error = OMX_SetParameter (OMXCAM_ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &portDefinition))){
      OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamPortDefinition: "
          "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
      running = 0;
      return OMXCAM_ErrorBadParameter;
    }
    
    //Configure H264 settings
    if (OMXCAM_setH264Settings (&settings->h264)){
      running = 0;
      return OMXCAM_ErrorBadParameter;
    }
    
    //Setup tunnel: camera (video) -> video_encode
    OMXCAM_trace ("Configuring tunnel '%s' -> '%s'\n", OMXCAM_ctx.camera.name,
        OMXCAM_ctx.video_encode.name);
    
    if ((error = OMX_SetupTunnel (OMXCAM_ctx.camera.handle, 71,
        OMXCAM_ctx.video_encode.handle, 200))){
      OMXCAM_setError ("%s: OMX_SetupTunnel: %s", __func__,
          OMXCAM_dump_OMX_ERRORTYPE (error));
      running = 0;
      return OMXCAM_ErrorSetupTunnel;
    }
  }
  
  //Setup tunnel: camera (preview) -> null_sink
  OMXCAM_trace ("Configuring tunnel '%s' -> '%s'\n", OMXCAM_ctx.camera.name,
      OMXCAM_ctx.null_sink.name);
  
  if ((error = OMX_SetupTunnel (OMXCAM_ctx.camera.handle, 70,
      OMXCAM_ctx.null_sink.handle, 240))){
    OMXCAM_setError ("%s: OMX_SetupTunnel: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    running = 0;
    return OMXCAM_ErrorSetupTunnel;
  }
  
  //Change to Idle
  if (changeVideoState (OMX_StateIdle, useEncoder)){
    running = 0;
    return OMXCAM_ErrorIdle;
  }
  
  //Enable the ports
  if (OMXCAM_enablePort (&OMXCAM_ctx.camera, 71)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (!useEncoder && OMXCAM_allocateBuffer (&OMXCAM_ctx.camera, 71)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortEnable, 0)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_enablePort (&OMXCAM_ctx.camera, 70)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortEnable, 0)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_enablePort (&OMXCAM_ctx.null_sink, 240)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.null_sink, OMXCAM_EventPortEnable, 0)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  
  if (useEncoder){
    if (OMXCAM_enablePort (&OMXCAM_ctx.video_encode, 200)){
      running = 0;
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortEnable, 0)){
      running = 0;
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_enablePort (&OMXCAM_ctx.video_encode, 201)){
      running = 0;
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_allocateBuffer (&OMXCAM_ctx.video_encode, 201)){
      running = 0;
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortEnable, 0)){
      running = 0;
      return OMXCAM_ErrorVideo;
    }
  }
  
  //Change to Executing
  if (changeVideoState (OMX_StateExecuting, useEncoder)){
    running = 0;
    return OMXCAM_ErrorExecuting;
  }
  
  //Set camera capture port
  if (OMXCAM_setCapturePort (71)){
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  
  //Save the thread id
  mainThread = pthread_self ();
  
  OMXCAM_trace ("Creating background thread\n");
  
  //Start the background thread
  if (pthread_mutex_init (&mutex, 0)){
    OMXCAM_setError ("%s: pthread_mutex_init", __func__);
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  
  threadArg.bufferCallback = settings->bufferCallback;
  threadArg.errorCallback = settings->errorCallback;
  threadArg.fillComponent = fillComponent;
  
  if (pthread_create (&bgThread, 0, capture, &threadArg)){
    OMXCAM_setError ("%s: pthread_create", __func__);
    running = 0;
    return OMXCAM_ErrorVideo;
  }
  
  return OMXCAM_ErrorNone;
}

OMXCAM_ERROR OMXCAM_stopVideo (){
  OMXCAM_trace ("Stopping video\n");
  
  //Don't return an error if an error occured inside the background thread
  //because the video was already deinitialized automatically, so this call to
  //stop() should be no-op
  if (bgError) return OMXCAM_ErrorVideoThread;

  if (!running){
    OMXCAM_setError ("%s: Video capture is not running", __func__);
    return OMXCAM_ErrorVideo;
  }
  
  //Check whether it's the main or background thread
  pthread_t thread = pthread_self ();
  
  if (pthread_equal (thread, mainThread)){
    OMXCAM_trace ("Stopping from main thread\n");
    
    if (pthread_mutex_lock (&mutex)){
      OMXCAM_setError ("%s: pthread_mutex_lock", __func__);
      return OMXCAM_ErrorVideo;
    }
    
    running = 0;
    
    if (pthread_mutex_unlock (&mutex)){
      OMXCAM_setError ("%s: pthread_mutex_unlock", __func__);
      return OMXCAM_ErrorVideo;
    }
    
    if (pthread_join (bgThread, 0)){
      OMXCAM_setError ("%s: pthread_join", __func__);
      return OMXCAM_ErrorVideo;
    }
  }else{
    //Background
    OMXCAM_trace ("Stopping from background thread\n");
    
    //If stop() is called from inside the background thread (from the
    //bufferCallback), there's no need to use mutexes and join(), just set
    //running to false and the thread will die naturally
    running = 0;
    
    //This var is used to prevent calling mutex_lock() after mutex_destroy()
    safeRunning = 0;
  }
  
  return deinit ();
}