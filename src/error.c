#include "omxcam.h"

static omxcam_errno last_error = OMXCAM_ERROR_NONE;

void omxcam_error_ (
    const char* fmt,
    const char* fn,
    const char* file,
    int line,
    ...){
  char buffer[256];
  va_list args;
  va_start (args, line);
  vsprintf (buffer, fmt, args);
  va_end (args);
  
  omxcam_trace ("error: %s (function: '%s', file: '%s', line %d)", buffer,
      fn, file, line);
}

#define OMXCAM_ERROR_NAME_GEN(_, name, __) case OMXCAM_ ## name: return #name;
const char* omxcam_error_name (omxcam_errno error){
  switch (error){
    OMXCAM_ERRNO_MAP (OMXCAM_ERROR_NAME_GEN)
    default:
      return 0;
  }
}
#undef OMXCAM_ERROR_NAME_GEN

#define OMXCAM_STRERROR_GEN(_, name, msg) case OMXCAM_ ## name: return msg;
const char* omxcam_strerror (omxcam_errno error){
  switch (error){
    OMXCAM_ERRNO_MAP (OMXCAM_STRERROR_GEN)
    default:
      return "unknown omxcam error";
  }
}
#undef OMXCAM_STRERROR_GEN

omxcam_errno omxcam_last_error (){
  return last_error;
}

void omxcam_set_last_error (omxcam_errno error){
  last_error = error;
}

void omxcam_perror (){
  fprintf (stderr, "omxcam %s: %s\n", omxcam_error_name (last_error),
      omxcam_strerror (last_error));
}