#include "omxcam_errors.h"

static char lastError[256];

void OMXCAM_setError (const char* fmt, ...){
  va_list args;
  va_start (args, fmt);
  vsprintf (lastError, fmt, args);
  va_end (args);
  
  OMXCAM_trace ("ERROR: %s\n", lastError);
}

char* OMXCAM_lastError (){
  return lastError;
}

const char* OMXCAM_errorToHuman (OMXCAM_ERROR error){
  switch (error){
    case OMXCAM_ErrorNone:
      return "OMXCAM_ErrorNone";
    case OMXCAM_ErrorInit:
      return "OMXCAM_ErrorInit";
    case OMXCAM_ErrorInitCamera:
      return "OMXCAM_ErrorInitCamera";
    case OMXCAM_ErrorInitImageEncoder:
      return "OMXCAM_ErrorInitImageEncoder";
    case OMXCAM_ErrorInitVideoEncoder:
      return "OMXCAM_ErrorInitVideoEncoder";
    case OMXCAM_ErrorInitNullSink:
      return "OMXCAM_ErrorInitNullSink";
    case OMXCAM_ErrorDeinit:
      return "OMXCAM_ErrorDeinit";
    case OMXCAM_ErrorDeinitCamera:
      return "OMXCAM_ErrorDeinitCamera";
    case OMXCAM_ErrorDeinitImageEncoder:
      return "OMXCAM_ErrorDeinitImageEncoder";
    case OMXCAM_ErrorDeinitVideoEncoder:
      return "OMXCAM_ErrorDeinitVideoEncoder";
    case OMXCAM_ErrorDeinitNullSink:
      return "OMXCAM_ErrorDeinitNullSink";
    case OMXCAM_ErrorCameraSettings:
      return "OMXCAM_ErrorCameraSettings";
    case OMXCAM_ErrorStill:
      return "OMXCAM_ErrorStill";
    case OMXCAM_ErrorVideo:
      return "OMXCAM_ErrorVideo";
    case OMXCAM_ErrorVideoThread:
      return "OMXCAM_ErrorVideoThread";
    case OMXCAM_ErrorBadParameter:
      return "OMXCAM_ErrorBadParameter";
    case OMXCAM_ErrorSetupTunnel:
      return "OMXCAM_ErrorSetupTunnel";
    case OMXCAM_ErrorLoaded:
      return "OMXCAM_ErrorLoaded";
    case OMXCAM_ErrorIdle:
      return "OMXCAM_ErrorIdle";
    case OMXCAM_ErrorExecuting:
      return "OMXCAM_ErrorExecuting";
    case OMXCAM_ErrorFormat:
      return "OMXCAM_ErrorFormat";
    case OMXCAM_ErrorSleep:
      return "OMXCAM_ErrorSleep";
    case OMXCAM_ErrorWake:
      return "OMXCAM_ErrorWake";
    default:
      return "unknown";
  }
}