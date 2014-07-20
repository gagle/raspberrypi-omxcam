#include "bench.h"

int h264 (bench_t* req){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.on_ready = req->on_ready;
  settings.on_stop = req->on_stop;
  settings.camera.width = req->width;
  settings.camera.height = req->height;
  
  return omxcam_video_start (&settings, req->ms);
}

int h264_npt (bench_t* req){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.camera.width = req->width;
  settings.camera.height = req->height;
  
  if (omxcam_video_start_npt (&settings)) return -1;
  
  req->on_ready ();
  req->on_stop ();
  
  return omxcam_video_stop_npt ();
}