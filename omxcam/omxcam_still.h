#ifndef OMXCAM_STILL_H
#define OMXCAM_STILL_H

#include "omxcam_omx.h"
#include "omxcam_errors.h"
#include "omxcam_core.h"
#include "omxcam_camera.h"
#include "omxcam_image_encode.h"

typedef struct {
  OMXCAM_CAMERA_SETTINGS camera;
  OMXCAM_FORMAT format;
  OMXCAM_JPEG_SETTINGS jpeg;
  uint32_t (*bufferCallback)(uint8_t* buffer, uint32_t length);
} OMXCAM_STILL_SETTINGS;

extern OMXCAM_CONTEXT ctx;

void OMXCAM_initStillSettings (OMXCAM_STILL_SETTINGS* settings);
OMXCAM_ERROR OMXCAM_still (OMXCAM_STILL_SETTINGS* settings);

#endif