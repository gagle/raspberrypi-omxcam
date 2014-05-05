#ifndef OMXCAM_IMAGE_ENCODE_H
#define OMXCAM_IMAGE_ENCODE_H

#include <time.h>

#include "omxcam_core.h"

typedef struct {
  char* key;
  char* value;
} OMXCAM_EXIF_TAG;

typedef struct {
  uint32_t quality;
  OMXCAM_BOOL exifEnable;
  OMXCAM_EXIF_TAG* exifTags;
  uint32_t exifValidTags;
  OMXCAM_BOOL ijg;
  OMXCAM_BOOL thumbnailEnable;
  uint32_t thumbnailWidth;
  uint32_t thumbnailHeight;
  OMXCAM_BOOL rawBayer;
} OMXCAM_JPEG_SETTINGS;

void OMXCAM_initJpegSettings (OMXCAM_JPEG_SETTINGS* settings);
int OMXCAM_addTag (char* key, char* value);
int OMXCAM_setJpegSettings (OMXCAM_JPEG_SETTINGS* settings);

#endif