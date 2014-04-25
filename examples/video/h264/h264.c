#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam/omxcam.h"

int logError (OMXCAM_ERROR error){
  printf ("ERROR: %s (%s)\n", OMXCAM_errorToHuman (error), OMXCAM_lastError ());
  return 1;
}

int fd;
uint32_t total = 0;

uint32_t bufferCallbackTime (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    return 1;
  }
  
  return 0;
}

void errorCallbackTime (OMXCAM_ERROR error){
  logError (error);
  
  //Cancel the sleep
  if ((error = OMXCAM_wake ())) logError (error);
}

uint32_t bufferCallbackLength (uint8_t* buffer, uint32_t length){
  total += length;
  
  //Max file size 2MB
  if (total > 2097152){
    OMXCAM_ERROR error;
    if ((error = OMXCAM_unlock ())) return logError (error);
    return 0;
  }

  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    return 1;
  }
  
  return 0;
}

void errorCallbackLength (OMXCAM_ERROR error){
  logError (error);
  
  //Unlock the main thread
  if ((error = OMXCAM_unlock ())) logError (error);
}

int saveTime (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return 1;
  }
  
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_startVideo (settings))) return logError (error);
  
  //Wait 3000ms
  if ((error = OMXCAM_sleep (3000))) return logError (error);
  
  if ((error = OMXCAM_stopVideo ())) return logError (error);
  
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
  
  if ((error = OMXCAM_startVideo (settings))) return logError (error);
  
  //Lock the main thread
  if ((error = OMXCAM_lock ())) return logError (error);
  
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
  
  //Record a video with the default settings (3000ms)
  OMXCAM_initVideoSettings (&video);
  video.bufferCallback = bufferCallbackTime;
  video.errorCallback = errorCallbackTime;
  
  if (saveTime ("video-default-time.h264", &video)) return 1;
  
  //Record a video  with the default settings (max file size of 2MB)
  video.bufferCallback = bufferCallbackLength;
  video.errorCallback = errorCallbackLength;
  
  if (saveLength ("video-default-length.h264", &video)) return 1;
  
  //Deinitialize the library
  printf ("deinitializing\n");
  if ((error = OMXCAM_deinit ())) return logError (error);
  
  printf ("ok\n");
  
  return 0;
}