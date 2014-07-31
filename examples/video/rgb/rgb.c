#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omxcam.h"

int fd;

uint32_t current;
uint32_t total;

int log_error (){
  omxcam_perror ();
  return 1;
}

void on_data (omxcam_buffer_t buffer){
  uint32_t length = buffer.length;
  int stop = 0;
  current += length;
  
  if (current >= total){
    length -= current - total;
    stop = 1;
  }
  
  //Append the buffer to the file
  if (pwrite (fd, buffer.data, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
    return;
  }
  
  if (stop){
    if (omxcam_video_stop ()) log_error ();
  }
}

int save (char* filename, omxcam_video_settings_t* settings){
  /*
  The RGB video comes in slices, that is, each buffer is part of a frame:
  buffer != frame -> buffer < frame. Take into account that a buffer can contain
  data from two consecutive frames because the frames are just concatenated one
  after the other. Therefore, you MUST control the storage/transmission of the
  frames because the video capture can be stopped at anytime, so it's likely
  that the last frame won't be received entirely, so the current received bytes
  MUST be discarded. For example:
  
  Note: Rf1 means channel Red of a pixel in the frame 1.
  
   ... Rf1 Gf1 Bf1 Rf2 Gf2 Bf2 ...
  |   |-----------|-----------|   |
  |   |last pixel |first pixel|   |
  |   |of frame 1 |of frame 2 |   |
  |-------------------------------|
  |            buffer             |
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  if (omxcam_video_start (settings, OMXCAM_CAPTURE_FOREVER)) log_error ();
  
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
  
  //1920x1080 @30fps by default
  
  settings.on_data = on_data;
  
  //RGB, 640x480 @30fps (10 frames)
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = 640;
  settings.camera.height = 480;
  settings.h264.inline_motion_vectors = OMXCAM_TRUE;
  
  current = 0;
  total = 640*480*3*10;
  
  if (save ("video-640x480.rgb", &settings)) return 1;
  
  //RGBA (alpha channel is unused, value 255), 642x480 @30fps (10 frames)
  //When no encoder is used, the width and the height need to be multiple of 32
  //and 16, therefore, 642 rounded up to the nearest multiple of 32 is 672
  settings.format = OMXCAM_FORMAT_RGBA8888;
  settings.camera.width = 642;
  settings.camera.height = 480;
  
  current = 0;
  total = 672*480*4*10;
  
  if (save ("video-672x480.rgba", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}