#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam/omxcam.h"

int logError (OMXCAM_ERROR error){
  printf ("ERROR: %s (%s)\n", OMXCAM_errorToHuman (error), OMXCAM_lastError ());
  return 1;
}

int fd;

uint32_t bufferCallback (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    return 1;
  }
  
  return 0;
}

void errorCallback (OMXCAM_ERROR error){
  printf ("Error: %s\n", OMXCAM_errorToHuman (error));
}

int save (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return 1;
  }
  
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_startVideo (settings))) return logError (error);
  
  //Wait 3 seconds
  sleep (3);
  
  if ((error = OMXCAM_stopVideo ())) return logError (error);
  
  //Close the file
  if (close (fd)){
    printf ("ERROR: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  OMXCAM_ERROR error;
  
  //Initialize the library
  printf ("initializing\n");
  if ((error = OMXCAM_init ())) return logError (error);
  
  OMXCAM_VIDEO_SETTINGS video;
  
  //Record a video with the default settings
  OMXCAM_initVideoSettings (&video);
  video.bufferCallback = bufferCallback;
  video.errorCallback = errorCallback;
  
  if (save ("video-default.h264", &video)) return 1;
  
  //Deinitialize the library
  printf ("deinitializing\n");
  if ((error = OMXCAM_deinit ())) return logError (error);
  
  printf ("ok\n");
  
  return 0;
}