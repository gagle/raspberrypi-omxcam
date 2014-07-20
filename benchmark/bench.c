#include "bench.h"

/*
Default settings are used. This is not a "real" benchmark since the capture time
depends on various settings like the shutter speed and scene illuminance. This
is just a little benchmark to confirm that the library is working as expected.

Note: Remove the flag -DOMXCAM_DEBUG from the makefile.

Results:

In video mode, the closer to 30fps and 1000ms, the better.
In still mode, the faster, the better, being 1fps the minimum desirable.

set up h264: 323 ms
tear down h264: 58 ms
set up h264 (npt): 319 ms
tear down h264 (npt): 39 ms
video rgb: 28.17 fps (1065 ms)
video yuv: 28.17 fps (1065 ms)
video yuv (npt): 27.86 fps (1077 ms)
*/

uint32_t start;
uint32_t end;
uint32_t diff1;
uint32_t diff2;

static uint32_t now (){
  struct timespec t;
  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec*1000 + t.tv_nsec/1.0e6;
}

static void print_time_video (char* label, uint32_t frames){
  printf ("%s: %.2f fps (%d ms)\n", label, frames/(diff1/1000.0), diff1);
}

/*static void print_time_still (char* label){
  printf ("%s: %.2f fps (%d ms)\n", label, 1000.0/diff1, diff1);
}*/

static int log_error (){
  omxcam_perror ();
  return 1;
}

static void on_ready (){
  start = now ();
}

static void on_stop (){
  diff1 = now () - start;
}

static void on_ready_set_up (){
  diff1 = now () - start;
}

static void on_stop_tear_down (){
  start = now ();
}

int main (){
  bench_t req;
  req.width = 640;
  req.height = 480;
  req.frames = 30;
  req.ms = 1000;
  
  req.on_ready = on_ready_set_up;
  req.on_stop = on_stop_tear_down;
  
  start = now ();
  if (h264 (&req)) return log_error ();
  diff2 = now () - start;
  printf ("set up h264: %d ms\n", diff1);
  printf ("tear down h264: %d ms\n", diff2);
  
  start = now ();
  if (h264_npt (&req)) return log_error ();
  diff2 = now () - start;
  printf ("set up h264 (npt): %d ms\n", diff1);
  printf ("tear down h264 (npt): %d ms\n", diff2);
  
  /*start = now ();
  if (jpeg (&req)) return log_error ();
  diff2 = now () - start;
  printf ("set up jpeg: %d ms\n", diff1);
  printf ("tear down jpeg: %d ms\n", diff2);*/
  
  req.on_ready = on_ready;
  req.on_stop = on_stop;
  
  if (rgb_video (&req)) return log_error ();
  print_time_video ("video rgb", req.frames);
  
  if (yuv_video (&req)) return log_error ();
  print_time_video ("video yuv", req.frames);
  
  if (yuv_video_npt (&req)) return log_error ();
  print_time_video ("video yuv (npt)", req.frames);
  
  /*if (rgb_still (&req)) return log_error ();
  print_time_still ("still rgb");
  
  if (yuv_still (&req)) return log_error ();
  print_time_still ("still yuv");*/
  
  return 0;
}