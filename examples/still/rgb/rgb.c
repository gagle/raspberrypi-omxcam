#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omxcam.h"

int log_error (){
  omxcam_perror ();
  return 1;
}

int fd;

void on_data (omxcam_buffer_t buffer){
  //Append the buffer to the file
  //Note: Writing the data directly to disk will slow down the capture speed
  //due to the I/O access. A posible workaround is to save the buffers into
  //memory, similar to the YUV example, and then write the whole image to disk
  if (pwrite (fd, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_still_stop ()) log_error ();
  }
}

int save (char* filename, omxcam_still_settings_t* settings){
  printf ("capturing %s\n", filename);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  if (omxcam_still_start (settings)) log_error ();
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  omxcam_still_settings_t settings;
  omxcam_still_init (&settings);
  
  //2592x1944 by default
  
  settings.on_data = on_data;
  //Shutter speed in microseconds, (1/8)*1e6
  settings.camera.shutter_speed = 125000;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  //RGB, 640x480
  settings.format = OMXCAM_FORMAT_RGB888;
  
  if (save ("still-640x480.rgb", &settings)) return 1;
  
  //RGBA (alpha channel is unused, value 255), 640x480
  settings.format = OMXCAM_FORMAT_RGBA8888;
  
  if (save ("still-640x480.rgba", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}