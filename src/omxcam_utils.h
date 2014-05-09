#ifndef OMXCAM_UTILS_H
#define OMXCAM_UTILS_H

#include <stdint.h>

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t yOffset;
  uint32_t yLength;
  uint32_t uOffset;
  uint32_t uLength;
  uint32_t vOffset;
  uint32_t vLength;
} OMXCAM_YUV_PLANES;

uint32_t OMXCAM_round (uint32_t divisor, uint32_t value);
void OMXCAM_getYUVPlanes (OMXCAM_YUV_PLANES* planes);
void OMXCAM_getYUVPlanesSlice (OMXCAM_YUV_PLANES* planes);

#endif