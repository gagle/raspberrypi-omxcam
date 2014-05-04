#include "omxcam.h"

OMXCAM_ERROR OMXCAM_init (){
  OMXCAM_ctx.camera.name = "OMX.broadcom.camera";
  OMXCAM_ctx.image_encode.name = "OMX.broadcom.image_encode";
  OMXCAM_ctx.video_encode.name = "OMX.broadcom.video_encode";
  OMXCAM_ctx.null_sink.name = "OMX.broadcom.null_sink";
  
  bcm_host_init ();
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_Init ())){
    OMXCAM_error ("OMX_Init: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorInit;
  }
  
  return OMXCAM_ErrorNone;
}

OMXCAM_ERROR OMXCAM_deinit (){
  OMX_ERRORTYPE error;
  
  if ((error = OMX_Deinit ())){
    OMXCAM_error ("OMX_Deinit: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return OMXCAM_ErrorDeinit;
  }

  bcm_host_deinit ();
  
  return OMXCAM_ErrorNone;
}