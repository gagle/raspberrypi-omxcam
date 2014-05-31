#include "omxcam.h"

void omxcam_trace (const char* fmt, ...){
#ifdef OMXCAM_DEBUG
  char buffer[256];
  va_list args;
  va_start (args, fmt);
  vsprintf (buffer, fmt, args);
  va_end (args);
  
  printf ("omxcam: %s\n", buffer);
#endif
}