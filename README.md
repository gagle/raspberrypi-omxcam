omxcam
======

#### OpenMAX camera abstraction layer for the Raspberry Pi ####

[Forum thread](http://www.raspberrypi.org/forums/viewtopic.php?f=43&t=72523)

The Raspberry Pi uses the Bellagio's implementation of the OpenMAX IL specification in order to expose the media codecs (audio/image/video) available for the board. Broadcom, the manufacturer of the SoC, extends it by adding some propietary communication between the components. OpenMAX it's by nature open source. The problem here is the Broadcom's specific implementation which is closed source, so despite being the Raspberry Pi a platform built on top of an open source specification, there are some hidden parts in the interface that you must discover as an IL client by trial and error and with the help of some Broadcom employees.

Furthermore, there is an extra layer called MMAL (Multi-Media Abstraction Layer) that sits on top of OpenMAX IL but it's also closed source and written by Broadcom. This API it's supposed to be used by the developers to communicate with the media components, including the camera module. But there's a problem with it, it lacks of documentation. It's a hard job to get used by simply looking at the raspistill/raspivid examples. MMAL it's not a camera library, you still need to write how the components interact with each other.

This library talks directly with OpenMAX IL and it's just an abstraction layer to ease the camera usage. It's not bloated with a lot of functionalities, it just provides the minimum functions to start and stop the streaming of the media content (image/video). You receive the raw content directly from the OpenMAX IL layer and you decide how you want to process the data.

You will notice that the only available encoding algorithms are JPEG and H264. That's for a good reason; they are the only encoders that are hardware-accelerated. Encoding the images/videos with another encoder would take too long.

#### Some references ####

If you want to see [SSCCE](http://www.sscce.org/) examples, check these two projects: [jpeg](https://github.com/gagle/raspberrypi-openmax-jpeg) and [h264](https://github.com/gagle/raspberrypi-openmax-h264).

For a global understanding of the camera module check the documentation of the [picamera](picamera.readthedocs.org) project.

### API ###

When you start the streaming, a new thread is spawned and it's the responsible of emitting the data. You define a buffer callback and you receive the data when it's ready to be consumed, that's all. You don't even need to `malloc()` and `free()` the buffers, just use them. If you need some kind of inter-thread communication, I recommend a lock-free queue. Look at the [concurrency kit](http://concurrencykit.org/) project for more details, especially the `ck_ring_*()` functions.

- [Error handling](#error_handling)
- [Camera modes](#camera_modes)
- [Image streaming](#image_streaming)
- [Video streaming](#video_streaming)
- [Common settings between image and video](#common_settings)
- [OpenGL](#opengl)

<a name="error_handling"></a>
#### Error handling ####



<a name="camera_modes"></a>
#### Camera modes ####



<a name="image_streaming"></a>
#### Image streaming ####

```c
void buffer_callback (uint8_t* buffer, uint32_t length){
  //buffer: the data
  //length: the length of the buffer
}

int main (){
  //The settings of the image capture
  omxcam_still_settings_t settings;
  
  //Initialize the settings with default values
  omxcam_still_init (&settings);
  
  //Set the buffer callback, this is mandatory
  settings.buffer_callback = buffer_callback;
  
  //Start the image streaming
  omxcam_still_start (&settings);
}
```

<a name="video_streaming"></a>
#### Video streaming ####

```c
void buffer_callback (uint8_t* buffer, uint32_t length){
  //buffer: the data
  //length: the length of the buffer
}

int main (){
  //The settings of the video capture
  omxcam_video_settings_t settings;
  
  //Initialize the settings with default values
  omxcam_video_init (&settings);
  
  //Set the buffer callback, this is mandatory
  settings.buffer_callback = buffer_callback;
  
  //Two capture modes: with or without a timer
  //Capture 3000ms
  omxcam_video_start (&settings, 3000);
  
  //Capture indefinitely
  omxcam_video_start ($settings, OMXCAM_CAPTURE_FOREVER);
  
  //Then, from anywhere in your code you can stop the video capture
  //omxcam_stop_video ();
}
```

<a name="common_settings"></a>
#### Common settings between image and video ####



<a name="opengl"></a>
#### OpenGL ####

