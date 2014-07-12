#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include "omxcam.h"

int fd;
uint32_t current = 0;
int timeout = 2000;
int quit = 0;

int log_error (){
  omxcam_perror ();
  return 1;
}

void signal_handler (int signal){
  quit = 1;
}

void on_ready (){
  signal (SIGALRM, signal_handler);
  
  struct itimerval timer;
  
  timer.it_value.tv_sec = timeout/1000;
  timer.it_value.tv_usec = (timeout*1000)%1000000;
  timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
  
  setitimer (ITIMER_REAL, &timer, 0);
}

void on_data (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop_async ()) log_error ();
  }
}

int save (char* filename, omxcam_video_settings_t* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  if (omxcam_video_start_async (settings)) return log_error ();
  
  while (!quit){
    //When read() is called, the current thread is lockd until 'on_data' is
    //executed or an error occurs
    if (omxcam_video_read_async ()) return log_error ();
  }
  
  if (omxcam_video_stop_async ()) return log_error ();
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  omxcam_video_settings_t settings;
  
  //Capture a video of ~2000ms, 640x480 @30fps
  omxcam_video_init (&settings);
  settings.on_ready = on_ready;
  settings.on_data = on_data;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (save ("video.h264", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}