#include "omxcam.h"
#include "internal.h"

static int omxcam__still_change_state (omxcam__state state, int use_encoder){
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
  
  if (omxcam__component_change_state (&omxcam__ctx.image_encode, state)){
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.image_encode, OMXCAM_EVENT_STATE_SET,
      0)){
    return -1;
  }
  
  return 0;
}

void omxcam_still_init (omxcam_still_settings_t* settings){
  omxcam__camera_init (&settings->camera, 2592, 1944);
  settings->format = OMXCAM_FORMAT_JPEG;
  omxcam__jpeg_init (&settings->jpeg);
  settings->buffer_callback = 0;
}

int omxcam_still_start (omxcam_still_settings_t* settings){
  omxcam__trace ("starting still capture");
  
  omxcam__set_last_error (OMXCAM_ERROR_NONE);
  
  if (!settings->buffer_callback){
    omxcam__error ("the 'buffer_callback' field must be defined");
    omxcam__set_last_error (OMXCAM_ERROR_BAD_PARAMETER);
    return -1;
  }
  if (settings->camera.width < settings->camera.height){
    omxcam__error ("'camera.width' must be >= than 'camera.height'");
    omxcam__set_last_error (OMXCAM_ERROR_BAD_PARAMETER);
    return -1;
  }
  
  if (omxcam__init ()){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  int use_encoder;
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
    case OMXCAM_FORMAT_JPEG:
      omxcam__ctx.image_encode.buffer_callback = settings->buffer_callback;
      use_encoder = 1;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      width = settings->camera.width;
      height = settings->camera.height;
      fill_component = &omxcam__ctx.image_encode;
      break;
    default:
      omxcam__set_last_error (OMXCAM_ERROR_FORMAT);
      return -1;
  }
  
  omxcam__trace ("%dx%d", settings->camera.width, settings->camera.height);
  
  if (omxcam__component_init (&omxcam__ctx.camera)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_CAMERA);
    return -1;
  }
  if (omxcam__component_init (&omxcam__ctx.null_sink)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam__component_init (&omxcam__ctx.image_encode)){
    omxcam__set_last_error (OMXCAM_ERROR_INIT_IMAGE_ENCODER);
    return -1;
  }
  
  if (omxcam__camera_load_drivers ()){
    omxcam__set_last_error (OMXCAM_ERROR_DRIVERS);
    return -1;
  }
  
  //Configure camera sensor
  omxcam__trace ("configuring '%s' sensor", omxcam__ctx.camera.name);
  
  OMX_PARAM_SENSORMODETYPE sensor_st;
  omxcam__omx_struct_init (sensor_st);
  sensor_st.nPortIndex = OMX_ALL;
  omxcam__omx_struct_init (sensor_st.sFrameSize);
  sensor_st.sFrameSize.nPortIndex = OMX_ALL;
  if ((error = OMX_GetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor_st))){
    omxcam__error ("OMX_GetParameter - OMX_IndexParamCommonSensorMode: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  sensor_st.bOneShot = OMX_TRUE;
  //sensor.sFrameSize.nWidth and sensor.sFrameSize.nHeight can be ignored,
  //they are configured with the port definition
  if ((error = OMX_SetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamCommonSensorMode: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Configure camera port definition
  omxcam__trace ("configuring '%s' port definition", omxcam__ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE port_st;
  omxcam__omx_struct_init (port_st);
  port_st.nPortIndex = 72;
  if ((error = OMX_GetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  port_st.format.image.nFrameWidth = width;
  port_st.format.image.nFrameHeight = height;
  port_st.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
  port_st.format.image.eColorFormat = color_format;
  port_st.format.image.nStride = stride;
  if ((error = OMX_SetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (error ==  OMX_ErrorBadParameter
        ? OMXCAM_ERROR_BAD_PARAMETER
        : OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Configure preview port at full resolution (better performance)
  //1920x1080 15fps
  port_st.nPortIndex = 70;
  //omxcam_round (1920, 32) -> 1920
  //omxcam_round (1080, 16) -> 1088
  port_st.format.video.nFrameWidth = 1920;
  port_st.format.video.nFrameHeight = 1088;
  port_st.format.video.eCompressionFormat = OMX_IMAGE_CodingUnused;
  port_st.format.video.eColorFormat = color_format;
  //15fps, 15 << 16 -> 983040
  port_st.format.video.xFramerate = 983040;
  port_st.format.video.nStride = 1920;
  if ((error = OMX_SetParameter (omxcam__ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam__dump_OMX_ERRORTYPE (error));
    omxcam__set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Configure camera settings
  if (omxcam__camera_configure_omx (&settings->camera, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    omxcam__trace ("configuring '%s' port definition",
        omxcam__ctx.image_encode.name);
    
    omxcam__omx_struct_init (port_st);
    port_st.nPortIndex = 341;
    if ((error = OMX_GetParameter (omxcam__ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam__error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    port_st.format.image.nFrameWidth = settings->camera.width;
    port_st.format.image.nFrameHeight = settings->camera.height;
    port_st.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    port_st.format.image.eColorFormat = OMX_COLOR_FormatUnused;
    port_st.format.image.nStride = stride;
    if ((error = OMX_SetParameter (omxcam__ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam__error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    
    //Configure JPEG settings
    if (omxcam__jpeg_configure_omx (&settings->jpeg)){
      omxcam__set_last_error (OMXCAM_ERROR_JPEG);
      return -1;
    }
    
    //Setup tunnel: camera (still) -> image_encode
    omxcam__trace ("configuring tunnel '%s' -> '%s'", omxcam__ctx.camera.name,
        omxcam__ctx.image_encode.name);
    
    if ((error = OMX_SetupTunnel (omxcam__ctx.camera.handle, 72,
        omxcam__ctx.image_encode.handle, 340))){
      omxcam__error ("OMX_SetupTunnel: %s", omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
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
  if (omxcam__still_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Enable the ports
  if (omxcam__component_port_enable (&omxcam__ctx.camera, 72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (!use_encoder && omxcam__buffer_alloc (&omxcam__ctx.camera, 72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__component_port_enable (&omxcam__ctx.camera, 70)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__component_port_enable (&omxcam__ctx.null_sink, 240)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.null_sink, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam__component_port_enable (&omxcam__ctx.image_encode, 340)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.image_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__component_port_enable (&omxcam__ctx.image_encode, 341)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__buffer_alloc (&omxcam__ctx.image_encode, 341)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.image_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
  }
  
  //Change to Executing
  if (omxcam__still_change_state (OMXCAM_STATE_EXECUTING, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_EXECUTING);
    return -1;
  }
  
  //Set camera capture port
  if (omxcam__camera_capture_port_set (72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Start consuming the buffers
  uint32_t end_events =
      OMXCAM_EVENT_BUFFER_FLAG | OMXCAM_EVENT_FILL_BUFFER_DONE;
  uint32_t current_events;
  
  while (1){
    //Get the buffer data (a slice of the image)
    if ((error = OMX_FillThisBuffer (fill_component->handle,
        omxcam__ctx.output_buffer))){
      omxcam__error ("OMX_FillThisBuffer: %s",
          omxcam__dump_OMX_ERRORTYPE (error));
      omxcam__set_last_error (OMXCAM_ERROR_CAPTURE);
      return -1;
    }
    
    //Wait until it's filled
    if (omxcam__event_wait (fill_component, OMXCAM_EVENT_FILL_BUFFER_DONE,
        &current_events)){
      omxcam__set_last_error (OMXCAM_ERROR_CAPTURE);
      return -1;
    }
    
    //Emit the buffer
    if (omxcam__ctx.output_buffer->nFilledLen){
      settings->buffer_callback (omxcam__ctx.output_buffer->pBuffer,
          omxcam__ctx.output_buffer->nFilledLen);
    }
    
    //When it's the end of the stream, an OMX_EventBufferFlag is emitted in all
    //the components in use. Then the FillBufferDone function is called in the
    //last component in the component's chain
    if (current_events == end_events){
      //Clear the EOS flags
      if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_BUFFER_FLAG,
          0)){
        omxcam__set_last_error (OMXCAM_ERROR_CAPTURE);
        return -1;
      }
      if (use_encoder &&
          omxcam__event_wait (&omxcam__ctx.image_encode,
              OMXCAM_EVENT_BUFFER_FLAG, 0)){
        omxcam__set_last_error (OMXCAM_ERROR_CAPTURE);
        return -1;
      }
      break;
    }
  }
  
  //Reset camera capture port
  if (omxcam__camera_capture_port_reset (72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Change to Idle
  if (omxcam__still_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam__set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Disable the ports
  if (omxcam__component_port_disable (&omxcam__ctx.camera, 72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (!use_encoder && omxcam__buffer_free (&omxcam__ctx.camera, 72)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__component_port_disable (&omxcam__ctx.camera, 70)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__component_port_disable (&omxcam__ctx.null_sink, 240)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam__event_wait (&omxcam__ctx.null_sink, OMXCAM_EVENT_PORT_DISABLE,
      0)){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam__component_port_disable (&omxcam__ctx.image_encode, 340)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.image_encode,
        OMXCAM_EVENT_PORT_DISABLE, 0)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__component_port_disable (&omxcam__ctx.image_encode, 341)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__buffer_free (&omxcam__ctx.image_encode, 341)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam__event_wait (&omxcam__ctx.image_encode,
        OMXCAM_EVENT_PORT_DISABLE, 0)){
      omxcam__set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
  }
  
  //Change to Loaded
  if (omxcam__still_change_state (OMXCAM_STATE_LOADED, use_encoder)){
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
  if (use_encoder && omxcam__component_deinit (&omxcam__ctx.image_encode)){
    omxcam__set_last_error (OMXCAM_ERROR_DEINIT_IMAGE_ENCODER);
    return -1;
  }
  
  if (omxcam__deinit ()){
    omxcam__set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  return 0;
}

int omxcam_still_stop (){
  omxcam__trace ("stopping still capture");
  
  omxcam__set_last_error (OMXCAM_ERROR_NONE);
  
  
  
  return 0;
}