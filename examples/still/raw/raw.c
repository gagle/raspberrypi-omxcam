#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam.h"

int log_error (){
  omxcam_perror ();
  return 1;
}

int fd;

omxcam_yuv_planes_t yuv_planes;
omxcam_yuv_planes_t yuv_planes_slice;
uint8_t* buffer_y;
uint8_t* buffer_u;
uint8_t* buffer_v;

void buffer_callback_rgb (uint8_t* buffer, uint32_t length){
  //Append the buffer to the file
  //Note: Writing the data directly to disk will slow down the capture speed
  //due to the I/O access. A posible workaround is to save the buffers into
  //memory, similar to the YUV example, and then write the whole image to disk
  if (pwrite (fd, buffer, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_still_stop ()) log_error ();
  }
}

void buffer_callback_yuv (uint8_t* buffer, uint32_t length){
  //Append the data to the buffers
  memcpy (buffer_y, buffer + yuv_planes_slice.offset_y,
      yuv_planes_slice.length_y);
  buffer_y += yuv_planes_slice.length_y;
  
  memcpy (buffer_u, buffer + yuv_planes_slice.offset_u,
      yuv_planes_slice.length_u);
  buffer_u += yuv_planes_slice.length_u;
  
  memcpy (buffer_v, buffer + yuv_planes_slice.offset_v,
      yuv_planes_slice.length_v);
  buffer_v += yuv_planes_slice.length_v;
}

int save_rgb (char* filename, omxcam_still_settings_t* settings){
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

int save_yuv (char* filename, omxcam_still_settings_t* settings){
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
  
  omxcam_yuv_planes(): given a width and height, it calculates the offsets and
    lengths of each plane.
  omxcam_yuv_planes_slice(): given a width and height, it calculates the
    offsets and lengths of each plane from a slice returned by the
    "bufferCallback" function.
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  //The width and the height might be modified because they must be divisible by
  //16. For example, 1944 is not divisible by 16, it is incremented to 1952
  omxcam_yuv_planes (&yuv_planes, 2592, 1944);
  omxcam_yuv_planes_slice (&yuv_planes_slice, 2592);
  
  //Allocate the buffers
  buffer_y = (uint8_t*)malloc (sizeof (uint8_t)*yuv_planes.length_y);
  buffer_u = (uint8_t*)malloc (sizeof (uint8_t)*yuv_planes.length_u);
  buffer_v = (uint8_t*)malloc (sizeof (uint8_t)*yuv_planes.length_v);
  
  if (omxcam_still_start (settings)) log_error ();
  
  //Reset the pointers to their initial address
  buffer_y -= yuv_planes.length_y;
  buffer_u -= yuv_planes.length_u;
  buffer_v -= yuv_planes.length_v;
  
  //Store the YUV planes
  
  if (pwrite (fd, buffer_y, yuv_planes.length_y, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    return 1;
  }
  if (pwrite (fd, buffer_u, yuv_planes.length_u, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    return 1;
  }
  if (pwrite (fd, buffer_v, yuv_planes.length_v, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    return 1;
  }
  
  free (buffer_y);
  free (buffer_u);
  free (buffer_v);
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int main (){
  //2592x1944 by default
  omxcam_still_settings_t settings;
  
  //Capture a raw RGB image
  omxcam_still_init (&settings);
  settings.buffer_callback = buffer_callback_rgb;
  settings.camera.shutter_speed_auto = OMXCAM_FALSE;
  //Shutter speed in milliseconds (1/8 by default: 125)
  settings.camera.shutter_speed = (uint32_t)((1.0/8.0)*1000);
  settings.format = OMXCAM_FORMAT_RGB888;
  
  if (save_rgb ("still.rgb", &settings)) return 1;
  
  //Capture a raw YUV420 image
  settings.buffer_callback = buffer_callback_yuv;
  settings.format = OMXCAM_FORMAT_YUV420;
  
  if (save_yuv ("still.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}