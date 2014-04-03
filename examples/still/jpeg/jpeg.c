#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam/omxcam.h"

void logError (char* fn, OMXCAM_ERROR error){
  printf ("ERROR: %s, %s ('%s')\n", fn, OMXCAM_errorToHuman (error),
      OMXCAM_lastError ());
}

int fd;

uint32_t bufferCallback (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    return -1;
  }
  
  return 0;
}

int save (char* filename, OMXCAM_STILL_SETTINGS* settings){
  printf ("capturing %s\n", filename);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return -1;
  }
  
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_still (settings))){
    logError ("OMXCAM_still", error);
    return -1;
  }
  
  //Close the file
  if (close (fd)){
    printf ("ERROR: close\n");
    return -1;
  }
  
  return 0;
}

int main (){
  OMXCAM_ERROR error;
  
  //Initialize the library
  printf ("initializing\n");
  if ((error = OMXCAM_init ())){
    logError ("OMXCAM_init", error);
    return 1;
  }
  
  OMXCAM_STILL_SETTINGS still;
  
  //Capture an image with the default settings
  OMXCAM_initStillSettings (&still);
  still.bufferCallback = bufferCallback;
  
  if (save ("still-default.jpg", &still)) return 1;
  
  //Capture an image with shutter speed 1/8, EV -10 and some EXIF tags
  OMXCAM_initStillSettings (&still);
  still.bufferCallback = bufferCallback;
  still.camera.shutterSpeedAuto = OMXCAM_FALSE;
  //Shutter speed in milliseconds
  still.camera.shutterSpeed = (uint32_t)((1.0/8.0)*1000);
  still.camera.exposureCompensation = -10;
  
  //See firmware/documentation/ilcomponents/image_decode.html for valid keys
  //See http://www.media.mit.edu/pia/Research/deepview/exif.html#IFD0Tags
  //for valid keys and their description
  OMXCAM_EXIF_TAG exifTags[] = {
    //Manufacturer
    { "IFD0.Make", "Raspberry Pi" }
  };
  still.jpeg.exifTags = exifTags;
  still.jpeg.exifValidTags = 1;
  
  if (save ("still.jpg", &still)) return 1;
  
  //Deinitialize the library
  printf ("deinitializing\n");
  if ((error = OMXCAM_deinit ())){
    logError ("OMXCAM_deinit", error);
    return 1;
  }
  
  printf ("ok\n");
  
  return 0;
}