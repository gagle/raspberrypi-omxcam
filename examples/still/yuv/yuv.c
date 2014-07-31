#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void on_data (omxcam_buffer_t buffer){
  //Append the data to the buffers
  memcpy (file_buffer + offset_y, buffer.data + yuv_planes_slice.offset_y,
      yuv_planes_slice.length_y);
  offset_y += yuv_planes_slice.length_y;
  
  memcpy (file_buffer + offset_u, buffer.data + yuv_planes_slice.offset_u,
      yuv_planes_slice.length_u);
  offset_u += yuv_planes_slice.length_u;
  
  memcpy (file_buffer + offset_v, buffer.data + yuv_planes_slice.offset_v,
      yuv_planes_slice.length_v);
  offset_v += yuv_planes_slice.length_v;
}

int save (char* filename, omxcam_still_settings_t* settings){
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
  */
  
  printf ("capturing %s\n", filename);
  
  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return 1;
  }
  
  omxcam_yuv_planes (settings->camera.width, settings->camera.height,
      &yuv_planes);
  omxcam_yuv_planes_slice (settings->camera.width, &yuv_planes_slice);
  
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
  omxcam_still_settings_t settings;
  omxcam_still_init (&settings);
  
  //2592x1944 by default
  
  settings.on_data = on_data;
  settings.format = OMXCAM_FORMAT_YUV420;
  //Shutter speed in milliseconds, (1/8)*1e6
  settings.camera.shutter_speed = 125000;
  settings.camera.width = 1296;
  
  /*
  Please note that the original aspect ratio of an image is 4:3. If you set
  dimensions with different ratios, the final image will still have the same
  aspect ratio (4:3) but you will notice that it will be cropped to the given
  dimensions.
  
  For example:
  - You want to take an image: 1296x730, 16:9.
  - The camera captures at 2592x1944, 4:3.
  - If you're capturing a raw image (no encoder), the width and the height need
    to be multiple of 32 and 16, respectively. You don't need to ensure that the
    dimensions are correct when capturing an image, this is done automatically,
    but you need to know them in order to open the file with the correct
    dimensions.
  - To go from 2592x1944 to 1296x730 the image needs to be resized to the
    "nearest" dimensions of the destination image but maintaining the 4:3 aspect
    ratio, that is, it is resized to 1296x972 (1296/(4/3) = 972).
  - The resized image it's cropped to 1312x736 in a centered way as depicted in
    the following diagram:
    
        --    ++++++++++++++++++++    --
    120 |     +                  +     |
        +-    +------------------+     |
        |     +                  +     |
    736 |     +                  +     | 976 (972 rounded up)
        |     +                  +     |
        +-    +------------------+     |
    120 |     +                  +     |
        --    ++++++++++++++++++++    --
                      1312
  
    The inner image is what you get and the outer image is what it's captured by
    the camera.
  */
  
  //YUV420, 1296x730, 16:9
  settings.camera.height = 730;
  
  if (save ("still-1312x736.yuv", &settings)) return 1;
  
  //YUV420, 1296x972, 4:3
  
  settings.camera.height = 972;
  
  if (save ("still-1312x976.yuv", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}