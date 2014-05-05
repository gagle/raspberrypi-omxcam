#include "omxcam_video.h"

typedef struct {
  void (*bufferCallback)(uint8_t* buffer, uint32_t length);
  OMXCAM_COMPONENT* fillComponent;
} OMXCAM_THREAD_ARG;

static int useEncoder;
static int safeRunning;
static int running = 0;
static int sleeping = 0;
static int locked = 0;
static int bgError = 0;
static pthread_t bgThread;
static pthread_mutex_t mutex;
static pthread_mutex_t condMutex;
static pthread_cond_t cond;
static OMXCAM_THREAD_ARG threadArg;

static int changeVideoState (OMX_STATETYPE state, int useEncoder){
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

static OMXCAM_ERROR initOMX (OMXCAM_VIDEO_SETTINGS* settings){
  OMXCAM_trace ("Initializing video");
  
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
    case OMXCAM_FormatRGBA8888:
      OMXCAM_ctx.camera.bufferCallback = settings->bufferCallback;
      useEncoder = 0;
      colorFormat = OMX_COLOR_Format32bitABGR8888;
      stride = settings->camera.width*4;
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
      return OMXCAM_ErrorFormat;
  }
  
  threadArg.bufferCallback = settings->bufferCallback;
  threadArg.fillComponent = fillComponent;
  
  if (OMXCAM_initComponent (&OMXCAM_ctx.camera)){
    return OMXCAM_ErrorInitCamera;
  }
  if (OMXCAM_initComponent (&OMXCAM_ctx.null_sink)){
    return OMXCAM_ErrorInitNullSink;
  }
  if (useEncoder && OMXCAM_initComponent (&OMXCAM_ctx.video_encode)){
    return OMXCAM_ErrorInitVideoEncoder;
  }
  
  if (OMXCAM_loadCameraDrivers ()){
    return OMXCAM_ErrorInitCamera;
  }
  
  //Configure camera port definition
  OMXCAM_trace ("Configuring '%s' port definition", OMXCAM_ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE portDefinition;
  OMX_INIT_STRUCTURE (portDefinition);
  portDefinition.nPortIndex = 71;
  if ((error = OMX_GetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &portDefinition))){
    OMXCAM_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
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
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorBadParameter;
  }
  
  //The preview port must be configured with the same settings as the video port
  portDefinition.nPortIndex = 70;
  if ((error = OMX_SetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &portDefinition))){
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorBadParameter;
  }
  
  //The framerate must be configured (again) with its own structure in the video
  //and preview ports
  OMXCAM_trace ("Configuring %s framerate", OMXCAM_ctx.camera.name);
  
  OMX_CONFIG_FRAMERATETYPE framerate;
  OMX_INIT_STRUCTURE (framerate);
  framerate.nPortIndex = 71;
  framerate.xEncodeFramerate = portDefinition.format.video.xFramerate;
  if ((error = OMX_SetConfig (OMXCAM_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate))){
    OMXCAM_error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorBadParameter;
  }
  
  //Preview port
  framerate.nPortIndex = 70;
  if ((error = OMX_SetConfig (OMXCAM_ctx.camera.handle,
      OMX_IndexConfigVideoFramerate, &framerate))){
    OMXCAM_error ("OMX_SetConfig - OMX_IndexConfigVideoFramerate: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorBadParameter;
  }
  
  //Configure camera settings
  if (OMXCAM_setCameraSettings (&settings->camera)){
    return OMXCAM_ErrorBadParameter;
  }
  
  if (useEncoder){
    OMXCAM_trace ("Configuring '%s' port definition",
        OMXCAM_ctx.video_encode.name);
    
    OMX_INIT_STRUCTURE (portDefinition);
    portDefinition.nPortIndex = 201;
    if ((error = OMX_GetParameter (OMXCAM_ctx.video_encode.handle,
        OMX_IndexParamPortDefinition, &portDefinition))){
      OMXCAM_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
          OMXCAM_dump_OMX_ERRORTYPE (error));
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
      OMXCAM_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
          OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorBadParameter;
    }
    
    //Configure H264 settings
    if (OMXCAM_setH264Settings (&settings->h264)){
      return OMXCAM_ErrorBadParameter;
    }
    
    //Setup tunnel: camera (video) -> video_encode
    OMXCAM_trace ("Configuring tunnel '%s' -> '%s'", OMXCAM_ctx.camera.name,
        OMXCAM_ctx.video_encode.name);
    
    if ((error = OMX_SetupTunnel (OMXCAM_ctx.camera.handle, 71,
        OMXCAM_ctx.video_encode.handle, 200))){
      OMXCAM_error ("OMX_SetupTunnel: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorSetupTunnel;
    }
  }
  
  //Setup tunnel: camera (preview) -> null_sink
  OMXCAM_trace ("Configuring tunnel '%s' -> '%s'", OMXCAM_ctx.camera.name,
      OMXCAM_ctx.null_sink.name);
  
  if ((error = OMX_SetupTunnel (OMXCAM_ctx.camera.handle, 70,
      OMXCAM_ctx.null_sink.handle, 240))){
    OMXCAM_error ("OMX_SetupTunnel: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorSetupTunnel;
  }
  
  //Change to Idle
  if (changeVideoState (OMX_StateIdle, useEncoder)){
    return OMXCAM_ErrorIdle;
  }
  
  //Enable the ports
  if (OMXCAM_enablePort (&OMXCAM_ctx.camera, 71)){
    return OMXCAM_ErrorVideo;
  }
  if (!useEncoder && OMXCAM_allocateBuffer (&OMXCAM_ctx.camera, 71)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortEnable, 0)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_enablePort (&OMXCAM_ctx.camera, 70)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortEnable, 0)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_enablePort (&OMXCAM_ctx.null_sink, 240)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.null_sink, OMXCAM_EventPortEnable, 0)){
    return OMXCAM_ErrorVideo;
  }
  
  if (useEncoder){
    if (OMXCAM_enablePort (&OMXCAM_ctx.video_encode, 200)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortEnable, 0)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_enablePort (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_allocateBuffer (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortEnable, 0)){
      return OMXCAM_ErrorVideo;
    }
  }
  
  //Change to Executing
  if (changeVideoState (OMX_StateExecuting, useEncoder)){
    return OMXCAM_ErrorExecuting;
  }
  
  //Set camera capture port
  if (OMXCAM_setCapturePort (71)){
    return OMXCAM_ErrorVideo;
  }
  
  return OMXCAM_ErrorNone;
}

