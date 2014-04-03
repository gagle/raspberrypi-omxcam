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
          OMXCAM_trace ("EVENT: OMX_CommandStateSet, state: %s\n",
              OMXCAM_dump_OMX_STATETYPE (nData2));
          OMXCAM_wake (component, OMXCAM_EventStateSet);
          break;
        case OMX_CommandPortDisable:
          OMXCAM_trace ("EVENT: OMX_CommandPortDisable, port: %d\n", nData2);
          OMXCAM_wake (component, OMXCAM_EventPortDisable);
          break;
        case OMX_CommandPortEnable:
          OMXCAM_trace ("EVENT: OMX_CommandPortEnable, port: %d\n", nData2);
          OMXCAM_wake (component, OMXCAM_EventPortEnable);
          break;
        case OMX_CommandFlush:
          OMXCAM_trace ("EVENT: OMX_CommandFlush, port: %d\n", nData2);
          OMXCAM_wake (component, OMXCAM_EventFlush);
          break;
        case OMX_CommandMarkBuffer:
          OMXCAM_trace ("EVENT: OMX_CommandMarkBuffer, port: %d\n", nData2);
          OMXCAM_wake (component, OMXCAM_EventMarkBuffer);
          break;
      }
      break;
    case OMX_EventError:
      OMXCAM_trace ("EVENT: %s\n", OMXCAM_dump_OMX_ERRORTYPE (nData1));
      OMXCAM_setError ("%s: OMX_EventError: %s", __func__,
          OMXCAM_dump_OMX_ERRORTYPE (nData1));
      OMXCAM_wake (component, OMXCAM_EventError);
      break;
    case OMX_EventMark:
      OMXCAM_trace ("EVENT: OMX_EventMark\n");
      OMXCAM_wake (component, OMXCAM_EventMark);
      break;
    case OMX_EventPortSettingsChanged:
      OMXCAM_trace ("EVENT: OMX_EventPortSettingsChanged, port: %d\n", nData1);
      OMXCAM_wake (component, OMXCAM_EventPortSettingsChanged);
      break;
    case OMX_EventParamOrConfigChanged:
      OMXCAM_trace ("EVENT: OMX_EventParamOrConfigChanged, nData1: %d, nData2: "
          "%X\n", nData1, nData2);
      OMXCAM_wake (component, OMXCAM_EventParamOrConfigChanged);
      break;
    case OMX_EventBufferFlag:
      OMXCAM_trace ("EVENT: OMX_EventBufferFlag, port: %d\n", nData1);
      OMXCAM_wake (component, OMXCAM_EventBufferFlag);
      break;
    case OMX_EventResourcesAcquired:
      OMXCAM_trace ("EVENT: OMX_EventResourcesAcquired\n");
      OMXCAM_wake (component, OMXCAM_EventResourcesAcquired);
      break;
    case OMX_EventDynamicResourcesAvailable:
      OMXCAM_trace ("EVENT: OMX_EventDynamicResourcesAvailable\n");
      OMXCAM_wake (component, OMXCAM_EventDynamicResourcesAvailable);
      break;
    default:
      //This should never execute, just ignore
      OMXCAM_trace ("EVENT: Unknown (%X)\n", eEvent);
      break;
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone (
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer){
  OMXCAM_COMPONENT* component = (OMXCAM_COMPONENT*)pAppData;
  
  OMXCAM_trace ("EVENT: FillBufferDone\n");
  OMXCAM_wake (component, OMXCAM_EventFillBufferDone);
  
  return OMX_ErrorNone;
}

void OMXCAM_wake (OMXCAM_COMPONENT* component, VCOS_UNSIGNED event){
  vcos_event_flags_set (&component->flags, event, VCOS_OR);
}

int OMXCAM_wait (
    OMXCAM_COMPONENT* component,
    VCOS_UNSIGNED events,
    VCOS_UNSIGNED* retrievedEvents){
  VCOS_UNSIGNED set;
  if (vcos_event_flags_get (&component->flags, events | OMXCAM_EventError,
      VCOS_OR_CONSUME, VCOS_SUSPEND, &set)){
    OMXCAM_setError ("%s: vcos_event_flags_get", __func__);
    return -1;
  }
  if (set == OMXCAM_EventError){
    //OMXCAM_setError() is set in the OMX's EventHandler
    return -1;
  }
  if (retrievedEvents){
    *retrievedEvents = set;
  }
  
  return 0;
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
  OMXCAM_trace ("Initializing component '%s'\n", component->name);

  OMX_ERRORTYPE error;
  
  if (vcos_event_flags_create (&component->flags, "component")){
    OMXCAM_setError ("%s: vcos_event_flags_create", __func__);
    return -1;
  }
  
  OMX_CALLBACKTYPE callbacks;
  callbacks.EventHandler = EventHandler;
  callbacks.FillBufferDone = FillBufferDone;
  
  if ((error = OMX_GetHandle (&component->handle, component->name, component,
      &callbacks))){
    OMXCAM_setError ("%s: OMX_GetHandle: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
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
      OMXCAM_setError ("%s: OMX_GetParameter - %s: %s", __func__,
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
  OMXCAM_trace ("Deinitializing component '%s'\n", component->name);
  
  OMX_ERRORTYPE error;
  
  vcos_event_flags_delete (&component->flags);

  if ((error = OMX_FreeHandle (component->handle))){
    OMXCAM_setError ("%s: OMX_FreeHandle: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_changeState (OMXCAM_COMPONENT* component, OMX_STATETYPE state){
  OMXCAM_trace ("Changing '%s' state to %s\n", component->name,
      OMXCAM_dump_OMX_STATETYPE (state));
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandStateSet, state,
      0))){
    OMXCAM_setError ("%s: OMX_SendCommand: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_enablePort (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Enabling port %d ('%s')\n", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortEnable,
      port, 0))){
    OMXCAM_setError ("%s: OMX_SendCommand: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_disablePort (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Disabling port %d ('%s')\n", port, component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SendCommand (component->handle, OMX_CommandPortDisable,
      port, 0))){
    OMXCAM_setError ("%s: OMX_SendCommand: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_allocateBuffer (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Allocating '%s' output buffer\n", component->name);
  
  OMX_ERRORTYPE error;
  
  OMX_PARAM_PORTDEFINITIONTYPE def;
  OMX_INIT_STRUCTURE (def);
  def.nPortIndex = port;
  if ((error = OMX_GetParameter (component->handle,
      OMX_IndexParamPortDefinition, &def))){
    OMXCAM_setError ("%s: OMX_GetParameter - OMX_IndexParamPortDefinition: %s",
        __func__, OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  if ((error = OMX_AllocateBuffer (component->handle,
      &OMXCAM_ctx.outputBuffer, port, 0, def.nBufferSize))){
    OMXCAM_setError ("%s: OMX_AllocateBuffer: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_freeBuffer (OMXCAM_COMPONENT* component, OMX_U32 port){
  OMXCAM_trace ("Releasing '%s' output buffer\n", component->name);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_FreeBuffer (component->handle, port,
      OMXCAM_ctx.outputBuffer))){
    OMXCAM_setError ("%s: OMX_FreeBuffer: %s", __func__,
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}