#include "omxcam.h"

#define OMXCAM_STR_(x) #x
#define OMXCAM_STR(x) OMXCAM_STR_(x)

#define OMXCAM_VERSION                                                         \
  ((OMXCAM_VERSION_MAJOR << 16) |                                              \
  (OMXCAM_VERSION_MINOR << 8) |                                                \
  (OMXCAM_VERSION_PATCH))

#define OMXCAM_VERSION_STRING                                                  \
  OMXCAM_STR(OMXCAM_VERSION_MAJOR) "."                                         \
  OMXCAM_STR(OMXCAM_VERSION_MINOR) "."                                         \
  OMXCAM_STR(OMXCAM_VERSION_PATCH)

uint32_t omxcam_version (){
  return OMXCAM_VERSION;
}

const char* omxcam_version_string (){
  return OMXCAM_VERSION_STRING;
}

#undef OMXCAM_STR
#undef OMXCAM_STR_