static OMXCAM_ERROR deinitOMX (){
  OMXCAM_trace ("Deinitializing video");
  
  running = 0;
  
  if (pthread_mutex_destroy (&mutex)){
    OMXCAM_error ("pthread_mutex_destroy");
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
    return OMXCAM_ErrorVideo;
  }
  if (!useEncoder && OMXCAM_freeBuffer (&OMXCAM_ctx.camera, 71)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortDisable, 0)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_disablePort (&OMXCAM_ctx.camera, 70)){
    return OMXCAM_ErrorVideo;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortDisable, 0)){
    return OMXCAM_ErrorVideo;
  }
  
  if (useEncoder){
    if (OMXCAM_disablePort (&OMXCAM_ctx.video_encode, 200)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_disablePort (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_freeBuffer (&OMXCAM_ctx.video_encode, 201)){
      return OMXCAM_ErrorVideo;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.video_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorVideo;
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

static int sleepThread (uint32_t ms){
  OMXCAM_trace ("Sleeping for %d ms", ms);
  
  if (pthread_mutex_init (&condMutex, 0)){
    OMXCAM_error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    OMXCAM_error ("pthread_cond_init");
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
  
  if (pthread_mutex_lock (&condMutex)){
    OMXCAM_error ("pthread_mutex_lock");
    return -1;
  }
  
  sleeping = 1;
  
  if ((error = pthread_cond_timedwait (&cond, &condMutex, &time)) &&
      error != ETIMEDOUT){
    OMXCAM_error ("pthread_cond_timedwait");
    sleeping = 0;
    error = 1;
  }else{
    error = 0;
  }
  
  sleeping = 0;
  
  if (pthread_mutex_unlock (&condMutex)){
    OMXCAM_error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (error) return -1;
  
  if (pthread_mutex_destroy (&condMutex)){
    OMXCAM_error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    OMXCAM_error ("pthread_cond_destroy");
    return -1;
  }
  
  OMXCAM_trace ("Main thread woken up");
  
  return 0;
}

static int wakeThread (){
  OMXCAM_trace ("Waking up from sleep");
  
  if (pthread_mutex_lock (&condMutex)){
    OMXCAM_error ("pthread_mutex_lock");
    return -1;
  }
  
  if (pthread_cond_signal (&cond)){
    OMXCAM_error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&condMutex)){
    OMXCAM_error ("pthread_mutex_unlock");
    return -1;
  }

  return 0;
}

static int lockThread (){
  OMXCAM_trace ("Locking main thread");
  
  if (pthread_mutex_init (&condMutex, 0)){
    OMXCAM_error ("pthread_mutex_init");
    return -1;
  }
  
  if (pthread_cond_init (&cond, 0)){
    OMXCAM_error ("pthread_cond_init");
    return -1;
  }
  
  int error = 0;
  
  if (pthread_mutex_lock (&condMutex)){
    OMXCAM_error ("pthread_mutex_lock");
    return -1;
  }
  
  locked = 1;
  
  if (pthread_cond_wait (&cond, &condMutex)){
    OMXCAM_error ("pthread_cond_wait");
    locked = 0;
    error = 1;
  }
  
  locked = 0;
  
  if (pthread_mutex_unlock (&condMutex)){
    OMXCAM_error ("pthread_mutex_unlock");
    return -1;
  }
  
  if (error) return -1;
  
  if (pthread_mutex_destroy (&condMutex)){
    OMXCAM_error ("pthread_mutex_destroy");
    return -1;
  }
  
  if (pthread_cond_destroy (&cond)){
    OMXCAM_error ("pthread_cond_destroy");
    return -1;
  }
  
  OMXCAM_trace ("Main thread unlocked");
  
  return 0;
}

static int unlockThread (){
  OMXCAM_trace ("Unlocking main thread");
  
  if (pthread_mutex_lock (&condMutex)){
    OMXCAM_error ("pthread_mutex_lock");
    return -1;
  }
  
  if (pthread_cond_signal (&cond)){
    OMXCAM_error ("pthread_cond_signal");
    return -1;
  }
  
  if (pthread_mutex_unlock (&condMutex)){
    OMXCAM_error ("pthread_mutex_unlock");
    return -1;
  }
  
  return 0;
}

static void handleError (){
  OMXCAM_trace ("Error while capturing");
  bgError = 1;
  OMXCAM_stopVideo ();
}

static void* captureVideo (void* threadArg){
  //The return value is not needed

  OMXCAM_THREAD_ARG* arg = (OMXCAM_THREAD_ARG*)threadArg;
  int stop = 0;
  OMX_ERRORTYPE error;
  
  safeRunning = 1;
  
  while (safeRunning){
    if (pthread_mutex_lock (&mutex)){
      OMXCAM_error ("pthread_mutex_lock");
      handleError ();
      return (void*)0;
    }
    
    stop = !running;
    
    if (pthread_mutex_unlock (&mutex)){
      OMXCAM_error ("pthread_mutex_unlock");
      handleError ();
      return (void*)0;
    }
    
    if (stop) break;
    
    if ((error = OMX_FillThisBuffer (arg->fillComponent->handle,
        OMXCAM_ctx.outputBuffer))){
      OMXCAM_error ("OMX_FillThisBuffer: %s",
          OMXCAM_dump_OMX_ERRORTYPE (error));
      handleError ();
      return (void*)0;
    }
    
    //Wait until it's filled
    if (OMXCAM_wait (arg->fillComponent, OMXCAM_EventFillBufferDone, 0)){
      handleError ();
      return (void*)0;
    }
    
    //Emit the buffer
    arg->bufferCallback (OMXCAM_ctx.outputBuffer->pBuffer,
        OMXCAM_ctx.outputBuffer->nFilledLen);
  }
  
  return (void*)0;
}

void OMXCAM_initVideoSettings (OMXCAM_VIDEO_SETTINGS* settings){
  OMXCAM_initCameraSettings (1920, 1080, &settings->camera);
  settings->format = OMXCAM_FormatH264;
  OMXCAM_initH264Settings (&settings->h264);
  settings->bufferCallback = 0;
}

OMXCAM_ERROR OMXCAM_startVideo (OMXCAM_VIDEO_SETTINGS* settings, uint32_t ms){
  OMXCAM_trace ("Starting video");
  
  if (!settings->bufferCallback){
    OMXCAM_error ("The 'bufferCallback' field must be defined");
    return OMXCAM_ErrorBadParameter;
  }
  if (running){
    OMXCAM_error ("Video capture is already running");
    return OMXCAM_ErrorVideo;
  }
  
  bgError = 0;
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_init ())) return error;
  
  if ((error = initOMX (settings))) return error;
  
  //Start the background thread
  OMXCAM_trace ("Creating background thread");
  
  if (pthread_mutex_init (&mutex, 0)){
    OMXCAM_error ("pthread_mutex_init");
    return OMXCAM_ErrorVideo;
  }
  
  running = 1;
  
  if (pthread_create (&bgThread, 0, captureVideo, &threadArg)){
    OMXCAM_error ("pthread_create");
    return OMXCAM_ErrorVideo;
  }
  
  //Block the main thread
  if (ms){
    if (sleepThread (ms)) return OMXCAM_ErrorSleep;
  }else{
    if (lockThread ()) return OMXCAM_ErrorLock;
  }
  
  if (bgError){
    //The video was already stopped due to an error
    OMXCAM_trace ("Video stopped due to an error");
    
    int err = bgError;
    bgError = 0;
    
    if (pthread_join (bgThread, 0)){
      OMXCAM_error ("pthread_join");
      return OMXCAM_ErrorVideo;
    }
    
    return err;
  }
  
  if (!running){
    //The video was already stopped by the client
    OMXCAM_trace ("Video stopped by the client");
    
    if (pthread_join (bgThread, 0)){
      OMXCAM_error ("pthread_join");
      return OMXCAM_ErrorVideo;
    }
    
    return OMXCAM_ErrorNone;
  }
  
  return OMXCAM_stopVideo ();
}

OMXCAM_ERROR OMXCAM_stopVideo (){
  OMXCAM_trace ("Stopping video");
  
  if (!running){
    OMXCAM_error ("Video capture is not running");
    return OMXCAM_ErrorVideo;
  }
  
  if (pthread_equal (pthread_self (), bgThread)){
    //Background thread
    OMXCAM_trace ("Stopping from background thread");
    
    //If stop() is called from inside the background thread (from the
    //bufferCallback), there's no need to use mutexes and join(), just set
    //running to false and the thread will die naturally
    running = 0;
    
    //This var is used to prevent calling mutex_lock() after mutex_destroy()
    safeRunning = 0;
  }else{
    //Main thread
    OMXCAM_trace ("Stopping from main thread");
    
    if (pthread_mutex_lock (&mutex)){
      OMXCAM_error ("pthread_mutex_lock");
      return OMXCAM_ErrorVideo;
    }
    
    running = 0;
    
    if (pthread_mutex_unlock (&mutex)){
      OMXCAM_error ("pthread_mutex_unlock");
      return OMXCAM_ErrorVideo;
    }
    
    if (pthread_join (bgThread, 0)){
      OMXCAM_error ("pthread_join");
      return OMXCAM_ErrorVideo;
    }
  }
  
  OMXCAM_ERROR error = deinitOMX ();
  
  if (sleeping){
    if (wakeThread ()){
      return error ? error : OMXCAM_ErrorWake;
    }
  }else if (locked){
    if (unlockThread ()){
      return error ? error : OMXCAM_ErrorUnlock;
    }
  }
  
  return OMXCAM_deinit ();
}