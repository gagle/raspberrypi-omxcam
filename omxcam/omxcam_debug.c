#include "omxcam_debug.h"

void OMXCAM_trace (const char* fmt, ...){
#ifdef OMXCAM_DEBUG
  va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
#endif
}