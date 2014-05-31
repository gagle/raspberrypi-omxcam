#include "omxcam.h"

#define OMXCAM_EVENT_WAKE(x) \
  if (omxcam_event_wake (component, x)){ \
    omxcam_event_error (component); \
    return OMX_ErrorNone; \
  }

OMX_ERRORTYPE event_handler (
    OMX_IN OMX_HANDLETYPE comp,
    OMX_IN OMX_PTR app_data,
    OMX_IN OMX_EVENTTYPE event,
    OMX_IN OMX_U32 data1,
    OMX_IN OMX_U32 data2,
    OMX_IN OMX_PTR event_data){
  omxcam_component_t* component = (omxcam_component_t*)app_data;
  
  switch (event){
    case OMX_EventCmdComplete:
      switch (data1){
        case OMX_CommandStateSet:
          omxcam_trace ("event: OMX_CommandStateSet, state: %s",
              omxcam_dump_OMX_STATETYPE (data2));
          OMXCAM_EVENT_WAKE (OMXCAM_EVENT_STATE_SET);
          break;
        case OMX_CommandPortDisable:
          omxcam_trace ("event: OMX_CommandPortDisable, port: %d", data2);
          OMXCAM_EVENT_WAKE (OMXCAM_EVENT_PORT_DISABLE);
          break;
        case OMX_CommandPortEnable:
          omxcam_trace ("event: OMX_CommandPortEnable, port: %d", data2);
          OMXCAM_EVENT_WAKE (OMXCAM_EVENT_PORT_ENABLE);
          break;
        case OMX_CommandFlush:
          omxcam_trace ("event: OMX_CommandFlush, port: %d", data2);
          OMXCAM_EVENT_WAKE (OMXCAM_EVENT_FLUSH);
          break;
        case OMX_CommandMarkBuffer:
          omxcam_trace ("event: OMX_CommandMarkBuffer, port: %d", data2);
          OMXCAM_EVENT_WAKE (OMXCAM_EVENT_MARK_BUFFER);
          break;
      }
      break;
    case OMX_EventError:
      omxcam_trace ("event: %s", omxcam_dump_OMX_ERRORTYPE (data1));
      omxcam_error ("OMX_EventError: %s", omxcam_dump_OMX_ERRORTYPE (data1));
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_ERROR);
      break;
    case OMX_EventMark:
      omxcam_trace ("event: OMX_EventMark");
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_MARK);
      break;
    case OMX_EventPortSettingsChanged:
      omxcam_trace ("event: OMX_EventPortSettingsChanged, port: %d", data1);
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_PORT_SETTINGS_CHANGED);
      break;
    case OMX_EventParamOrConfigChanged:
      omxcam_trace ("event: OMX_EventParamOrConfigChanged, data1: %d, data2: "
          "%X", data1, data2);
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_PARAM_OR_CONFIG_CHANGED);
      break;
    case OMX_EventBufferFlag:
      omxcam_trace ("event: OMX_EventBufferFlag, port: %d", data1);
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_BUFFER_FLAG);
      break;
    case OMX_EventResourcesAcquired:
      omxcam_trace ("event: OMX_EventResourcesAcquired");
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_RESOURCES_ACQUIRED);
      break;
    case OMX_EventDynamicResourcesAvailable:
      omxcam_trace ("event: OMX_EventDynamicResourcesAvailable");
      OMXCAM_EVENT_WAKE (OMXCAM_EVENT_DYNAMIC_RESOURCES_AVAILABLE);
      break;
    default:
      //This should never execute, log and ignore
      omxcam_error ("event: unknown (%X)", event);
      break;
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE fill_buffer_done (
    OMX_IN OMX_HANDLETYPE comp,
    OMX_IN OMX_PTR app_data,
    OMX_IN OMX_BUFFERHEADERTYPE* buffer){
  omxcam_component_t* component = (omxcam_component_t*)app_data;
  
  omxcam_trace ("event: FillBufferDone");
  OMXCAM_EVENT_WAKE (OMXCAM_EVENT_FILL_BUFFER_DONE);
  
  return OMX_ErrorNone;
}

