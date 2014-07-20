#include "omxcam.h"
#include "internal.h"

void omxcam__h264_init (omxcam_h264_settings_t* settings){
  //Decent bitrate for 1080p
  settings->bitrate = 17000000;
  //If the IDR period is set to off, only one IDR frame will be inserted at the
  //beginning of the stream
  settings->idr_period = OMXCAM_H264_IDR_PERIOD_OFF;
  settings->sei = OMXCAM_FALSE;
}

int omxcam__h264_validate (omxcam_h264_settings_t* settings){
  if (!omxcam__h264_is_valid_bitrate (settings->bitrate)){
    omxcam__error ("invalid 'h264.bitrate' value");
    return -1;
  }
  return 0;
}

int omxcam__h264_is_valid_bitrate (uint32_t bitrate){
  return bitrate >= 1 && bitrate <= 25000000;
}

int omxcam__h264_configure_omx (omxcam_h264_settings_t* settings){
  omxcam__trace ("configuring '%s' settings", omxcam__ctx.video_encode.name);
  
  OMX_ERRORTYPE error;
  
  //Bitrate
  OMX_VIDEO_PARAM_BITRATETYPE bitrate_st;
  omxcam__omx_struct_init (bitrate_st);
  bitrate_st.eControlRate = OMX_Video_ControlRateVariable;
  bitrate_st.nTargetBitrate = settings->bitrate;
  bitrate_st.nPortIndex = 201;
  if ((error = OMX_SetParameter (omxcam__ctx.video_encode.handle,
      OMX_IndexParamVideoBitrate, &bitrate_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamVideoBitrate: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Codec
  OMX_VIDEO_PARAM_PORTFORMATTYPE format_st;
  omxcam__omx_struct_init (format_st);
  format_st.nPortIndex = 201;
  //H.264/AVC
  format_st.eCompressionFormat = OMX_VIDEO_CodingAVC;
  if ((error = OMX_SetParameter (omxcam__ctx.video_encode.handle,
      OMX_IndexParamVideoPortFormat, &format_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamVideoPortFormat: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //IDR period
  OMX_VIDEO_CONFIG_AVCINTRAPERIOD idr_st;
  omxcam__omx_struct_init (idr_st);
  idr_st.nPortIndex = 201;
  if ((error = OMX_GetConfig (omxcam__ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr_st))){
    omxcam__error ("OMX_GetConfig - OMX_IndexConfigVideoAVCIntraPeriod: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    return -1;
  }
  idr_st.nIDRPeriod = settings->idr_period;
  if ((error = OMX_SetConfig (omxcam__ctx.video_encode.handle,
      OMX_IndexConfigVideoAVCIntraPeriod, &idr_st))){
    omxcam__error ("OMX_SetConfig - OMX_IndexConfigVideoAVCIntraPeriod: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //SEI messages
  OMX_PARAM_BRCMVIDEOAVCSEIENABLETYPE sei_st;
  omxcam__omx_struct_init (sei_st);
  sei_st.nPortIndex = 201;
  sei_st.bEnable = !!settings->sei;
  if ((error = OMX_SetParameter (omxcam__ctx.video_encode.handle,
      OMX_IndexParamBrcmVideoAVCSEIEnable, &sei_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamBrcmVideoAVCSEIEnable: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}