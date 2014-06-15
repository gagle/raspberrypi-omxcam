#include "omxcam.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/*
Captures h264-encoded video at VGA resolution and 30fps indefinitely and writes
the stream to the stdout. Press ctrl-c to quit.

Usage:

$ ./h264-stdout > video.h264

Note: Remove the flag -DOMXCAM_DEBUG from the makefile.
*/

int log_error (){
  omxcam_perror ();
  return 1;
}

void buffer_callback (uint8_t* buffer, uint32_t length){
  //Write the buffers to the stdout
  if (write (STDOUT_FILENO, buffer, length) == -1){
    fprintf (stderr, "error: write\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

static void signal_handler (int signal){
  if (omxcam_video_stop ()) log_error ();
}

int main (){
  omxcam_video_settings_t settings;
  omxcam_video_init (&settings);
  
  settings.buffer_callback = buffer_callback;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  signal (SIGINT, signal_handler);
  signal (SIGTERM, signal_handler);
  signal (SIGQUIT, signal_handler);
  signal (SIGHUP, signal_handler);
  
  if (omxcam_video_start (&settings, OMXCAM_CAPTURE_FOREVER)){
    return log_error ();
  }
  
  return 0;
}