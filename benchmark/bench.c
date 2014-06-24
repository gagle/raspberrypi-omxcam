#include <stdio.h>
#include <time.h>

#include "omxcam.h"
#include "rgb.h"
#include "yuv.h"

#define TIME(label, code)                                                      \
  ms = now ();                                                                 \
  if (code) return log_error ();                                               \
  diff = now () - ms;

#define TIME_VIDEO(label, code)                                                \
  TIME (label, code)                                                           \
  printf (#label ": %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);

#define TIME_STILL(label, code)                                                \
  TIME (label, code)                                                           \
  printf (#label ": %.2f fps (%d ms)\n", 1000.0/diff, diff);

/*
Results:

In video mode, the closer to 30fps and 1000ms, the better.
In still mode, the faster, the better.

video rgb: 21.80 fps (1376 ms)
video yuv: 21.93 fps (1368 ms)
still rgb: 0.96 fps (1039 ms)
still yuv: 0.96 fps (1042 ms)
*/

long now (){
  struct timespec spec;
  clock_gettime (CLOCK_MONOTONIC, &spec);
  return spec.tv_sec*1000 + spec.tv_nsec/1.0e6;
}

int log_error (){
  omxcam_perror ();
  return 1;
}

int main (){
  long ms;
  int diff;
  
  int width = 640;
  int height = 480;
  int frames = 30;
  
  TIME_VIDEO (video rgb, rgb_video (width, height, frames));
  TIME_VIDEO (video yuv, yuv_video (width, height, frames));
  TIME_STILL (still rgb, rgb_still (width, height));
  TIME_STILL (still yuv, yuv_still (width, height));
  
  return 0;
}