#ifndef OMXCAM_ERRORS_H
#define OMXCAM_ERRORS_H

#include <stdarg.h>
#include <stdio.h>

#include "omxcam_omx.h"
#include "omxcam_debug.h"

#define OMXCAM_error(message,...) OMXCAM_setError(message,__func__,__FILE__,__LINE__,##__VA_ARGS__)

typedef enum {
  OMXCAM_ErrorNone,
  OMXCAM_ErrorInit,
  OMXCAM_ErrorInitCamera,
  OMXCAM_ErrorInitImageEncoder,
  OMXCAM_ErrorInitVideoEncoder,
  OMXCAM_ErrorInitNullSink,
  OMXCAM_ErrorDeinit,
  OMXCAM_ErrorDeinitCamera,
  OMXCAM_ErrorDeinitImageEncoder,
  OMXCAM_ErrorDeinitVideoEncoder,
  OMXCAM_ErrorDeinitNullSink,
  OMXCAM_ErrorCameraSettings,
  OMXCAM_ErrorStill,
  OMXCAM_ErrorVideo,
  OMXCAM_ErrorVideoCapture,
  OMXCAM_ErrorBadParameter,
  OMXCAM_ErrorSetupTunnel,
  OMXCAM_ErrorLoaded,
  OMXCAM_ErrorIdle,
  OMXCAM_ErrorExecuting,
  OMXCAM_ErrorFormat,
  OMXCAM_ErrorSleep,
  OMXCAM_ErrorWake,
  OMXCAM_ErrorLock,
  OMXCAM_ErrorUnlock
} OMXCAM_ERROR;

void OMXCAM_setError (
    const char* fmt,
    const char* fn,
    const char* file,
    int line,
    ...);
char* OMXCAM_lastError ();
const char* OMXCAM_errorToHuman (OMXCAM_ERROR error);
const char* OMXCAM_dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);

#endif