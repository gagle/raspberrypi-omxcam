#include "omxcam.h"

int omxcam_camera_load_drivers (){
  /*
  This is a specific behaviour of the Broadcom's Raspberry Pi OpenMAX IL
  implementation module because the OMX_SetConfig() and OMX_SetParameter() are
  blocking functions but the drivers are loaded asynchronously, that is, an
  event is fired to signal the completion. Basically, what you're saying is:
  
  "When the parameter with index OMX_IndexParamCameraDeviceNumber is set, load
  the camera drivers and emit an OMX_EventParamOrConfigChanged event"
  
  The red led of the camera will be turned on after this call.
  */
  
  omxcam_trace ("loading '%s' drivers", omxcam_ctx.camera.name);
  
  OMX_ERRORTYPE error;

  OMX_CONFIG_REQUESTCALLBACKTYPE req_st;
  OMXCAM_INIT_STRUCTURE (req_st);
  req_st.nPortIndex = OMX_ALL;
  req_st.nIndex = OMX_IndexParamCameraDeviceNumber;
  req_st.bEnable = OMX_TRUE;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigRequestCallback, &req_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigRequestCallback: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  OMX_PARAM_U32TYPE dev_st;
  OMXCAM_INIT_STRUCTURE (dev_st);
  dev_st.nPortIndex = OMX_ALL;
  //ID for the camera device
  dev_st.nU32 = 0;
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamCameraDeviceNumber, &dev_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamCameraDeviceNumber: "
        "%s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return omxcam_event_wait (&omxcam_ctx.camera,
      OMXCAM_EVENT_PARAM_OR_CONFIG_CHANGED, 0);
}

