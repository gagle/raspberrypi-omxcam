#include "rgb.h"

static uint32_t current = 0;
static uint32_t total;
static int bg_error = 0;

static void on_data_video (uint8_t* buffer, uint32_t length){
  current += length;
  
  if (current >= total){
    bg_error = omxcam_video_stop ();
  }
}

static void on_data_still (uint8_t* buffer, uint32_t length){
  //No-op
}

int rgb_video (int width, int height, int frames){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.on_data = on_data_video;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = width;
  settings.camera.height = height;
  
  total = width*height*3*frames;
  
  int error = omxcam_video_start (&settings, 0);
  
  return error ? error : bg_error;
}

int rgb_still (int width, int height){
  omxcam_still_settings_t settings;
  
  omxcam_still_init (&settings);
  settings.on_data = on_data_still;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = width;
  settings.camera.height = height;
  
  return omxcam_still_start (&settings);
}