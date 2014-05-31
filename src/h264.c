#include "omxcam.h"

void omxcam_h264_init (omxcam_h264_settings_t* settings){
  settings->bitrate = 10000000;
  settings->idr_period = 60;
}

int omxcam_h264_configure_omx (omxcam_h264_settings_t* settings){
  omxcam_trace ("configuring '%s' settings", omxcam_ctx.video_encode.name);
  
  OMX_ERRORTYPE error;
  
  //Bitrate
  OMX_VIDEO_PARAM_BITRATETYPE bitrate_st;
  OMXCAM_INIT_STRUCTURE (bitrate_st);
  bitrate_st.eControlRate = OMX_Video_ControlRateVariable;
  bitrate_st.nTargetBitrate = settings->bitrate;
  bitrate_st.nPortIndex = 201;
  if ((error = OMX_SetParameter (omxcam_ctx.video_encode.handle,
      OMX_IndexParamVideoBitrate, &bitrate_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamVideoBitrate: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Codec
  OMX_VIDEO_PARAM_PORTFORMATTYPE format_st;
  OMXCAM_INIT_STRUCTURE (format_st);
  format_st.nPortIndex = 201;
  //H.264/AVC
  format_st.eCompressionFormat = OMX_VIDEO_CodingAVC;
  if ((error = OMX_SetParameter (omxcam_ctx.video_encode.handle,
      OMX_IndexParamVideoPortFormat, &format_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamVideoPortFormat: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //IDR period
  OMX_VIDEO_CONFIG_AVCINTRAPERIOD idr_st;
  OMXCAM_INIT_STRUCTURE (idr_st);
  idr_st.nPortIndex = 201;
  if ((error = OMX_GetConfig (omxcam_ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr_st))){
    omxcam_error ("OMX_GetConfig - OMX_IndexConfigVideoAVCIntraPeriod: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  idr_st.nIDRPeriod = settings->idr_period;
  if ((error = OMX_SetConfig (omxcam_ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigVideoAVCIntraPeriod: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}