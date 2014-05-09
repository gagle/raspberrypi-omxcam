#include "omxcam_core.h"

OMX_ERRORTYPE EventHandler (
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1,
    OMX_IN OMX_U32 nData2,
    OMX_IN OMX_PTR pEventData){
  OMXCAM_COMPONENT* component = (OMXCAM_COMPONENT*)pAppData;
  
  switch (eEvent){
    case OMX_EventCmdComplete:
      switch (nData1){
        case OMX_CommandStateSet:
          OMXCAM_trace ("EVENT: OMX_CommandStateSet, state: %s",
              OMXCAM_dump_OMX_STATETYPE (nData2));
          OMXCAM_continue (component, OMXCAM_EventStateSet);
          break;
        case OMX_CommandPortDisable:
          OMXCAM_trace ("EVENT: OMX_CommandPortDisable, port: %d", nData2);
          OMXCAM_continue (component, OMXCAM_EventPortDisable);
          break;
        case OMX_CommandPortEnable:
          OMXCAM_trace ("EVENT: OMX_CommandPortEnable, port: %d", nData2);
          OMXCAM_continue (component, OMXCAM_EventPortEnable);
          break;
        case OMX_CommandFlush:
          OMXCAM_trace ("EVENT: OMX_CommandFlush, port: %d", nData2);
          OMXCAM_continue (component, OMXCAM_EventFlush);
          break;
        case OMX_CommandMarkBuffer:
          OMXCAM_trace ("EVENT: OMX_CommandMarkBuffer, port: %d", nData2);
          OMXCAM_continue (component, OMXCAM_EventMarkBuffer);
          break;
      }
      break;
    case OMX_EventError:
      OMXCAM_trace ("EVENT: %s", OMXCAM_dump_OMX_ERRORTYPE (nData1));
      OMXCAM_error ("OMX_EventError: %s", OMXCAM_dump_OMX_ERRORTYPE (nData1));
      OMXCAM_continue (component, OMXCAM_EventError);
      break;
    case OMX_EventMark:
      OMXCAM_trace ("EVENT: OMX_EventMark");
      OMXCAM_continue (component, OMXCAM_EventMark);
      break;
    case OMX_EventPortSettingsChanged:
      OMXCAM_trace ("EVENT: OMX_EventPortSettingsChanged, port: %d", nData1);
      OMXCAM_continue (component, OMXCAM_EventPortSettingsChanged);
      break;
    case OMX_EventParamOrConfigChanged:
      OMXCAM_trace ("EVENT: OMX_EventParamOrConfigChanged, nData1: %d, nData2: "
          "%X", nData1, nData2);
      OMXCAM_continue (component, OMXCAM_EventParamOrConfigChanged);
      break;
    case OMX_EventBufferFlag:
      OMXCAM_trace ("EVENT: OMX_EventBufferFlag, port: %d", nData1);
      OMXCAM_continue (component, OMXCAM_EventBufferFlag);
      break;
    case OMX_EventResourcesAcquired:
      OMXCAM_trace ("EVENT: OMX_EventResourcesAcquired");
      OMXCAM_continue (component, OMXCAM_EventResourcesAcquired);
      break;
    case OMX_EventDynamicResourcesAvailable:
      OMXCAM_trace ("EVENT: OMX_EventDynamicResourcesAvailable");
      OMXCAM_continue (component, OMXCAM_EventDynamicResourcesAvailable);
      break;
    default:
      //This should never execute, just ignore
      OMXCAM_trace ("EVENT: Unknown (%X)", eEvent);
      OMXCAM_error ("Unkown event: %X", eEvent);
      break;
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone (
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer){
  OMXCAM_COMPONENT* component = (OMXCAM_COMPONENT*)pAppData;
  
  OMXCAM_trace ("EVENT: FillBufferDone");
  OMXCAM_continue (component, OMXCAM_EventFillBufferDone);
  
  return OMX_ErrorNone;
}

int OMXCAM_wait (
    OMXCAM_COMPONENT* component,
    VCOS_UNSIGNED events,
    VCOS_UNSIGNED* retrievedEvents){
  VCOS_UNSIGNED set;
  if (vcos_event_flags_get (&component->flags, events | OMXCAM_EventError,
      VCOS_OR_CONSUME, VCOS_SUSPEND, &set)){
    OMXCAM_error ("vcos_event_flags_get");
    return -1;
  }
  if (set == OMXCAM_EventError){
    //OMXCAM_error() is set in the OMX's EventHandler
    return -1;
  }
  if (retrievedEvents){
    *retrievedEvents = set;
  }
  
  return 0;
}

void OMXCAM_continue (OMXCAM_COMPONENT* component, VCOS_UNSIGNED event){
  vcos_event_flags_set (&component->flags, event, VCOS_OR);
}

static const char* getPortType (OMX_INDEXTYPE type){
  switch (type){
    case OMX_IndexParamAudioInit:
      return "OMX_IndexParamAudioInit";
    case OMX_IndexParamVideoInit:
      return "OMX_IndexParamVideoInit";
    case OMX_IndexParamImageInit:
      return "OMX_IndexParamImageInit";
    case OMX_IndexParamOtherInit:
      return "OMX_IndexParamOtherInit";
    default:
      return "unknown";
  }
}

int OMXCAM_initComponent (OMXCAM_COMPONENT* component){
  OMXCAM_trace ("Initializing component '%s'", component->name);

  OMX_ERRORTYPE error;
  
  if (vcos_event_flags_create (&component->flags, "component")){
    OMXCAM_error ("vcos_event_flags_create");
    return -1;
  }
  
  OMX_CALLBACKTYPE callbacks;
  callbacks.EventHandler = EventHandler;
  callbacks.FillBufferDone = FillBufferDone;
  
  if ((error = OMX_GetHandle (&component->handle, component->name, component,
      &callbacks))){
    OMXCAM_error ("OMX_GetHandle: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Disable all the ports
  OMX_INDEXTYPE types[] = {
    OMX_IndexParamAudioInit,
    OMX_IndexParamVideoInit,
    OMX_IndexParamImageInit,
    OMX_IndexParamOtherInit
  };
  OMX_PORT_PARAM_TYPE ports;
  OMX_INIT_STRUCTURE (ports);

  int i;
  for (i=0; i<4; i++){
    if ((error = OMX_GetParameter (component->handle, types[i], &ports))){
      OMXCAM_error ("OMX_GetParameter - %s: %s",
          OMXCAM_dump_OMX_ERRORTYPE (error), getPortType (types[i]));
      return -1;
    }
    
    OMX_U32 port;
    for (port=ports.nStartPortNumber;
        port<ports.nStartPortNumber + ports.nPorts; port++){
      if (OMXCAM_disablePort (component, port)) return -1;
      if (OMXCAM_wait (component, OMXCAM_EventPortDisable, 0)) return -1;
    }
  }
  
  return 0;
}

int OMXCAM_deinitComponent (OMXCAM_COMPONENT* component){
  OMXCAM_trace ("Deinitializing component '%s'", component->name);
  
  OMX_ERRORTYPE error;
  
  vcos_event_flags_delete (&component->flags);

  if ((error = OMX_FreeHandle (component->handle))){
    OMXCAM_error ("OMX_FreeHandle: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_changeState (OMXCAM_COMPONENT* component, OMX_STATETYPE state){
  OMXCAM_trace ("Changing '%s' state to %s", component->name,
      OMXCAM_dump_OMX_STATETYPE (state));
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandStateSet, state,
      0))){
    OMXCAM_error ("OMX_SendCommand: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_enablePort (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Enabling port %d ('%s')", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortEnable,
      port, 0))){
    OMXCAM_error ("OMX_SendCommand: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_disablePort (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Disabling port %d ('%s')", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortDisable,
      port, 0))){
    OMXCAM_error ("OMX_SendCommand: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_allocateBuffer (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Allocating '%s' output buffer", component->name);
  
  OMX_ERRORTYPE error;
  
  OMX_PARAM_PORTDEFINITIONTYPE def;
  OMX_INIT_STRUCTURE (def);
  def.nPortIndex = port;
  if ((error = OMX_GetParameter (component->handle,
      OMX_IndexParamPortDefinition, &def))){
    OMXCAM_error ("OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  if ((error = OMX_AllocateBuffer (component->handle,
      &OMXCAM_ctx.outputBuffer, port, 0, def.nBufferSize))){
    OMXCAM_error ("OMX_AllocateBuffer: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_freeBuffer (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Releasing '%s' output buffer", component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_FreeBuffer (component->handle, port,
      OMXCAM_ctx.outputBuffer))){
    OMXCAM_error ("OMX_FreeBuffer: %s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

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