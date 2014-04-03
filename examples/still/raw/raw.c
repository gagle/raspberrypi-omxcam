#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam/omxcam.h"

void logError (char* fn, OMXCAM_ERROR error){
  printf ("ERROR: %s, %s ('%s')\n", fn, OMXCAM_errorToHuman (error),
      OMXCAM_lastError ());
}

int fd;
OMXCAM_YUV_PLANES yuvPlanes;
OMXCAM_YUV_PLANES yuvPlanesSlice;
uint8_t* yBuffer;
uint8_t* uBuffer;
uint8_t* vBuffer;

uint32_t bufferCallbackRGB (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    return -1;
  }
  
  return 0;
}

uint32_t bufferCallbackYUV (uint8_t* buffer, uint32_t length){
  //Append the data to the yuv buffers
  
  memcpy (yBuffer, buffer + yuvPlanesSlice.yOffset, yuvPlanesSlice.yLength);
  yBuffer += yuvPlanesSlice.yLength;
  memcpy (uBuffer, buffer + yuvPlanesSlice.uOffset, yuvPlanesSlice.uLength);
  uBuffer += yuvPlanesSlice.uLength;
  memcpy (vBuffer, buffer + yuvPlanesSlice.vOffset, yuvPlanesSlice.vLength);
  vBuffer += yuvPlanesSlice.vLength;
  
  return 0;
}

int saveRGB (char* filename, OMXCAM_STILL_SETTINGS* settings){
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

int saveYUV (char* filename, OMXCAM_STILL_SETTINGS* settings){
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
  
  //Fix the pointers to their initial address
  yBuffer -= yuvPlanes.yLength;
  uBuffer -= yuvPlanes.uLength;
  vBuffer -= yuvPlanes.vLength;
  
  //Store the YUV planes
  
  if (pwrite (fd, yBuffer, yuvPlanes.yLength, 0) == -1){
    printf ("ERROR: pwrite\n");
    return -1;
  }
  if (pwrite (fd, uBuffer, yuvPlanes.uLength, 0) == -1){
    printf ("ERROR: pwrite\n");
    return -1;
  }
  if (pwrite (fd, vBuffer, yuvPlanes.vLength, 0) == -1){
    printf ("ERROR: pwrite\n");
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
  
  //Capture a raw RGB image
  OMXCAM_initStillSettings (&still);
  still.bufferCallback = bufferCallbackRGB;
  still.camera.shutterSpeedAuto = OMXCAM_FALSE;
  still.format = OMXCAM_FormatRGB888;
  
  if (saveRGB ("still.rgb", &still)) return 1;
  
  //Capture a raw YUV420 image
  OMXCAM_initStillSettings (&still);
  still.bufferCallback = bufferCallbackYUV;
  still.camera.shutterSpeedAuto = OMXCAM_FALSE;
  still.format = OMXCAM_FormatYUV420;
  
  /*
  The camera returns YUV420PackedPlanar buffers/slices
  Packed means that each slice has y + u + v planes
  Planar means that each YUV component is located in a different plane/array
  PackedPlannar allows you to process each plane at the same time, that is,
  you don't need to wait to receive the entire Y plane to begin processing
  the U plane. This is good if you want to stream the buffers, but when you
  need to store the data into a file, you need to store the entire planes
  one after the other, that is:
  
  WRONG: store the buffers as they come
    (y+u+v) + (y+u+v) + (y+u+v) + (y+u+v) + ...
    
  RIGHT: save the slices in different buffers and then store the entire planes
    (y+y+y+y+...) + (u+u+u+u+...) + (v+v+v+v+...)
  
  Therefore, you need to buffer the entire planes if you want to store them into
  a file. For this purpose you have two functions:
  
  OMXCAM_getYUVPlanes(): given a width and height, it calculates the offsets and
    lengths of each plane.
  OMXCAM_getYUVPlanesSlice(): given a width and height, it calculates the
    offsets and lengths of each plane from a slice returned by the
    "bufferCallback" function.
  */
  
  //The width and the height can be modified because they must be divisible by
  //16. For example, 1944 is not divisible by 16, it is incremented to 1952
  //2592x1944 are the default values
  yuvPlanes.width = yuvPlanesSlice.width = 2592;
  yuvPlanes.height = yuvPlanesSlice.height = 1944;
  OMXCAM_getYUVPlanes (&yuvPlanes);
  OMXCAM_getYUVPlanesSlice (&yuvPlanesSlice);
  
  //Allocate the buffers
  yBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.yLength);
  uBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.uLength);
  vBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.vLength);
  
  if (saveYUV ("still.yuv", &still)) return 1;
  
  free (yBuffer);
  free (uBuffer);
  free (vBuffer);
  
  //Deinitialize the library
  printf ("deinitializing\n");
  if ((error = OMXCAM_deinit ())){
    logError ("OMXCAM_deinit", error);
    return 1;
  }
  
  printf ("ok\n");
  
  return 0;
}