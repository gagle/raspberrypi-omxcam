#include "bench.h"

int jpeg (bench_t* req){
  omxcam_still_settings_t settings;
  
  omxcam_still_init (&settings);
  settings.on_ready = req->on_ready;
  settings.on_stop = req->on_stop;
  settings.camera.width = req->width;
  settings.camera.height = req->height;
  
  return omxcam_still_start (&settings);
}