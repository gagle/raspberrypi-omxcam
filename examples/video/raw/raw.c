#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam/omxcam.h"

/*
This examples captures two videos (RGB and YUV) of 10 frames each one.
*/

int logError (OMXCAM_ERROR error){
  printf ("ERROR: %s (%s)\n", OMXCAM_errorName (error), OMXCAM_strError ());
  return 1;
}

int fd;

//We want to capture: 640x480, RGB (3 bytes per pixel), 10 frames
uint32_t rgbCurrent = 0;
uint32_t rgbTotal = 640*480*3*10;

//We want to capture: 640x480, YUV420, 10 frames
uint32_t yuvCurrent = 0;
uint32_t yuvFrames = 0;
uint32_t yuvFrame;
OMXCAM_YUV_PLANES yuvPlanes;
OMXCAM_YUV_PLANES yuvPlanesSlice;
uint8_t* yBuffer;
uint8_t* uBuffer;
uint8_t* vBuffer;

void bufferCallbackRGB (uint8_t* buffer, uint32_t length){
  int stop = 0;
  rgbCurrent += length;
  
  if (rgbCurrent >= rgbTotal){
    length -= rgbCurrent - rgbTotal;
    stop = 1;
  }
  
  OMXCAM_ERROR error;
  
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    printf ("ERROR: pwrite\n");
    if ((error = OMXCAM_stopVideo ())) logError (error);
    return;
  }
  
  if (stop){
    if ((error = OMXCAM_stopVideo ())) logError (error);
  }
}

void bufferCallbackYUV (uint8_t* buffer, uint32_t length){
  yuvCurrent += length;
  OMXCAM_ERROR error;
  
  //Append the data to the buffers
  memcpy (yBuffer, buffer + yuvPlanesSlice.yOffset, yuvPlanesSlice.yLength);
  yBuffer += yuvPlanesSlice.yLength;
  memcpy (uBuffer, buffer + yuvPlanesSlice.uOffset, yuvPlanesSlice.uLength);
  uBuffer += yuvPlanesSlice.uLength;
  memcpy (vBuffer, buffer + yuvPlanesSlice.vOffset, yuvPlanesSlice.vLength);
  vBuffer += yuvPlanesSlice.vLength;
  
  if (yuvCurrent == yuvFrame){
    //An entire YUV frame has been received
    yuvCurrent = 0;
    
    //Reset the pointers to their initial address
    yBuffer -= yuvPlanes.yLength;
    uBuffer -= yuvPlanes.uLength;
    vBuffer -= yuvPlanes.vLength;
    
    //Store the YUV planes
    if (pwrite (fd, yBuffer, yuvPlanes.yLength, 0) == -1){
      printf ("ERROR: pwrite\n");
      if ((error = OMXCAM_stopVideo ())) logError (error);
      return;
    }
    if (pwrite (fd, uBuffer, yuvPlanes.uLength, 0) == -1){
      printf ("ERROR: pwrite\n");
      if ((error = OMXCAM_stopVideo ())) logError (error);
      return;
    }
    if (pwrite (fd, vBuffer, yuvPlanes.vLength, 0) == -1){
      printf ("ERROR: pwrite\n");
      if ((error = OMXCAM_stopVideo ())) logError (error);
      return;
    }
    
    if (++yuvFrames == 10){
      //All the frames have been received
      if ((error = OMXCAM_stopVideo ())) logError (error);
    }
  }
}

int saveRGB (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
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
    printf ("ERROR: open\n");
    return 1;
  }
  
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_startVideo (settings, 0))) return logError (error);
  
  //Close the file
  if (close (fd)){
    printf ("ERROR: close\n");
    return 1;
  }
  
  return 0;
}

int saveYUV (char* filename, OMXCAM_VIDEO_SETTINGS* settings){
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
  
  In contrast to the RGB video, a buffer contains data of ONLY one frame, but
  you still need to control de storage/transmission of the frames because the
  video can be stopped at any time.
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    printf ("ERROR: open\n");
    return 1;
  }
  
  //The width and the height might be modified because they must be divisible by
  //16. In this case, both the width and the height are divisible by 16.
  yuvPlanes.width = yuvPlanesSlice.width = 640;
  yuvPlanes.height = yuvPlanesSlice.height = 480;
  OMXCAM_getYUVPlanes (&yuvPlanes);
  OMXCAM_getYUVPlanesSlice (&yuvPlanesSlice);
  
  //Frame size
  yuvFrame = yuvPlanes.vOffset + yuvPlanes.vLength;
  
  //Allocate the buffers
  yBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.yLength);
  uBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.uLength);
  vBuffer = (uint8_t*)malloc (sizeof (uint8_t)*yuvPlanes.vLength);
  
  OMXCAM_ERROR error;
  
  if ((error = OMXCAM_startVideo (settings, 0))) return logError (error);
  
  free (yBuffer);
  free (uBuffer);
  free (vBuffer);
  
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
  
  //Capture a raw RGB video
  OMXCAM_initVideoSettings (&settings);
  settings.bufferCallback = bufferCallbackRGB;
  settings.format = OMXCAM_FormatRGB888;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (saveRGB ("video.rgb", &settings)) return 1;
  
  //Capture a raw YUV420 video
  settings.bufferCallback = bufferCallbackYUV;
  settings.format = OMXCAM_FormatYUV420;
  
  if (saveYUV ("video.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}