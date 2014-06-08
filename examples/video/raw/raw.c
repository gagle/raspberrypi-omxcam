#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "omxcam.h"

int fd;

uint32_t rgb_current = 0;
uint32_t rgb_total = 640*480*3*10;

uint32_t yuv_current = 0;
uint32_t yuv_frames = 0;
uint32_t yuv_frame_size;
omxcam_yuv_planes_t yuv_planes;
omxcam_yuv_planes_t yuv_planes_slice;
uint32_t offset_y;
uint32_t offset_u;
uint32_t offset_v;
uint8_t* file_buffer;

int log_error (){
  omxcam_perror ();
  return 1;
}

void buffer_callback_rgb (uint8_t* buffer, uint32_t length){
  int stop = 0;
  rgb_current += length;
  
  if (rgb_current >= rgb_total){
    length -= rgb_current - rgb_total;
    stop = 1;
  }
  
  //Append the buffer to the file
  if (pwrite (fd, buffer, length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_video_stop ()) log_error ();
    return;
  }
  
  if (stop){
    if (omxcam_video_stop ()) log_error ();
  }
}

void buffer_callback_yuv (uint8_t* buffer, uint32_t length){
  yuv_current += length;
  
  //Append the data to the buffer
  memcpy (file_buffer + offset_y, buffer + yuv_planes_slice.offset_y,
      yuv_planes_slice.length_y);
  offset_y += yuv_planes_slice.length_y;
  
  memcpy (file_buffer + offset_u, buffer + yuv_planes_slice.offset_u,
      yuv_planes_slice.length_u);
  offset_u += yuv_planes_slice.length_u;
  
  memcpy (file_buffer + offset_v, buffer + yuv_planes_slice.offset_v,
      yuv_planes_slice.length_v);
  offset_v += yuv_planes_slice.length_v;
  
  if (yuv_current == yuv_frame_size){
    //An entire YUV frame has been received
    yuv_current = 0;
    
    offset_y = yuv_planes.offset_y;
    offset_u = yuv_planes.offset_u;
    offset_v = yuv_planes.offset_v;
    
    if (pwrite (fd, file_buffer, yuv_frame_size, 0) == -1){
      fprintf (stderr, "error: pwrite\n");
      if (omxcam_video_stop ()) log_error ();
      return;
    }
    
    if (++yuv_frames == 10){
      //All the frames have been received
      if (omxcam_video_stop ()) log_error ();
    }
  }
}

int save_rgb (char* filename, omxcam_video_settings_t* settings){
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
  
  if (omxcam_video_start (settings, 0)) log_error ();
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return 1;
  }
  
  return 0;
}

int save_yuv (char* filename, omxcam_video_settings_t* settings){
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
      &yuv_planes);
  omxcam_yuv_planes_slice (settings->camera.width, &yuv_planes_slice);
  
  //Frame size
  yuv_frame_size = yuv_planes.offset_v + yuv_planes.length_v;
  
  offset_y = yuv_planes.offset_y;
  offset_u = yuv_planes.offset_u;
  offset_v = yuv_planes.offset_v;
  
  //Allocate the buffer
  file_buffer = (uint8_t*)malloc (sizeof (uint8_t)*yuv_frame_size);
  
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
  //1920x1080 30fps by default
  omxcam_video_settings_t settings;
  
  //Capture a raw RGB video, 640x480 @30fps (10 frames)
  omxcam_video_init (&settings);
  settings.buffer_callback = buffer_callback_rgb;
  settings.format = OMXCAM_FORMAT_RGB888;
  settings.camera.width = 640;
  settings.camera.height = 480;
  
  if (save_rgb ("video.rgb", &settings)) return 1;
  
  //Capture a raw YUV420 video, 640x480 @30fps (10 frames)
  settings.buffer_callback = buffer_callback_yuv;
  settings.format = OMXCAM_FORMAT_YUV420;
  
  if (save_yuv ("video.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}