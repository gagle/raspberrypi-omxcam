#include "omxcam.h"

static int omxcam_still_change_state (omxcam_state state, int use_encoder){
  if (omxcam_component_change_state (&omxcam_ctx.camera, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (omxcam_component_change_state (&omxcam_ctx.null_sink, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  if (!use_encoder) return 0;
  
  if (omxcam_component_change_state (&omxcam_ctx.image_encode, state)){
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_STATE_SET, 0)){
    return -1;
  }
  
  return 0;
}

void omxcam_still_init (omxcam_still_settings_t* settings){
  omxcam_camera_init (&settings->camera, 2592, 1944);
  settings->format = OMXCAM_FORMAT_JPEG;
  omxcam_jpeg_init (&settings->jpeg);
  settings->buffer_callback = 0;
}

int omxcam_still_start (omxcam_still_settings_t* settings){
  omxcam_trace ("starting still capture");
  
  omxcam_set_last_error (OMXCAM_ERROR_NONE);
  
  if (!settings->buffer_callback){
    omxcam_error ("the 'buffer_callback' field must be defined");
    omxcam_set_last_error (OMXCAM_ERROR_BAD_PARAMETER);
    return -1;
  }
  
  if (omxcam_init ()){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  int use_encoder;
  OMX_COLOR_FORMATTYPE color_format;
  OMX_U32 stride;
  omxcam_component_t* fill_component;
  OMX_ERRORTYPE error;
  
  //Stride is byte-per-pixel*width
  //See mmal/util/mmal_util.c, mmal_encoding_width_to_stride()
  
  switch (settings->format){
    case OMXCAM_FORMAT_RGB888:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format24bitRGB888;
      stride = settings->camera.width*3;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_RGBA8888:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_Format32bitABGR8888;
      stride = settings->camera.width*4;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_YUV420:
      omxcam_ctx.camera.buffer_callback = settings->buffer_callback;
      use_encoder = 0;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fill_component = &omxcam_ctx.camera;
      break;
    case OMXCAM_FORMAT_JPEG:
      omxcam_ctx.image_encode.buffer_callback = settings->buffer_callback;
      use_encoder = 1;
      color_format = OMX_COLOR_FormatYUV420PackedPlanar;
      stride = settings->camera.width;
      fill_component = &omxcam_ctx.image_encode;
      break;
    default:
      omxcam_set_last_error (OMXCAM_ERROR_FORMAT);
      return -1;
  }
  
  if (omxcam_component_init (&omxcam_ctx.camera)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_CAMERA);
    return -1;
  }
  if (omxcam_component_init (&omxcam_ctx.null_sink)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam_component_init (&omxcam_ctx.image_encode)){
    omxcam_set_last_error (OMXCAM_ERROR_INIT_IMAGE_ENCODER);
    return -1;
  }
  
  if (omxcam_camera_load_drivers ()){
    omxcam_set_last_error (OMXCAM_ERROR_DRIVERS);
    return -1;
  }
  
  //Configure camera sensor
  omxcam_trace ("configuring '%s' sensor", omxcam_ctx.camera.name);
  
  OMX_PARAM_SENSORMODETYPE sensor_st;
  OMXCAM_INIT_STRUCTURE (sensor_st);
  sensor_st.nPortIndex = OMX_ALL;
  OMXCAM_INIT_STRUCTURE (sensor_st.sFrameSize);
  sensor_st.sFrameSize.nPortIndex = OMX_ALL;
  if ((error = OMX_GetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor_st))){
    omxcam_error ("OMX_GetParameter - OMX_IndexParamCommonSensorMode: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  sensor_st.bOneShot = OMX_TRUE;
  //sensor.sFrameSize.nWidth and sensor.sFrameSize.nHeight can be ignored,
  //they are configured with the port definition
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamCommonSensorMode, &sensor_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamCommonSensorMode: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Configure camera port definition
  omxcam_trace ("configuring '%s' port definition", omxcam_ctx.camera.name);
  
  OMX_PARAM_PORTDEFINITIONTYPE port_st;
  OMXCAM_INIT_STRUCTURE (port_st);
  port_st.nPortIndex = 72;
  if ((error = OMX_GetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  port_st.format.image.nFrameWidth = settings->camera.width;
  port_st.format.image.nFrameHeight = settings->camera.height;
  port_st.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
  port_st.format.image.eColorFormat = color_format;
  
  //TODO: stride, sliceHeight? http://www.raspberrypi.org/phpBB3/viewtopic.php?f=28&t=22019
  port_st.format.image.nStride = stride;
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Configure preview port at capture resolution (better performance)
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
  port_st.format.video.nStride = stride;
  if ((error = OMX_SetParameter (omxcam_ctx.camera.handle,
      OMX_IndexParamPortDefinition, &port_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Configure camera settings
  if (omxcam_camera_configure_omx (&settings->camera, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    omxcam_trace ("configuring '%s' port definition",
        omxcam_ctx.image_encode.name);
    
    OMXCAM_INIT_STRUCTURE (port_st);
    port_st.nPortIndex = 341;
    if ((error = OMX_GetParameter (omxcam_ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    port_st.format.image.nFrameWidth = settings->camera.width;
    port_st.format.image.nFrameHeight = settings->camera.height;
    port_st.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    port_st.format.image.eColorFormat = OMX_COLOR_FormatUnused;
    if ((error = OMX_SetParameter (omxcam_ctx.image_encode.handle,
        OMX_IndexParamPortDefinition, &port_st))){
      omxcam_error ("OMX_SetParameter - OMX_IndexParamPortDefinition: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    
    //Configure JPEG settings
    if (omxcam_jpeg_configure_omx (&settings->jpeg)){
      omxcam_set_last_error (OMXCAM_ERROR_JPEG);
      return -1;
    }
    
    //Setup tunnel: camera (still) -> image_encode
    omxcam_trace ("configuring tunnel '%s' -> '%s'", omxcam_ctx.camera.name,
        omxcam_ctx.image_encode.name);
    
    if ((error = OMX_SetupTunnel (omxcam_ctx.camera.handle, 72,
        omxcam_ctx.image_encode.handle, 340))){
      omxcam_error ("OMX_SetupTunnel: %s", omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
  }
  
  //Setup tunnel: camera (preview) -> null_sink
  omxcam_trace ("configuring tunnel '%s' -> '%s'", omxcam_ctx.camera.name,
      omxcam_ctx.null_sink.name);
  
  if ((error = OMX_SetupTunnel (omxcam_ctx.camera.handle, 70,
      omxcam_ctx.null_sink.handle, 240))){
    omxcam_error ("OMX_SetupTunnel: %s", omxcam_dump_OMX_ERRORTYPE (error));
    omxcam_set_last_error (OMXCAM_ERROR_VIDEO);
    return -1;
  }
  
  //Change to Idle
  if (omxcam_still_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Enable the ports
  if (omxcam_component_port_enable (&omxcam_ctx.camera, 72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (!use_encoder && omxcam_buffer_alloc (&omxcam_ctx.camera, 72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_component_port_enable (&omxcam_ctx.camera, 70)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_component_port_enable (&omxcam_ctx.null_sink, 240)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_PORT_ENABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam_component_port_enable (&omxcam_ctx.image_encode, 340)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_component_port_enable (&omxcam_ctx.image_encode, 341)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_buffer_alloc (&omxcam_ctx.image_encode, 341)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_PORT_ENABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
  }
  
  //Change to Executing
  if (omxcam_still_change_state (OMXCAM_STATE_EXECUTING, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_EXECUTING);
    return -1;
  }
  
  //Set camera capture port
  if (omxcam_camera_capture_port_set (72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Start consuming the buffers
  uint32_t end_events =
      OMXCAM_EVENT_BUFFER_FLAG | OMXCAM_EVENT_FILL_BUFFER_DONE;
  uint32_t current_events;
  
  while (1){
    //Get the buffer data (a slice of the image)
    if ((error = OMX_FillThisBuffer (fill_component->handle,
        omxcam_ctx.output_buffer))){
      omxcam_error ("OMX_FillThisBuffer: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      omxcam_set_last_error (OMXCAM_ERROR_CAPTURE);
      return -1;
    }
    
    //Wait until it's filled
    if (omxcam_event_wait (fill_component, OMXCAM_EVENT_FILL_BUFFER_DONE,
        &current_events)){
      omxcam_set_last_error (OMXCAM_ERROR_CAPTURE);
      return -1;
    }
    
    //Emit the buffer
    if (omxcam_ctx.output_buffer->nFilledLen){
      settings->buffer_callback (omxcam_ctx.output_buffer->pBuffer,
          omxcam_ctx.output_buffer->nFilledLen);
    }
    
    //When it's the end of the stream, an OMX_EventBufferFlag is emitted in all
    //the components in use. Then the FillBufferDone function is called in the
    //last component in the component's chain
    if (current_events == end_events){
      //Clear the EOS flags
      if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_BUFFER_FLAG, 0)){
        omxcam_set_last_error (OMXCAM_ERROR_CAPTURE);
        return -1;
      }
      if (use_encoder &&
          omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_BUFFER_FLAG,
          0)){
        omxcam_set_last_error (OMXCAM_ERROR_CAPTURE);
        return -1;
      }
      break;
    }
  }
  
  //Reset camera capture port
  if (omxcam_camera_capture_port_reset (72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  //Change to Idle
  if (omxcam_still_change_state (OMXCAM_STATE_IDLE, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_IDLE);
    return -1;
  }
  
  //Disable the ports
  if (omxcam_component_port_disable (&omxcam_ctx.camera, 72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (!use_encoder && omxcam_buffer_free (&omxcam_ctx.camera, 72)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_component_port_disable (&omxcam_ctx.camera, 70)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.camera, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_component_port_disable (&omxcam_ctx.null_sink, 240)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  if (omxcam_event_wait (&omxcam_ctx.null_sink, OMXCAM_EVENT_PORT_DISABLE, 0)){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  if (use_encoder){
    if (omxcam_component_port_disable (&omxcam_ctx.image_encode, 340)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_PORT_DISABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_component_port_disable (&omxcam_ctx.image_encode, 341)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_buffer_free (&omxcam_ctx.image_encode, 341)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
    if (omxcam_event_wait (&omxcam_ctx.image_encode, OMXCAM_EVENT_PORT_DISABLE,
        0)){
      omxcam_set_last_error (OMXCAM_ERROR_STILL);
      return -1;
    }
  }
  
  //Change to Loaded
  if (omxcam_still_change_state (OMXCAM_STATE_LOADED, use_encoder)){
    omxcam_set_last_error (OMXCAM_ERROR_LOADED);
    return -1;
  }
  
  if (omxcam_component_deinit (&omxcam_ctx.camera)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_CAMERA);
    return -1;
  }
  if (omxcam_component_deinit (&omxcam_ctx.null_sink)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_NULL_SINK);
    return -1;
  }
  if (use_encoder && omxcam_component_deinit (&omxcam_ctx.image_encode)){
    omxcam_set_last_error (OMXCAM_ERROR_DEINIT_IMAGE_ENCODER);
    return -1;
  }
  
  if (omxcam_deinit ()){
    omxcam_set_last_error (OMXCAM_ERROR_STILL);
    return -1;
  }
  
  return 0;
}

int omxcam_still_stop (){
  omxcam_trace ("stopping still capture");
  
  omxcam_set_last_error (OMXCAM_ERROR_NONE);
  
  
  
  return 0;
}