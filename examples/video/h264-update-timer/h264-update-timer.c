#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#include "omxcam.h"

//If you need to iterate through all the values of a setting, you can use the
//available mapping macros.

//Edit the following macros to apply all the possible values of a setting.
#define MAP OMXCAM_IMAGE_FILTER_MAP
#define MAP_LENGTH OMXCAM_IMAGE_FILTER_MAP_LENGTH
#define FN omxcam_video_update_image_filter

#define VALUES(_, value) value,

int values[] = {
  MAP (VALUES)
};

int fd;
int current;
int interval = 1000;
int stop = 0;

int log_error (){
  omxcam_perror ();
  return 1;
}

void signal_handler (int signal){
  if (stop){
    struct itimerval timer;
    timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;
    setitimer (ITIMER_REAL, &timer, 0);
  
    if (omxcam_video_stop ()) log_error ();
    return;
  }
  if (omxcam_video_update_image_filter (values[current])) log_error ();
  if (++current == MAP_LENGTH) stop = 1;
}

void on_ready (){
  signal (SIGALRM, signal_handler);
  
  struct itimerval timer;
  
  timer.it_value.tv_sec = interval/1000;
  timer.it_value.tv_usec = (interval*1000)%1000000;
  timer.it_interval = timer.it_value;
  
  setitimer (ITIMER_REAL, &timer, 0);
}

void on_data (omxcam_buffer_t buffer){
  if (pwrite (fd, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
  }
}

int save (char* filename, omxcam_video_settings_t* settings){
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
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