#include <stdio.h>
#include <time.h>

#include "omxcam.h"
#include "rgb.h"
#include "yuv.h"

/*
Results:

In video mode, the closer to 30fps and 1000ms, the better.
In still mode, the faster, the better.

video rgb: 21.80 fps (1376 ms)
video yuv: 21.91 fps (1369 ms)
still yuv: 1.24 fps (805 ms)
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
  
  ms = now ();
  if (rgb_video (width, height, frames)) return log_error ();
  diff = now () - ms;
  printf ("video rgb: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  ms = now ();
  if (yuv_video (width, height, frames)) return log_error ();
  diff = now () - ms;
  printf ("video yuv: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  /*ms = now ();
  if (rgb_still (width, height)) return log_error ();
  diff = now () - ms;
  printf ("still rgb: %.2f fps (%d ms)\n", 1000.0/diff, diff);*/
  
  ms = now ();
  if (yuv_still (width, height)) return log_error ();
  diff = now () - ms;
  printf ("still yuv: %.2f fps (%d ms)\n", 1000.0/diff, diff);
  
  return 0;
}