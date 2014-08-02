omxcam
======

#### OpenMAX camera abstraction layer for the Raspberry Pi ####

[Forum thread](http://www.raspberrypi.org/forums/viewtopic.php?f=43&t=72523)

The Raspberry Pi uses the Bellagio's implementation of the OpenMAX IL specification in order to expose the media codecs (audio/image/video) available for the board. Broadcom, the manufacturer of the SoC, extends it by adding some propietary communication between the components. OpenMAX it's by nature open source. The problem here is the Broadcom's specific implementation which is closed source, so despite being the Raspberry Pi a platform built on top of an open source specification, there are some hidden parts in the interface that you must discover as an IL client by trial and error and with the little help that some of the Broadcom employees can provide.

Furthermore, there is an extra layer called MMAL (Multi-Media Abstraction Layer) that sits on top of OpenMAX IL but it's also closed source and written by Broadcom. This API it's supposed to be used by the developers to communicate with the media components, including the camera module. But there's a problem with it, it lacks of documentation. It's a hard job to get used to it by simply looking at the raspistill/raspivid examples. MMAL it's not a camera library, so you still need to write how the components interact with each other.

This library talks directly with OpenMAX IL and it's just an abstraction layer to ease the camera usage. It's not bloated with a lot of functionalities, it just provides the minimum functions to start and stop the streaming of the media content (image/video). You receive the content directly from the OpenMAX IL layer and you decide how to process the data.

You will notice that the only available encoding algorithms are JPEG and H264. That's for a good reason; they are the only encoders that are hardware-accelerated. Encoding the images/videos with another encoder would take too long.

#### Some references ####

If you want to see [SSCCE](http://www.sscce.org/) examples, check these two projects: [jpeg](https://github.com/gagle/raspberrypi-openmax-jpeg) and [h264](https://github.com/gagle/raspberrypi-openmax-h264).

For a global understanding of the camera module check the documentation of the [picamera](http://picamera.readthedocs.org) project.

### API ###

When you start the streaming, a new thread is spawned and it's the responsible of emitting the data. You define a buffer callback and you receive the data when it's ready to be consumed, that's all. You don't even need to `malloc()` and `free()` the buffers, just use them. If you need some kind of inter-thread communication, I recommend a lock-free queue. Look at the [concurrency kit](http://concurrencykit.org/) project for more details, especially the `ck_ring_*()` functions.

- [Build](#build)
- [Error handling](#error_handling)
- [Camera modes](#camera_modes)
- [Camera settings](#camera_settings)
- [Image streaming](#image_streaming)
- [Video streaming](#video_streaming)
- [OpenGL](#opengl)
- [Utilities](#utilities)

The [omxcam.h](https://github.com/gagle/raspberrypi-omxcam/blob/master/include/omxcam.h) header file is what you need to include in your projects. All the enumerations, type definitions and prototypes are located here. This README file introduces the library and explains just a few things. For a full reference, please take a look at the header `omxcam.h`.

<a name="build"></a>
#### Build ####

Two makefiles are provided with each example: `Makefile` and `Makefile-shared`. If you compile an example with the latter makefile you will also need to execute the makefile [./Makefile-shared](https://github.com/gagle/raspberrypi-omxcam/blob/master/Makefile-shared) in order to compile the library as a shared library. This will generate the file `./lib/libomxcam.so`.

For example, to compile the example [./examples/still/jpeg](https://github.com/gagle/raspberrypi-omxcam/tree/master/examples/still/jpeg) with the whole code embedded in the binary file:

```
$ cd ./examples/still/jpeg
$ make
```
To compile the library as a shared library and link it to the binary file:

```
$ make -f Makefile-shared
$ cd ./examples/still/jpeg
$ make -f Makefile-shared
```

Please take into account that the shared library needs to be located in the `./lib` directory due to this LDFLAG that you can find in [./examples/Makefile-shared-common](https://github.com/gagle/raspberrypi-omxcam/blob/master/examples/Makefile-shared-common):

```
-Wl,-rpath=$(OMXCAM_HOME)/lib
```

If you need to store the library in another place, change the path, use the environment variable `LD_LIBRARY_PATH` or put the library in a common place, e.g. `/usr/lib`.

<a name="error_handling"></a>
#### Error handling ####

All the functions that have a non-void return, return an `int` type: `0` if the function succeeds, `-1` otherwise. If something fails, you can call to `omxcam_last_error()` to get the last error number. You have additional functions such as `omxcam_error_name(omxcam_errno)` which returns the error name, `omxcam_strerror(omxcam_errno)` which returns the string message describing the error and `omxcam_perror()` which formats and prints the last error to the stderr, something similar to this:

```
omxcam: OMXCAM_ERROR_INIT_CAMERA: cannot initialize the 'camera' component
```

You should not get any error. If you receive an error and you are sure that it's not due to bad parameters, you can enable the debugging flag `-DOMXCAM_DEBUG` and recompile the library. An even more specific error message should be printed to the stdout, for example:

```
omxcam: error: OMX_EventError: OMX_ErrorInsufficientResources (function: 'event_handler', file: '../../../src/core.c', line: 41)
```

Copy all the debug messages and open an issue.

All the error codes and their descriptive messages are:

```
  X (0, ERROR_NONE, "success")                                                 \
  X (1, ERROR_CAMERA_MODULE, "camera module is not ready to be used")          \
  X (2, ERROR_DRIVERS, "cannot load the camera drivers")                       \
  X (3, ERROR_INIT, "initialization error")                                    \
  X (4, ERROR_INIT_CAMERA, "cannot initialize the 'camera' component")         \
  X (5, ERROR_INIT_IMAGE_ENCODER, "cannot initialize the 'image_encode' "      \
      "component")                                                             \
  X (6, ERROR_INIT_VIDEO_ENCODER, "cannot initialize the 'video_encode' "      \
      "component")                                                             \
  X (7, ERROR_INIT_NULL_SINK, "cannot initialize the 'null_sink' component")   \
  X (8, ERROR_DEINIT, "deinitialization error")                                \
  X (9, ERROR_DEINIT_CAMERA, "cannot deinitialize the 'camera' component")     \
  X (10, ERROR_DEINIT_IMAGE_ENCODER, "cannot deinitialize the 'image_encode' " \
      "component")                                                             \
  X (11, ERROR_DEINIT_VIDEO_ENCODER, "cannot deinitialize the 'video_encode' " \
      "component")                                                             \
  X (12, ERROR_DEINIT_NULL_SINK, "cannot deinitialize the 'null_sink' "        \
      "component")                                                             \
  X (13, ERROR_CAPTURE, "error while capturing")                               \
  X (14, ERROR_CAMERA_RUNNING, "camera is already running")                    \
  X (15, ERROR_CAMERA_NOT_RUNNING, "camera is not running")                    \
  X (16, ERROR_CAMERA_STOPPING, "camera is already being stopped")             \
  X (17, ERROR_CAMERA_UPDATE, "camera is not ready to be updated")             \
  X (18, ERROR_BAD_PARAMETER, "incorrect parameter value")                     \
  X (19, ERROR_VIDEO_ONLY, "action can be executed only in video mode")        \
  X (20, ERROR_STILL, "still error")                                           \
  X (21, ERROR_VIDEO, "video error")                                           \
  X (22, ERROR_JPEG, "error configuring jpeg encoder")                         \
  X (23, ERROR_H264, "error configuring h264 encoder")                         \
  X (24, ERROR_LOADED, "cannot transition to the Loaded state")                \
  X (25, ERROR_IDLE, "cannot transition to the Idle state")                    \
  X (26, ERROR_EXECUTING, "cannot transition to the Executing state")          \
  X (27, ERROR_FORMAT, "invalid encoding format")                              \
  X (28, ERROR_SLEEP, "cannot sleep the thread")                               \
  X (29, ERROR_WAKE, "cannot wake the thread")                                 \
  X (30, ERROR_LOCK, "cannot lock the thread")                                 \
  X (31, ERROR_UNLOCK, "cannot unlock the thread")
```

Note: If you receive a `ERROR_CAMERA_MODULE` error, make sure that the property `gpu_mem_512` or `gpu_mem_256` (depending on the model of the Raspberry Pi) has `128` or more MB in the file `/boot/config.txt`. Uncomment if necessary. For example, if you have a Raspberry Pi model B, Arch Linux ARM (build from june 2014) defaults to:

```
#gpu_mem_512=64
```

After:

```
gpu_mem_512=128
```

<a name="camera_modes"></a>
#### Camera modes ####



<a name="camera_settings"></a>
#### Camera settings ####

The `omxcam_still_settings_t` and `omxcam_video_settings_t` structs have a `camera` field that is used to configure the camera settings. Its type definition is `omxcam_camera_settings_t` and has the following fields:

```
type                     name                    default                     range
----                     ----                    -------                     -----
uint32_t                 width                   image 2592, video 1920      16 .. 2592 | 1920
uint32_t                 height                  image 1944, video 1080      16 .. 1944 | 1080
int32_t                  sharpness               0                           -100 .. 100
int32_t                  contrast                0                           -100 .. 100
uint32_t                 brightness              50                          0 .. 100
int32_t                  saturation              0                           -100 .. 100
uint32_t                 shutter_speed           0 (auto)                    0 ..
omxcam_iso               iso                     OMXCAM_ISO_AUTO
omxcam_exposure          exposure                OMXCAM_EXPOSURE_AUTO
int32_t                  exposure_compensation   0                           -24 .. 24
omxcam_mirror            mirror                  OMXCAM_MIRROR_NONE
omxcam_rotation          rotation                OMXCAM_ROTATION_NONE
omxcam_color_effects_t   color_effects
  omxcam_bool              enabled               OMXCAM_FALSE
  uint32_t                 u                     128                         0 .. 255
  uint32_t                 v                     128                         0 .. 255
omxcam_bool              color_denoise           OMXCAM_TRUE
omxcam_metering          metering                OMXCAM_METERING_AVERAGE
omxcam_white_balance_t   white_balance
  omxcam_white_balance     mode                  OMXCAM_WHITE_BALANCE_AUTO
  uint32_t                 red_gain              100                         0 .. 
  uint32_t                 blue_gain             100                         0 ..
omxcam_image_filter      image_filter            OMXCAM_IMAGE_FILTER_NONE
omxcam_drc               drc                     OMXCAM_DRC_OFF
omxcam_roi_t             roi
  uint32_t                 top                   0                           0 .. 100
  uint32_t                 left                  0                           0 .. 100
  uint32_t                 width                 0                           0 .. 100
  uint32_t                 height                0                           0 .. 100
uint32_t                 framerate               30                          2 ..
omxcam_bool              frame_stabilisation     OMXCAM_FALSE
```

All the previous settings can be used in video and still mode, except:

- Still only: `color_denoise`.
- Video only: `framerate`, `frame_stabilisation`.

For example, if you want to take a grayscale jpeg image with vga resolution (640x480), vertically mirrored and with a fixed shutter speed of 1/2 second:

```c
omxcam_still_settings_t settings;

omxcam_still_init (&settings);

settings.camera.mirror = OMXCAM_MIRROR_VERTICAL;

//Shutter speed in milliseconds
settings.camera.shutter_speed = 500;

//When 'camera.color_effects' is enabled, 'camera.color_effects.u' and
//'camera.color_effects.v' are used. They default to 128, the values for a
//grayscale image
settings.camera.color_effects = OMXCAM_TRUE;
```

The `omxcam_h264_settings_t` struct has the following fields:

```
type                     name                    default                     range
----                     ----                    -------                     -----
uint32_t                 bitrate                 17000000                    1 .. 25000000
uint32_t                 idr_period              OMXCAM_H264_IDR_PERIOD_OFF  0 ..
omxcam_bool              sei                     OMXCAM_FALSE
omxcam_eede_t            eede
  omxcam_bool              enabled               OMXCAM_FALSE
  uint32_t                 loss_rate             0                           0 .. 100
omxcam_quantization_t    qp
  omxcam_bool              enabled               OMXCAM_FALSE
  uint32_t                 i                     OMXCAM_H264_QP_OFF          1 .. 51
  uint32_t                 p                     OMXCAM_H264_QP_OFF          1 .. 51
```

<a name="image_streaming"></a>
#### Image streaming ####

```c
#include "omxcam.h"

void on_data (uint8_t* buffer, uint32_t length){
  //buffer: the data
  //length: the length of the buffer
}

int main (){
  //The settings of the image capture
  omxcam_still_settings_t settings;
  
  //Initialize the settings with default values (jpeg, 2592x1944)
  omxcam_still_init (&settings);
  
  //Set the buffer callback, this is mandatory
  settings.on_data = on_data;
  
  //Start the image streaming
  omxcam_still_start (&settings);
  
  //Then, from anywhere in your code you can stop the image capture
  //omxcam_stop_still ();
}
```

<a name="video_streaming"></a>
#### Video streaming ####

```c
#include "omxcam.h"

void on_data (uint8_t* buffer, uint32_t length){
  //buffer: the data
  //length: the length of the buffer
}

int main (){
  //The settings of the video capture
  omxcam_video_settings_t settings;
  
  //Initialize the settings with default values (h264, 1920x1080, 30fps)
  omxcam_video_init (&settings);
  
  //Set the buffer callback, this is mandatory
  settings.on_data = on_data;
  
  //Two capture modes: with or without a timer
  //Capture 3000ms
  omxcam_video_start (&settings, 3000);
  
  //Capture indefinitely
  omxcam_video_start (&settings, OMXCAM_CAPTURE_FOREVER);
  
  //Then, from anywhere in your code you can stop the video capture
  //omxcam_stop_video ();
}
```

<a name="opengl"></a>
#### OpenGL ####



<a name="utilities"></a>
#### Utilities ####

__Version__

Two functions are available:

- ___uint32_t omxcam_version()___  
  Returns the library version packed into a single integer. 8 bits are used for each component, with the patch number stored in the 8 least significant bits, e.g. version 1.2.3 returns `0x010203`.

- ___const char* omxcam_version()___  
  Returns the library version number as a string, e.g. `1.2.3`.

__YUV planes__

When capturing YUV images/video, you need to calculate the offset and length of each plane if you want to manipulate them somehow. There are a couple of functions that you can use for that purpose:

- __void omxcam_yuv_planes (uint32_t width, uint32_t height, omxcam_yuv_planes_t* planes)__  
  Given the width and height of a frame, returns an `omxcam_yuv_planes_t` struct with the offset and length of each plane. The struct definition contains the following fields:

  ```c
  uint32_t offset_y;
  uint32_t length_y;
  uint32_t offset_u;
  uint32_t length_u;
  uint32_t offset_v;
  uint32_t length_v;
  ```
  
  Tip: you can calculate the size of a yuv frame this way: `offset_v + length_v`.

- ___void omxcam_yuv_planes_slice (uint32_t width, omxcam_yuv_planes_t* planes)___  
   Same as `omxcam_yuv_planes()` but used to calculate the offset and length of the planes of a payload buffer.

Look at the [still/yuv](https://github.com/gagle/raspberrypi-omxcam/blob/master/examples/still/yuv/yuv.c) and [video/yuv](https://github.com/gagle/raspberrypi-omxcam/blob/master/examples/video/yuv/yuv.c) examples for further details.