static int omxcam_config_capture_port (uint32_t port, OMX_BOOL set){
  OMX_ERRORTYPE error;
  
  OMX_CONFIG_PORTBOOLEANTYPE port_st;
  OMXCAM_INIT_STRUCTURE (port_st);
  port_st.nPortIndex = port;
  port_st.bEnabled = set;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigPortCapturing, &port_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigPortCapturing: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_camera_capture_port_set (uint32_t port){
  omxcam_trace ("setting '%s' capture port", omxcam_ctx.camera.name);
  return omxcam_config_capture_port (port, OMX_TRUE);
}

int omxcam_camera_capture_port_reset (uint32_t port){
  omxcam_trace ("resetting '%s' capture port", omxcam_ctx.camera.name);
  return omxcam_config_capture_port (port, OMX_FALSE);
}

void omxcam_camera_init (
    omxcam_camera_settings_t* settings,
    uint32_t width,
    uint32_t height){
  settings->width = width;
  settings->height = height;
  settings->sharpness = 0;
  settings->contrast = 0;
  settings->brightness = 50;
  settings->saturation = 0;
  settings->shutter_speed_auto = OMXCAM_TRUE;
  //1/8 seconds in milliseconds
  settings->shutter_speed = 125;
  settings->iso_auto = OMXCAM_TRUE;
  settings->iso = 100;
  settings->exposure = OMXCAM_EXPOSURE_AUTO;
  settings->exposure_compensation = 0;
  settings->mirror = OMXCAM_MIRROR_NONE;
  settings->rotation = OMXCAM_ROTATION_NONE;
  settings->color_enable = OMXCAM_FALSE;
  settings->color_u = 128;
  settings->color_v = 128;
  settings->noise_reduction_enable = OMXCAM_TRUE;
  settings->frame_stabilisation_enable = OMXCAM_FALSE;
  settings->metering = OMXCAM_METERING_AVERAGE;
  settings->white_balance = OMXCAM_WHITE_BALANCE_AUTO;
  settings->white_balance_red_gain = 0.1;
  settings->white_balance_blue_gain = 0.1;
  settings->image_filter = OMXCAM_IMAGE_FILTER_NONE;
  settings->framerate = 30;
}

int omxcam_camera_configure_omx (omxcam_camera_settings_t* settings, int video){
  omxcam_trace ("configuring '%s' settings", omxcam_ctx.camera.name);

  OMX_ERRORTYPE error;
  
  //Sharpness
  OMX_CONFIG_SHARPNESSTYPE sharpness_st;
  OMXCAM_INIT_STRUCTURE (sharpness_st);
  sharpness_st.nPortIndex = OMX_ALL;
  sharpness_st.nSharpness = settings->sharpness;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonSharpness, &sharpness_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonSharpness: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Contrast
  OMX_CONFIG_CONTRASTTYPE contrast_st;
  OMXCAM_INIT_STRUCTURE (contrast_st);
  contrast_st.nPortIndex = OMX_ALL;
  contrast_st.nContrast = settings->contrast;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonContrast, &contrast_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonContrast: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Saturation
  OMX_CONFIG_SATURATIONTYPE saturation_st;
  OMXCAM_INIT_STRUCTURE (saturation_st);
  saturation_st.nPortIndex = OMX_ALL;
  saturation_st.nSaturation = settings->saturation;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonSaturation, &saturation_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonSaturation: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Brightness
  OMX_CONFIG_BRIGHTNESSTYPE brightness_st;
  OMXCAM_INIT_STRUCTURE (brightness_st);
  brightness_st.nPortIndex = OMX_ALL;
  brightness_st.nBrightness = settings->brightness;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonBrightness, &brightness_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonBrightness: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Exposure value
  OMX_CONFIG_EXPOSUREVALUETYPE exposure_value_st;
  OMXCAM_INIT_STRUCTURE (exposure_value_st);
  exposure_value_st.nPortIndex = OMX_ALL;
  exposure_value_st.eMetering = settings->metering;
  exposure_value_st.xEVCompensation =
      (OMX_S32)((settings->exposure_compensation<<16)/6.0);
  exposure_value_st.nShutterSpeedMsec = settings->shutter_speed*1000;
  exposure_value_st.bAutoShutterSpeed = settings->shutter_speed_auto;
  exposure_value_st.nSensitivity = settings->iso;
  exposure_value_st.bAutoSensitivity = settings->iso_auto;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonExposureValue, &exposure_value_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonExposureValue: "
        "%s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Exposure control
  OMX_CONFIG_EXPOSURECONTROLTYPE exposure_control_st;
  OMXCAM_INIT_STRUCTURE (exposure_control_st);
  exposure_control_st.nPortIndex = OMX_ALL;
  exposure_control_st.eExposureControl = settings->exposure;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonExposure, &exposure_control_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonExposure: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Frame stabilisation
  OMX_CONFIG_FRAMESTABTYPE frame_stabilisation_st;
  OMXCAM_INIT_STRUCTURE (frame_stabilisation_st);
  frame_stabilisation_st.nPortIndex = OMX_ALL;
  frame_stabilisation_st.bStab = settings->frame_stabilisation_enable;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonFrameStabilisation, &frame_stabilisation_st))){
    omxcam_error ("OMX_SetConfig - "
        "OMX_IndexConfigCommonFrameStabilisation: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //White balance
  OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_st;
  OMXCAM_INIT_STRUCTURE (white_balance_st);
  white_balance_st.nPortIndex = OMX_ALL;
  white_balance_st.eWhiteBalControl = settings->white_balance;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonWhiteBalance, &white_balance_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonWhiteBalance: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //White balance gains (if white balance is set to off)
  if (!settings->white_balance){
    OMX_CONFIG_CUSTOMAWBGAINSTYPE white_balance_gains_st;
    OMXCAM_INIT_STRUCTURE (white_balance_gains_st);
    white_balance_gains_st.xGainR =
        (OMX_U32)(settings->white_balance_red_gain*65536);
    white_balance_gains_st.xGainB =
        (OMX_U32)(settings->white_balance_blue_gain*65536);
    if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
        OMX_IndexConfigCustomAwbGains, &white_balance_gains_st))){
      omxcam_error ("OMX_SetConfig - OMX_IndexConfigCustomAwbGains: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      return -1;
    }
  }
  
  //Image filter
  OMX_CONFIG_IMAGEFILTERTYPE image_filter_st;
  OMXCAM_INIT_STRUCTURE (image_filter_st);
  image_filter_st.nPortIndex = OMX_ALL;
  image_filter_st.eImageFilter = settings->image_filter;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonImageFilter, &image_filter_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonImageFilter: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Mirror
  OMX_CONFIG_MIRRORTYPE mirror_st;
  OMXCAM_INIT_STRUCTURE (mirror_st);
  mirror_st.nPortIndex = video ? 71 : 72;
  mirror_st.eMirror = settings->mirror;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonMirror, &mirror_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonMirror: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Rotation
  OMX_CONFIG_ROTATIONTYPE rotation_st;
  OMXCAM_INIT_STRUCTURE (rotation_st);
  rotation_st.nPortIndex = video ? 71 : 72;
  rotation_st.nRotation = settings->rotation;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonRotate, &rotation_st))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigCommonRotate: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Color enhancement
  OMX_CONFIG_COLORENHANCEMENTTYPE color_enhancement_st;
  OMXCAM_INIT_STRUCTURE (color_enhancement_st);
  color_enhancement_st.nPortIndex = OMX_ALL;
  color_enhancement_st.bColorEnhancement = settings->color_enable;
  color_enhancement_st.nCustomizedU = settings->color_u;
  color_enhancement_st.nCustomizedV = settings->color_v;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigCommonColorEnhancement, &color_enhancement_st))){
    omxcam_error ("OMX_SetConfig - "
        "OMX_IndexConfigCommonColorEnhancement: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Denoise
  OMX_CONFIG_BOOLEANTYPE denoise_st;
  OMXCAM_INIT_STRUCTURE (denoise_st);
  denoise_st.bEnabled = settings->noise_reduction_enable;
  if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
      OMX_IndexConfigStillColourDenoiseEnable, &denoise_st))){
    omxcam_error ("OMX_SetConfig - "
        "OMX_IndexConfigStillColourDenoiseEnable: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}