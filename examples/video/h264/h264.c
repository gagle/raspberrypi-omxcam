#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam.h"

int fd;
uint32_t current = 0;

int log_error (){
  omxcam_perror ();
  return 1;
}

void buffer_callback_time (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

void buffer_callback_length (uint8_t* buffer, uint32_t length){
  current += length;
  
  //Max file size 2MB
  if (current > 2097152){
    if (omxcam_video_stop ()) log_error ();
    return;
  }
  
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

int save_time (char* filename, omxcam_video_settings_t* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  //Capture ~2000ms (capture time, not file duration)
  if (omxcam_video_start (settings, 2000)) return log_error ();
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int save_length (char* filename, omxcam_video_settings_t* settings){
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
  //1920x1080 @30fps by default
  omxcam_video_settings_t settings;
  
  //Capture a video of ~2000ms (capture time, not file duration), 640x480 @90fps
  omxcam_video_init (&settings);
  settings.buffer_callback = buffer_callback_time;
  //http://picamera.readthedocs.org/en/release-1.4/fov.html#camera-modes
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (save_time ("video-time.h264", &settings)) return 1;
  
  //Capture a video of 2MB, 1920x1080 @30fps
  omxcam_video_init (&settings);
  settings.buffer_callback = buffer_callback_length;
  
  //if (save_length ("video-length.h264", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}