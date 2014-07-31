#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "omxcam.h"

int fd;
long start;
long change;
long stop;
int changed;

//Time spent writing to disk, this is basically to ensure a 2-second video
int delay = 110;

int log_error (){
  omxcam_perror ();
  return 1;
}

long now (){
  struct timespec spec;
  clock_gettime (CLOCK_MONOTONIC, &spec);
  return spec.tv_sec*1000 + spec.tv_nsec/1.0e6;
}

void on_ready (){
  //Record 1s, change settings and record 1s
  change = now () + 1000 + delay;
}

void on_data (omxcam_buffer_t buffer){
  int ms = now ();
  
  if (!changed){
    if (ms >= change){
      changed = 1;
      stop = ms + 1000 + delay;
      //If an error occurs, the camera is still running, so you need stop it
      //manually if you want to stop recording
      if (omxcam_video_update_saturation (100)) log_error ();
      if (omxcam_video_update_mirror (OMXCAM_MIRROR_HORIZONTAL)) log_error ();
    }
  }else{
    if (ms >= stop){
      if (omxcam_video_stop ()) log_error ();
      return;
    }
  }
  
  //Append the buffer to the file
  if (pwrite (fd, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

int save (char* filename, omxcam_video_settings_t* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  //Capture indefinitely
  if (omxcam_video_start (settings, OMXCAM_CAPTURE_FOREVER)){
    return log_error ();
  }
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  omxcam_video_settings_t settings;
  omxcam_video_init (&settings);
  
  settings.on_ready = on_ready;
  settings.on_data = on_data;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (save ("video.h264", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}