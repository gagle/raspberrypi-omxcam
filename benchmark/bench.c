#include <stdio.h>
#include <time.h>

#include "omxcam/omxcam.h"
#include "rgb.h"
#include "yuv.h"

/*
Results:

frames: 30
width: 640
height: 480
rgb: 20.63 fps (1454 ms)
yuv: 21.83 fps (1374 ms)
*/

long now (){
  struct timespec spec;
  clock_gettime (CLOCK_MONOTONIC, &spec);
  return spec.tv_sec*1000 + spec.tv_nsec/1.0e6;
}

int logError (OMXCAM_ERROR error){
  printf ("ERROR: %s (%s)\n", OMXCAM_errorToHuman (error), OMXCAM_lastError ());
  return 1;
}

int main (){
  OMXCAM_ERROR error;
  long ms;
  int diff;
  
  int frames = 30;
  int width = 640;
  int height = 480;
  
  printf ("frames: %d\n", frames);
  printf ("width: %d\n", width);
  printf ("height: %d\n", height);
  
  ms = now ();
  if ((error = rgb (frames, width, height))) return logError (error);
  diff = now () - ms;
  printf ("rgb: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  ms = now ();
  if ((error = yuv (frames, width, height))) return logError (error);
  diff = now () - ms;
  printf ("yuv: %.2f fps (%d ms)\n", frames/(diff/1000.0), diff);
  
  return 0;
}