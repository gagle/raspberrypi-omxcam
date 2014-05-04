#include "omxcam/omxcam.h"

static OMXCAM_YUV_PLANES planes;
static uint32_t current = 0;
static uint32_t frames = 0;
static uint32_t frame;
static int totalFrames;
static OMXCAM_ERROR bgError;

static void bufferCallback (uint8_t* buffer, uint32_t length){
  current += length;
  
  if (current == frame){
    current = 0;
    
    if (++frames == totalFrames){
      bgError = OMXCAM_stopVideo ();
    }
  }
}

OMXCAM_ERROR yuv (int frames, int width, int height){
  OMXCAM_VIDEO_SETTINGS settings;
  
  OMXCAM_initVideoSettings (&settings);
  settings.bufferCallback = bufferCallback;
  settings.format = OMXCAM_FormatYUV420;
  settings.camera.width = width;
  settings.camera.height = height;
  
  planes.width = width;
  planes.height = height;
  OMXCAM_getYUVPlanes (&planes);
  
  frame = planes.vOffset + planes.vLength;
  totalFrames = frames;
  
  bgError = OMXCAM_ErrorNone;
  
  OMXCAM_ERROR error = OMXCAM_startVideo (&settings, 0);
  
  return error ? error : bgError;
}