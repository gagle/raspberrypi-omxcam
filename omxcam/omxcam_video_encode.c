#include "omxcam_video_encode.h"

void OMXCAM_initH264Settings (OMXCAM_H264_SETTINGS* settings){
  settings->bitrate = 10000000;
  settings->idrPeriod = 60;
}

int OMXCAM_setH264Settings (OMXCAM_H264_SETTINGS* settings){
  OMXCAM_trace ("Configuring '%s' settings\n", OMXCAM_ctx.video_encode.name);
  
  OMX_ERRORTYPE error;
  
  //Bitrate
  OMX_VIDEO_PARAM_BITRATETYPE bitrate;
  OMX_INIT_STRUCTURE (bitrate);
  bitrate.eControlRate = OMX_Video_ControlRateVariable;
  bitrate.nTargetBitrate = settings->bitrate;
  bitrate.nPortIndex = 201;
  if ((error = OMX_SetParameter (OMXCAM_ctx.video_encode.handle,
      OMX_IndexParamVideoBitrate, &bitrate))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamVideoBitrate: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Codec
  OMX_VIDEO_PARAM_PORTFORMATTYPE format;
  OMX_INIT_STRUCTURE (format);
  format.nPortIndex = 201;
  //H.264/AVC
  format.eCompressionFormat = OMX_VIDEO_CodingAVC;
  if ((error = OMX_SetParameter (OMXCAM_ctx.video_encode.handle,
      OMX_IndexParamVideoPortFormat, &format))){
    OMXCAM_setError ("%s: OMX_SetParameter - OMX_IndexParamVideoPortFormat: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //IDR period
  OMX_VIDEO_CONFIG_AVCINTRAPERIOD idr;
  OMX_INIT_STRUCTURE (idr);
  idr.nPortIndex = 201;
  if ((error = OMX_GetConfig (OMXCAM_ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr))){
    OMXCAM_setError ("%s: OMX_GetConfig - OMX_IndexConfigVideoAVCIntraPeriod: "
        "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  idr.nIDRPeriod = settings->idrPeriod;
  if ((error = OMX_SetConfig (OMXCAM_ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr))){
    OMXCAM_setError ("%s: OMX_SetConfig - OMX_IndexConfigVideoAVCIntraPeriod: "
        "%s", __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}