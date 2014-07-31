#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam.h"

int fd;
int fd_motion;

int log_error (){
  omxcam_perror ();
  return 1;
}

void on_data (omxcam_buffer_t buffer){
  if (pwrite (fd, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

void on_motion (omxcam_buffer_t buffer){
  if (pwrite (fd_motion, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite (motion)\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

int save (char* filename, char* motion, omxcam_video_settings_t* settings){
  printf ("capturing %s\n", filename);
  printf ("capturing motion %s\n", motion);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  fd_motion = open (motion, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd_motion == -1){
    fprintf (stderr, "error: open (motion)\n");
    return 1;
  }
  
  if (omxcam_video_start (settings, 2000)) return log_error ();
  
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  if (close (fd_motion)){
    fprintf (stderr, "error: close (motion)\n");
    return 1;
  }
  
  return 0;
}

int main (){
  omxcam_video_settings_t settings;
  
  omxcam_video_init (&settings);
  settings.on_data = on_data;
  settings.on_motion = on_motion;
  settings.camera.width = 640;
  settings.camera.height = 480;
  settings.h264.inline_motion_vectors = OMXCAM_TRUE;
  
  if (save ("video.h264", "motion", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}