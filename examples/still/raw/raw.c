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
uint32_t offset_y;
uint32_t offset_u;
uint32_t offset_v;
uint8_t* file_buffer;

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
  memcpy (file_buffer + offset_y, buffer + yuv_planes_slice.offset_y,
      yuv_planes_slice.length_y);
  offset_y += yuv_planes_slice.length_y;
  
  memcpy (file_buffer + offset_u, buffer + yuv_planes_slice.offset_u,
      yuv_planes_slice.length_u);
  offset_u += yuv_planes_slice.length_u;
  
  memcpy (file_buffer + offset_v, buffer + yuv_planes_slice.offset_v,
      yuv_planes_slice.length_v);
  offset_v += yuv_planes_slice.length_v;
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
  The camera returns YUV420PackedPlanar buffers/slices.
  Packed means that each slice has a little portion of y + u + v planes.
  Planar means that each YUV component is located in a different plane/array,
  that is, it's not interleaved.
  PackedPlannar allows you to process each plane at the same time, that is,
  you don't need to wait to receive the entire Y plane to begin processing
  the U plane. This is good if you want to stream and manipulate the buffers,
  but when you need to store the data into a file, you need to store the entire
  planes one after the other, that is:
  
  WRONG: store the buffers as they come
    (y+u+v) + (y+u+v) + (y+u+v) + (y+u+v) + ...
    
  RIGHT: save the slices in different buffers and then store the entire planes
    (y+y+y+y+...) + (u+u+u+u+...) + (v+v+v+v+...)
  
  To ease the planes manipulation you have the following function:
  
  omxcam_yuv_planes(): given a width and height, it calculates the offsets and
    lengths of each plane.
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  omxcam_yuv_planes (&yuv_planes, settings->camera.width,
      settings->camera.height);
  omxcam_yuv_planes (&yuv_planes_slice, settings->camera.width,
      settings->slice_height);
  
  int yuv_frame_size = yuv_planes.offset_v + yuv_planes.length_v;
  offset_y = yuv_planes.offset_y;
  offset_u = yuv_planes.offset_u;
  offset_v = yuv_planes.offset_v;
  
  //Allocate the buffer
  file_buffer = (uint8_t*)malloc (sizeof (uint8_t)*yuv_frame_size);
  
  if (omxcam_still_start (settings)) log_error ();
  
  if (pwrite (fd, file_buffer, yuv_frame_size, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    return 1;
  }
  
  free (file_buffer);
  
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
  
  //Capture a raw RGB image (640x480)
  omxcam_still_init (&settings);
  settings.buffer_callback = buffer_callback_rgb;
  settings.camera.shutter_speed_auto = OMXCAM_FALSE;
  //Shutter speed in milliseconds (1/8 by default: 125)
  settings.camera.shutter_speed = (uint32_t)((1.0/8.0)*1000);
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  //if (save_rgb ("still.rgb", &settings)) return 1;
  
  /*
  Please note that the original aspect ratio from an image is 4:3. If you set
  dimensions with different ratios, the final image will still have the same
  aspect ratio but you will notice that it will be cropped to the given
  dimensions.
  
  For example:
  1296x730 (1312x736 rounded up) has a 16:9 aspect ratio, so the image that will
  be captured will have the dimensions 1296x972 (1296/(4/3) = 972)
  (1312x976 correctly rounded) after it is resized from 2592x1944, and then it
  will be cropped to 1312x736. Remember that the width must be multiple of 32
  (1296->1312) and the height of 16 (730->736).
  Just to compare, take two YUV images: 1296x730 and 1296x972. This is what will
  happen:
  
      --    ++++++++++++++++++++    --
  120 |     +                  +     |
      +-    +------------------+     |
      |     +                  +     |
  736 |     +                  +     | 976
      |     +                  +     |
      +-    +------------------+     |
  120 |     +                  +     |
      --    ++++++++++++++++++++    --
                    1312
  
  The inner image is what you get but the outer image is what it's captured by
  the camera.
  */
  
  //16:9
  settings.buffer_callback = buffer_callback_yuv;
  settings.format = OMXCAM_FORMAT_YUV420;
  settings.camera.width = 1296;
  settings.camera.height = 730;
  
  if (save_yuv ("still-1312x736.yuv", &settings)) return 1;
  
  //4:3
  settings.buffer_callback = buffer_callback_yuv;
  settings.format = OMXCAM_FORMAT_YUV420;
  settings.camera.width = 1296;
  settings.camera.height = 972;
  
  if (save_yuv ("still-1312x976.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}