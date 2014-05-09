#ifndef OMXCAM_CORE_H
#define OMXCAM_CORE_H

#include <string.h>

#include "omxcam_omx.h"
#include "omxcam_dump.h"
#include "omxcam_errors.h"
#include "omxcam_debug.h"
#include "omxcam_utils.h"

typedef enum {
  OMXCAM_FALSE,
  OMXCAM_TRUE,
} OMXCAM_BOOL;

typedef enum {
  OMXCAM_EventError = 0x1,
  OMXCAM_EventPortEnable = 0x2,
  OMXCAM_EventPortDisable = 0x4,
  OMXCAM_EventStateSet = 0x8,
  OMXCAM_EventFlush = 0x10,
  OMXCAM_EventMarkBuffer = 0x20,
  OMXCAM_EventMark = 0x40,
  OMXCAM_EventPortSettingsChanged = 0x80,
  OMXCAM_EventParamOrConfigChanged = 0x100,
  OMXCAM_EventBufferFlag = 0x200,
  OMXCAM_EventResourcesAcquired = 0x400,
  OMXCAM_EventDynamicResourcesAvailable = 0x800,
  OMXCAM_EventFillBufferDone = 0x1000
} OMXCAM_EVENT;

typedef enum {
  OMXCAM_FormatRGB888,
  OMXCAM_FormatRGBA8888,
  OMXCAM_FormatYUV420,
  OMXCAM_FormatJPEG,
  OMXCAM_FormatH264
} OMXCAM_FORMAT;

//Wrapper of an OMX component with the necessary data
typedef struct {
  OMX_HANDLETYPE handle;
  VCOS_EVENT_FLAGS_T flags;
  OMX_STRING name;
  void (*bufferCallback)(uint8_t* buffer, uint32_t length);
} OMXCAM_COMPONENT;

//Application's context
typedef struct {
  OMXCAM_COMPONENT camera;
  OMXCAM_COMPONENT image_encode;
  OMXCAM_COMPONENT video_encode;
  OMXCAM_COMPONENT null_sink;
  OMX_BUFFERHEADERTYPE* outputBuffer;
} OMXCAM_CONTEXT;

OMXCAM_CONTEXT OMXCAM_ctx;

OMX_ERRORTYPE EventHandler (
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1,
    OMX_IN OMX_U32 nData2,
    OMX_IN OMX_PTR pEventData);
OMX_ERRORTYPE FillBufferDone (
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

void OMXCAM_continue (OMXCAM_COMPONENT* component, VCOS_UNSIGNED event);
int OMXCAM_wait (
    OMXCAM_COMPONENT* component,
    VCOS_UNSIGNED events,
    VCOS_UNSIGNED* retrievedEvents);
int OMXCAM_initComponent (OMXCAM_COMPONENT* component);
int OMXCAM_deinitComponent (OMXCAM_COMPONENT* component);
int OMXCAM_changeState (OMXCAM_COMPONENT* component, OMX_STATETYPE state);
int OMXCAM_enablePort (OMXCAM_COMPONENT* component, OMX_U32 port);
int OMXCAM_disablePort (OMXCAM_COMPONENT* component, OMX_U32 port);
int OMXCAM_allocateBuffer (OMXCAM_COMPONENT* component, OMX_U32 port);
int OMXCAM_freeBuffer (OMXCAM_COMPONENT* component, OMX_U32 port);
OMXCAM_ERROR OMXCAM_init ();
OMXCAM_ERROR OMXCAM_deinit ();

#endif