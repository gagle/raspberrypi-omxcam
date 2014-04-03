#include "omxcam_utils.h"

uint32_t OMXCAM_round (uint32_t divisor, uint32_t value){
  return (value + divisor - 1) & ~(divisor - 1);
}

void OMXCAM_getYUVPlanes (OMXCAM_YUV_PLANES* planes){
  //Add padding, width and height must be divisible by 16
  uint32_t width = OMXCAM_round (16, planes->width);
  uint32_t height = OMXCAM_round (16, planes->height);
  
  planes->yOffset = 0;
  planes->yLength = width*height;
  planes->uOffset = planes->yLength;
  //(width/2)*(height/2)
  planes->uLength = (width >> 1)*(height >> 1);
  planes->vOffset = planes->yLength + planes->uLength;
  planes->vLength = planes->uLength;
}

void OMXCAM_getYUVPlanesSlice (OMXCAM_YUV_PLANES* planes){
  //Add padding, width and height must be divisible by 16
  uint32_t width = OMXCAM_round (16, planes->width);
  
  //slice height = 16 for stills
  //TODO: see if video slices are also 16
  planes->yOffset = 0;
  planes->yLength = width << 4;
  planes->uOffset = planes->yLength;
  //(width/2)*(sliceHeight/2)
  planes->uLength = width << 2;
  planes->vOffset = planes->yLength + planes->uLength;
  planes->vLength = planes->uLength;
}