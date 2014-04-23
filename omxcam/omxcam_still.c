#include "omxcam_still.h"

static int changeStillState (OMX_STATETYPE state, int useEncoder){
  if (OMXCAM_changeState (&OMXCAM_ctx.camera, state)){
    return -1;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventStateSet, 0)){
    return -1;
  }
  
  if (!useEncoder) return 0;
  
  if (OMXCAM_changeState (&OMXCAM_ctx.image_encode, state)){
    return -1;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventStateSet, 0)){
    return -1;
  }
  
  return 0;
}

void OMXCAM_initStillSettings (OMXCAM_STILL_SETTINGS* settings){
  OMXCAM_initCameraSettings (2592, 1944, &settings->camera);
  settings->format = OMXCAM_FormatJPEG;
  OMXCAM_initJpegSettings (&settings->jpeg);
  settings->bufferCallback = 0;
}

OMXCAM_ERROR OMXCAM_still (OMXCAM_STILL_SETTINGS* settings){
  OMXCAM_trace ("Capturing still\n");
  
  int useEncoder;
  OMX_COLOR_FORMATTYPE colorFormat;
  OMX_U32 stride;
  OMXCAM_COMPONENT* fillComponent;
  OMX_ERRORTYPE error;
  
  //Stride is byte-per-pixel*width
  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
  
  switch (settings->format){
    case OMXCAM_FormatRGB888:
      OMXCAM_ctx.camera.bufferCallback = settings->bufferCallback;
      useEncoder = OMXCAM_FALSE;
      colorFormat = OMX_COLOR_Format24bitRGB888;
      stride = settings->camera.width*3;
      fillComponent = &OMXCAM_ctx.camera;
      break;
    case OMXCAM_FormatYUV420:
      OMXCAM_ctx.camera.bufferCallback = settings->bufferCallback;
      useEncoder = OMXCAM_FALSE;
      colorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fillComponent = &OMXCAM_ctx.camera;
      break;
    case OMXCAM_FormatJPEG:
      OMXCAM_ctx.image_encode.bufferCallback = settings->bufferCallback;
      useEncoder = OMXCAM_TRUE;
      colorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fillComponent = &OMXCAM_ctx.image_encode;
      break;
    default:
      return OMXCAM_ErrorFormat;
  }

  if (OMXCAM_initComponent (&OMXCAM_ctx.camera)){
    return OMXCAM_ErrorInitCamera;
  }
  if (useEncoder && OMXCAM_initComponent (&OMXCAM_ctx.image_encode)){
    return OMXCAM_ErrorInitImageEncoder;
  }
  
  if (OMXCAM_loadCameraDrivers ()) return OMXCAM_ErrorInitCamera;
  
  //Configure camera sensor
  OMXCAM_trace ("Configuring '%s' sensor\n", OMXCAM_ctx.camera.name);
  
  OMX_PARAM_SENSORMODETYPE sensor;
  OMX_INIT_STRUCTURE (sensor);
  sensor.nPortIndex = OMX_ALL;
  OMX_INIT_STRUCTURE (sensor.sFrameSize);
  sensor.sFrameSize.nPortIndex = OMX_ALL;
  if ((error = OMX_GetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor))){
    OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamCommonSensorMode: "
        "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorInitCamera;
  }
  sensor.bOneShot = OMX_TRUE;
  //sensor.sFrameSize.nWidth and sensor.sFrameSize.nHeight can be ignored,
  //they are configured with the port definition
  if ((error = OMX_SetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamCommonSensorMode: "
        "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorInitCamera;
  }

  //Change to Idle
  if (changeStillState (OMX_StateIdle, useEncoder)){
    return OMXCAM_ErrorIdle;
  }
  
  //Configure camera port definition
  OMXCAM_trace ("Configuring '%s' port definition\n", OMXCAM_ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE def;
  OMX_INIT_STRUCTURE (def);
  def.nPortIndex = 72;
  if ((error = OMX_GetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &def))){
    OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorStill;
  }
  
  def.format.image.nFrameWidth = settings->camera.width;
  def.format.image.nFrameHeight = settings->camera.height;
  def.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
  def.format.image.eColorFormat = colorFormat;
  
  //TODO: stride, sliceHeight? http://www.raspberrypi.org/phpBB3/viewtopic.php?f=28&t=22019
  def.format.image.nStride = stride;
  if ((error = OMX_SetParameter (OMXCAM_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &def))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorBadParameter;
  }
  
  //Configure camera settings
  if (OMXCAM_setCameraSettings (&settings->camera)){
    return OMXCAM_ErrorBadParameter;
  }
  
  if (useEncoder){
    OMXCAM_trace ("Configuring '%s' port definition\n",
        OMXCAM_ctx.image_encode.name);
    
    OMX_INIT_STRUCTURE (def);
    def.nPortIndex = 341;
    if ((error = OMX_GetParameter (OMXCAM_ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &def))){
      OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamPortDefinition: "
          "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorStill;
    }
    def.format.image.nFrameWidth = settings->camera.width;
    def.format.image.nFrameHeight = settings->camera.height;
    def.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    def.format.image.eColorFormat = OMX_COLOR_FormatUnused;
    if ((error = OMX_SetParameter (OMXCAM_ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &def))){
      OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamPortDefinition: "
          "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorBadParameter;
    }
    
    //Configure JPEG settings
    if (OMXCAM_setJpegSettings (&settings->jpeg)){
      return OMXCAM_ErrorBadParameter;
    }
    
    //Setup tunnel: camera (still) -> image_encode
    OMXCAM_trace ("Configuring tunnel '%s' -> '%s'\n", OMXCAM_ctx.camera.name,
        OMXCAM_ctx.image_encode.name);
    
    if ((error = OMX_SetupTunnel (OMXCAM_ctx.camera.handle, 72,
        OMXCAM_ctx.image_encode.handle, 340))){
      OMXCAM_setError ("%s: OMX_SetupTunnel: %s", __func__,
          OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorSetupTunnel;
    }
  }
  
  //Enable the ports
  if (OMXCAM_enablePort (&OMXCAM_ctx.camera, 72)){
    return OMXCAM_ErrorStill;
  }
  if (!useEncoder && OMXCAM_allocateBuffer (&OMXCAM_ctx.camera, 72)){
    return OMXCAM_ErrorStill;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortEnable, 0)){
    return OMXCAM_ErrorStill;
  }
  
  if (useEncoder){
    if (OMXCAM_enablePort (&OMXCAM_ctx.image_encode, 340)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventPortEnable, 0)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_enablePort (&OMXCAM_ctx.image_encode, 341)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_allocateBuffer (&OMXCAM_ctx.image_encode, 341)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventPortEnable, 0)){
      return OMXCAM_ErrorStill;
    }
  }
  
  //Change to Executing
  if (changeStillState (OMX_StateExecuting, useEncoder)){
    return OMXCAM_ErrorExecuting;
  }
  
  //Set camera capture port
  if (OMXCAM_setCapturePort (72)){
    return OMXCAM_ErrorStill;
  }
  
  //Start consuming the buffers
  VCOS_UNSIGNED endFlags = OMXCAM_EventBufferFlag | OMXCAM_EventFillBufferDone;
  VCOS_UNSIGNED retrievedEvents;
  
  while (1){
    //Get the buffer data (a slice of the image)
    if ((error = OMX_FillThisBuffer (fillComponent->handle,
        OMXCAM_ctx.outputBuffer))){
      OMXCAM_setError ("%s: OMX_FillThisBuffer: %s", __func__,
          OMXCAM_dump_OMX_ERRORTYPE (error));
      return OMXCAM_ErrorStill;
    }
    
    //Wait until it's filled
    if (OMXCAM_wait (fillComponent, OMXCAM_EventFillBufferDone,
        &retrievedEvents)){
      return OMXCAM_ErrorStill;
    }
    
    //Emit the buffer
    if (settings->bufferCallback &&
        OMXCAM_ctx.outputBuffer->nFilledLen &&
        settings->bufferCallback (OMXCAM_ctx.outputBuffer->pBuffer,
        OMXCAM_ctx.outputBuffer->nFilledLen)){
      return OMXCAM_ErrorStill;
    }
    
    //When it's the end of the stream, an OMX_EventBufferFlag is emitted in all
    //the components in use. Then the FillBufferDone function is called in the
    //last component in the component's chain
    if (retrievedEvents == endFlags){
      //Clear the EOS flags
      if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventBufferFlag, 0)){
        return OMXCAM_ErrorStill;
      }
      if (useEncoder &&
          OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventBufferFlag, 0)){
        return OMXCAM_ErrorStill;
      }
      break;
    }
  }
  
  //Reset camera capture port
  if (OMXCAM_resetCapturePort (72)){
    return OMXCAM_ErrorStill;
  }
  
  //Change to Idle
  if (changeStillState (OMX_StateIdle, useEncoder)){
    return OMXCAM_ErrorIdle;
  }
  
  //Disable the ports
  if (OMXCAM_disablePort (&OMXCAM_ctx.camera, 72)){
    return OMXCAM_ErrorStill;
  }
  if (!useEncoder && OMXCAM_freeBuffer (&OMXCAM_ctx.camera, 72)){
    return OMXCAM_ErrorStill;
  }
  if (OMXCAM_wait (&OMXCAM_ctx.camera, OMXCAM_EventPortDisable, 0)){
    return OMXCAM_ErrorStill;
  }
  
  if (useEncoder){
    if (OMXCAM_disablePort (&OMXCAM_ctx.image_encode, 340)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_disablePort (&OMXCAM_ctx.image_encode, 341)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_freeBuffer (&OMXCAM_ctx.image_encode, 341)){
      return OMXCAM_ErrorStill;
    }
    if (OMXCAM_wait (&OMXCAM_ctx.image_encode, OMXCAM_EventPortDisable, 0)){
      return OMXCAM_ErrorStill;
    }
  }
  
  //Change to Loaded
  if (changeStillState (OMX_StateLoaded, useEncoder)){
    return OMXCAM_ErrorLoaded;
  }
  
  if (OMXCAM_deinitComponent (&OMXCAM_ctx.camera)){
    return OMXCAM_ErrorDeinitCamera;
  }
  if (useEncoder && OMXCAM_deinitComponent (&OMXCAM_ctx.image_encode)){
    return OMXCAM_ErrorDeinitImageEncoder;
  }
  
  return OMXCAM_ErrorNone;
}