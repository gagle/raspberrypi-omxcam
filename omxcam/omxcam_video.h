#ifndef OMXCAM_VIDEO_H
#define OMXCAM_VIDEO_H

#include <pthread.h>

#include "omxcam_omx.h"
#include "omxcam_errors.h"
#include "omxcam_core.h"
#include "omxcam_camera.h"
#include "omxcam_video_encode.h"

typedef struct {
  OMXCAM_CAMERA_SETTINGS camera;
  OMXCAM_FORMAT format;
  OMXCAM_H264_SETTINGS h264;
  uint32_t (*bufferCallback)(uint8_t* buffer, uint32_t length);
  void (*errorCallback)(OMXCAM_ERROR error);
} OMXCAM_VIDEO_SETTINGS;

extern OMXCAM_CONTEXT ctx;

void OMXCAM_initVideoSettings (OMXCAM_VIDEO_SETTINGS* settings);
OMXCAM_ERROR OMXCAM_startVideo (OMXCAM_VIDEO_SETTINGS* settings);
OMXCAM_ERROR OMXCAM_stopVideo ();

#endif