#include "rgb.h"

static uint32_t current = 0;
static uint32_t total;
static OMXCAM_ERROR bgError;

static void bufferCallback (uint8_t* buffer, uint32_t length){
  current += length;
  
  if (current >= total){
    bgError = OMXCAM_stopVideo ();
  }
}

OMXCAM_ERROR rgb (int frames, int width, int height){
  OMXCAM_VIDEO_SETTINGS settings;
  
  OMXCAM_initVideoSettings (&settings);
  settings.bufferCallback = bufferCallback;
  settings.format = OMXCAM_FormatRGB888;
  settings.camera.width = width;
  settings.camera.height = height;
  
  bgError = OMXCAM_ErrorNone;
  total = width*height*3*frames;
  
  OMXCAM_ERROR error = OMXCAM_startVideo (&settings, 0);
  
  return error ? error : bgError;
}