#ifndef OMXCAM_VIDEO_ENCODE_H
#define OMXCAM_VIDEO_ENCODE_H

#include "omxcam_omx.h"
#include "omxcam_errors.h"
#include "omxcam_core.h"

typedef struct {
  uint32_t bitrate;
  uint32_t idrPeriod;
} OMXCAM_H264_SETTINGS;

void OMXCAM_initH264Settings (OMXCAM_H264_SETTINGS* settings);
int OMXCAM_setH264Settings (OMXCAM_H264_SETTINGS* settings);

#endif