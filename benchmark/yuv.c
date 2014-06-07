#include "yuv.h"

static omxcam_yuv_planes_t planes;
static uint32_t current = 0;
static uint32_t frames = 0;
static uint32_t frame;
static int total_frames;
static int bg_error = 0;

static void buffer_callback (uint8_t* buffer, uint32_t length){
  current += length;
  
  if (current == frame){
    current = 0;
    
    if (++frames == total_frames){
      bg_error = omxcam_video_stop ();
    }
  }
}

int yuv (int frames, int width, int height){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.buffer_callback = buffer_callback;
  settings.format = OMXCAM_FORMAT_YUV420;
  settings.camera.width = width;
  settings.camera.height = height;
  
  omxcam_yuv_planes (width, height, &planes);
  
  frame = planes.offset_v + planes.length_v;
  total_frames = frames;
  
  int error = omxcam_video_start (&settings, 0);
  
  return error ? error : bg_error;
}