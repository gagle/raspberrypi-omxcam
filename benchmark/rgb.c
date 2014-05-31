#include "rgb.h"

static uint32_t current = 0;
static uint32_t total;
static int bg_error = 0;

static void buffer_callback (uint8_t* buffer, uint32_t length){
  current += length;
  
  if (current >= total){
    bg_error = omxcam_video_stop ();
  }
}

int rgb (int frames, int width, int height){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.buffer_callback = buffer_callback;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = width;
  settings.camera.height = height;
  
  total = width*height*3*frames;
  
  int error = omxcam_video_start (&settings, 0);
  
  return error ? error : bg_error;
}