#include <stdio.h>
#include <time.h>

#include "omxcam.h"
#include "rgb.h"
#include "yuv.h"

/*
Results: (the closer to 30fps and 1000ms, the better)

640x480, 30 frames
rgb: 21.83 fps (1374 ms)
yuv: 21.93 fps (1368 ms)
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
  
  int frames = 30;
  int width = 640;
  int height = 480;
  
  printf ("%dx%d, %d frames\n", width, height, frames);
  
  ms = now ();
  if (rgb (frames, width, height)) return log_error ();
  diff = now () - ms;
  printf ("rgb: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  ms = now ();
  if (yuv (frames, width, height)) return log_error ();
  diff = now () - ms;
  printf ("yuv: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  return 0;
}