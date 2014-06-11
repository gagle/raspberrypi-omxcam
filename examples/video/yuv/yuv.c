#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omxcam.h"

int fd;

uint32_t current = 0;
uint32_t frames = 0;
uint32_t frame_size;
omxcam_yuv_planes_t planes;
omxcam_yuv_planes_t planes_slice;
uint32_t offset_y;
uint32_t offset_u;
uint32_t offset_v;
uint8_t* file_buffer;

int log_error (){
  omxcam_perror ();
  return 1;
}

void buffer_callback (uint8_t* buffer, uint32_t length){
  current += length;
  
  //Append the data to the buffer
  memcpy (file_buffer + offset_y, buffer + planes_slice.offset_y,
      planes_slice.length_y);
  offset_y += planes_slice.length_y;
  
  memcpy (file_buffer + offset_u, buffer + planes_slice.offset_u,
      planes_slice.length_u);
  offset_u += planes_slice.length_u;
  
  memcpy (file_buffer + offset_v, buffer + planes_slice.offset_v,
      planes_slice.length_v);
  offset_v += planes_slice.length_v;
  
  if (current == frame_size){
    //An entire YUV frame has been received
    current = 0;
    
    offset_y = planes.offset_y;
    offset_u = planes.offset_u;
    offset_v = planes.offset_v;
    
    if (pwrite (fd, file_buffer, frame_size, 0) == -1){
      fprintf (stderr, "error: pwrite\n");
      if (omxcam_video_stop ()) log_error ();
      return;
    }
    
    if (++frames == 10){
      //All the frames have been received
      if (omxcam_video_stop ()) log_error ();
    }
  }
}

int save (char* filename, omxcam_video_settings_t* settings){
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
  
  For this purpose you have the following functions:
  
  omxcam_yuv_planes(): Given the width and height of a frame, returns the offset
    and length of each of the yuv planes.
  omxcam_yuv_planes_slice(): Same as 'omxcam_yuv_planes()' but used with the
    payload buffers.
  
  In contrast to the RGB video, a YUV buffer contains data that belongs only to
  one frame, but you still need to control de storage/transmission of the frames
  because the video can be stopped at any time, so you need to make sure that
  the last whole frame is stored correctly.
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  omxcam_yuv_planes (settings->camera.width, settings->camera.height,
      &planes);
  omxcam_yuv_planes_slice (settings->camera.width, &planes_slice);
  
  //Frame size
  frame_size = planes.offset_v + planes.length_v;
  
  offset_y = planes.offset_y;
  offset_u = planes.offset_u;
  offset_v = planes.offset_v;
  
  //Allocate the buffer
  file_buffer = (uint8_t*)malloc (sizeof (uint8_t)*frame_size);
  
  if (omxcam_video_start (settings, 0)) log_error ();
  
  free (file_buffer);
  
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
  
  //YUV420, 640x480 @30fps (10 frames)
  settings.buffer_callback = buffer_callback;
  settings.format = OMXCAM_FORMAT_YUV420;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (save ("video-640x480.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}