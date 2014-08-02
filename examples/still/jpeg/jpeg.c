#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "omxcam.h"

int log_error (){
  omxcam_perror ();
  return 1;
}

int fd;

void on_data (omxcam_buffer_t buffer){
  //Append the buffer to the file
  //Note: Writing the data directly to disk will slow down the capture speed
  //due to the I/O access. A posible workaround is to save the buffers into
  //memory, similar to the still/raw.c (YUV) example, and then write the
  //whole image to disk
  if (pwrite (fd, buffer.data, buffer.length, 0) == -1){
    fprintf (stderr, "error: pwrite\n");
    if (omxcam_still_stop ()) log_error ();
  }
}

int save (char* filename, omxcam_still_settings_t* settings){
  printf ("capturing %s\n", filename);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd == -1){
    fprintf (stderr, "error: open\n");
    return -1;
  }
  
  if (omxcam_still_start (settings)) return log_error ();
  
  //Close the file
  if (close (fd)){
    fprintf (stderr, "error: close\n");
    return -1;
  }
  
  return 0;
}

int main (){
  //2592x1944 by default
  omxcam_still_settings_t settings;
  
  //Capture an image with default settings
  omxcam_still_init (&settings);
  settings.on_data = on_data;
  //settings.camera.shutter_speed = 1000*1000;
  //settings.camera.exposure = OMXCAM_EXPOSURE_NIGHT;
  settings.camera.shutter_speed = 50000;
  settings.camera.iso = 200;
  settings.jpeg.thumbnail.width = 0;
  settings.jpeg.thumbnail.height = 0;
  settings.camera.image_filter = OMXCAM_IMAGE_FILTER_SHARPEN;
  
  if (save ("still-default-2592x1944.jpg", &settings)) return 1;
  
  //Capture gray image with shutter speed 1/4, EV -10 and some EXIF tags
  settings.camera.exposure_compensation = -10;
  //Shutter speed in microseconds, (1/4)*1e6
  settings.camera.shutter_speed = 250000;
  //Values of color_effects.u and color_effects.v are 128 by default,
  //a gray image
  settings.camera.color_effects.enabled = OMXCAM_TRUE;
  
  //See firmware/documentation/ilcomponents/image_decode.html for valid keys
  //See http://www.media.mit.edu/pia/Research/deepview/exif.html#IFD0Tags
  //for valid keys and their descriptions
  omxcam_exif_tag_t exif_tags[] = {
    //Manufacturer
    { "IFD0.Make", "omxcam" }
  };
  settings.jpeg.exif.tags = exif_tags;
  settings.jpeg.exif.valid_tags = 1;
  
  //if (save ("still-2592x1944.jpg", &settings)) return 1;
  
  printf ("ok\n");
  
  return 0;
}