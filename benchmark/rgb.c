#include "bench.h"

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

int rgb_video (bench_t* req){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.on_ready = req->on_ready;
  settings.on_data = on_data_video;
  settings.on_stop = req->on_stop;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = req->width;
  settings.camera.height = req->height;
  
  total = req->width*req->height*3*req->frames;
  
  int error = omxcam_video_start (&settings, OMXCAM_CAPTURE_FOREVER);
  
  return error ? error : bg_error;
}

int rgb_still (bench_t* req){
  omxcam_still_settings_t settings;
  
  omxcam_still_init (&settings);
  settings.on_ready = req->on_ready;
  settings.on_data = on_data_still;
  settings.on_stop = req->on_stop;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = req->width;
  settings.camera.height = req->height;
  
  return omxcam_still_start (&settings);
}