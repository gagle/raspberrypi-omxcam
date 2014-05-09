#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam.h"

int logError (OMXCAM_ERROR error){
  printf ("ERROR: %s (%s)\n", OMXCAM_errorName (error), OMXCAM_strError ());
  return 1;
}

int fd;
uint32_t current = 0;

void bufferCallbackTime (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    OMXCAM_ERROR error;
    if ((error = OMXCAM_stopVideo ())) logError (error);
  }
}

void bufferCallbackLength (uint8_t* buffer, uint32_t length){
  OMXCAM_ERROR error;
  current += length;
  
  //Max file size 2MB
  if (current > 2097152){
    if ((error = OMXCAM_stopVideo ())) logError (error);
    return;
  }

  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    if ((error = OMXCAM_stopVideo ())) logError (error);
  }
}

int saveTime (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return 1;
  }
  
  OMXCAM_ERROR error;
  
  //Wait 3000ms
  if ((error = OMXCAM_startVideo (settings, 3000))) return logError (error);
  
  //Close the file
  if (close (fd)){
    printf ("ERROR: close\n");
    return 1;
  }
  
  return 0;
}

int saveLength (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return 1;
  }
  
  OMXCAM_ERROR error;
  
  //Wait indefinitely
  if ((error = OMXCAM_startVideo (settings, 0))) return logError (error);
  
  //Close the file
  if (close (fd)){
    printf ("ERROR: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  //1920x1080 30fps by default
  OMXCAM_VIDEO_SETTINGS settings;
  
  //Record a video of 3000ms
  OMXCAM_initVideoSettings (&settings);
  settings.bufferCallback = bufferCallbackTime;
  
  if (saveTime ("video-time.h264", &settings)) return 1;
  
  //Record a video of 2MB
  settings.bufferCallback = bufferCallbackLength;
  
  if (saveLength ("video-length.h264", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}