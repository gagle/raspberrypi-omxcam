#include "omxcam.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/*
Captures h264-encoded video at 320x240 @10fps indefinitely and writes the stream
to the stdout. Press ctrl-c to quit. The low resolution and framerate are ideal
to stream the content over the internet, the required KB/s are pretty low. It
depends on the image being captured but around 20KB/s is a good aproximation.
Make your tests.

Usage:

$ ./h264-stdout > video.h264

Note: Remove the flag -DOMXCAM_DEBUG from the makefile.
*/

int log_error (){
  omxcam_perror ();
  return 1;
}

void on_data (uint8_t* buffer, uint32_t length){
  //Write the buffers to the stdout
  if (write (STDOUT_FILENO, buffer, length) == -1){
    fprintf (stderr, "error: write\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

void signal_handler (int signal){
  if (omxcam_video_stop ()) log_error ();
}

int main (){
  omxcam_video_settings_t settings;
  omxcam_video_init (&settings);
  
  settings.on_data = on_data;
  settings.camera.width = 640;
  settings.camera.height = 480;
  //settings.camera.width = 320;
  //settings.camera.height = 240;
  //settings.camera.framerate = 10;
  
  signal (SIGINT, signal_handler);
  signal (SIGTERM, signal_handler);
  signal (SIGQUIT, signal_handler);
  signal (SIGHUP, signal_handler);
  
  if (omxcam_video_start (&settings, OMXCAM_CAPTURE_FOREVER)){
    return log_error ();
  }
  
  return 0;
}