int omxcam_component_port_enable (omxcam_component_t* component, uint32_t port){
  omxcam_trace ("enabling port %d ('%s')", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortEnable,
      port, 0))){
    omxcam_error ("OMX_SendCommand: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_component_port_disable (
    omxcam_component_t* component,
    uint32_t port){
  omxcam_trace ("disabling port %d ('%s')", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortDisable,
      port, 0))){
    omxcam_error ("OMX_SendCommand: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_component_init (omxcam_component_t* component){
  omxcam_trace ("initializing component '%s'", component->name);

  OMX_ERRORTYPE error;
  
  if (omxcam_event_create (component)) return -1;
  
  /*if (vcos_event_flags_create (&component->flags, "component")){
    omxcam_error ("vcos_event_flags_create");
    return -1;
  }*/
  
  OMX_CALLBACKTYPE callbacks;
  callbacks.EventHandler = event_handler;
  callbacks.FillBufferDone = fill_buffer_done;
  
  if ((error = OMX_GetHandle (&component->handle, component->name, component,
      &callbacks))){
    omxcam_error ("OMX_GetHandle: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Disable all the ports
  OMX_INDEXTYPE component_types[] = {
    OMX_IndexParamAudioInit,
    OMX_IndexParamVideoInit,
    OMX_IndexParamImageInit,
    OMX_IndexParamOtherInit
  };
  OMX_PORT_PARAM_TYPE ports_st;
  OMXCAM_INIT_STRUCTURE (ports_st);

  int i;
  for (i=0; i<4; i++){
    if ((error = OMX_GetParameter (component->handle, component_types[i],
        &ports_st))){
      omxcam_error ("OMX_GetParameter - %s: %s",
          omxcam_dump_OMX_ERRORTYPE (error),
          omxcam_dump_OMX_INDEXTYPE (component_types[i]));
      return -1;
    }
    
    OMX_U32 port;
    for (port=ports_st.nStartPortNumber;
        port<ports_st.nStartPortNumber + ports_st.nPorts; port++){
      if (omxcam_component_port_disable (component, port)) return -1;
      if (omxcam_event_wait (component, OMXCAM_EVENT_PORT_DISABLE, 0)){
        return -1;
      }
    }
  }
  
  return 0;
}

int omxcam_component_deinit (omxcam_component_t* component){
  omxcam_trace ("deinitializing component '%s'", component->name);
  
  OMX_ERRORTYPE error;
  
  //vcos_event_flags_delete (&component->flags);
  if (omxcam_event_destroy (component)) return -1;

  if ((error = OMX_FreeHandle (component->handle))){
    omxcam_error ("OMX_FreeHandle: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_component_change_state (
    omxcam_component_t* component,
    omxcam_state state){
  omxcam_trace ("changing '%s' state to %s", component->name,
      omxcam_dump_OMX_STATETYPE (state));
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandStateSet, state,
      0))){
    omxcam_error ("OMX_SendCommand: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_buffer_alloc (omxcam_component_t* component, uint32_t port){
  omxcam_trace ("allocating '%s' output buffer", component->name);
  
  OMX_ERRORTYPE error;
  
  OMX_PARAM_PORTDEFINITIONTYPE def_st;
  OMXCAM_INIT_STRUCTURE (def_st);
  def_st.nPortIndex = port;
  if ((error = OMX_GetParameter (component->handle,
      OMX_IndexParamPortDefinition, &def_st))){
    omxcam_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  if ((error = OMX_AllocateBuffer (component->handle,
      &omxcam_ctx.output_buffer, port, 0, def_st.nBufferSize))){
    omxcam_error ("OMX_AllocateBuffer: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_buffer_free (omxcam_component_t* component, uint32_t port){
  omxcam_trace ("releasing '%s' output buffer", component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_FreeBuffer (component->handle, port,
      omxcam_ctx.output_buffer))){
    omxcam_error ("OMX_FreeBuffer: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_init (){
  omxcam_ctx.camera.name = OMXCAM_CAMERA_NAME;
  omxcam_ctx.image_encode.name = OMXCAM_IMAGE_ENCODE_NAME;
  omxcam_ctx.video_encode.name = OMXCAM_VIDEO_ENCODE_NAME;
  omxcam_ctx.null_sink.name = OMXCAM_NULL_SINK_NAME;
  
  bcm_host_init ();
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_Init ())){
    omxcam_error ("OMX_Init: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_deinit (){
  OMX_ERRORTYPE error;
  
  if ((error = OMX_Deinit ())){
    omxcam_error ("OMX_Deinit: %s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }

  bcm_host_deinit ();
  
  return 0;
}