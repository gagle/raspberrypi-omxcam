#include "omxcam.h"

uint32_t omxcam_round (uint32_t value, uint32_t divisor){
  return (divisor + value - 1) & ~(divisor - 1);
}

void omxcam_yuv_planes (
    omxcam_yuv_planes_t* planes,
    uint32_t width,
    uint32_t height){
  //The width and height must be divisible by 16
  width = omxcam_round (width, 16);
  height = omxcam_round (height, 16);
  
  planes->offset_y = 0;
  planes->length_y = width*height;
  planes->offset_u = planes->length_y;
  //(width/2)*(height/2)
  planes->length_u = (width >> 1)*(height >> 1);
  planes->offset_v = planes->length_y + planes->length_u;
  planes->length_v = planes->length_u;
}