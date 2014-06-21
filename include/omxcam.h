#ifndef OMXCAM_H
#define OMXCAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <IL/OMX_Broadcom.h>

#include "omxcam_version.h"

#if __GNUC__ >= 4
# define OMXCAM_EXTERN __attribute__ ((visibility ("default")))
#else
# define OMXCAM_EXTERN //Empty
#endif

//Handy way to sleep forever while recording a video
#define OMXCAM_CAPTURE_FOREVER 0

//Error definitions, expand if necessary
#define OMXCAM_ERRNO_MAP(XX)                                                   \
  XX (0, ERROR_NONE, "success")                                                \
  XX (1, ERROR_CAMERA_MODULE, "camera module is not ready to be used")         \
  XX (2, ERROR_DRIVERS, "cannot load the camera drivers")                      \
  XX (3, ERROR_INIT, "initialization error")                                   \
  XX (4, ERROR_INIT_CAMERA, "cannot initialize the 'camera' component")        \
  XX (5, ERROR_INIT_IMAGE_ENCODER, "cannot initialize the 'image_encode' "     \
      "component")                                                             \
  XX (6, ERROR_INIT_VIDEO_ENCODER, "cannot initialize the 'video_encode' "     \
      "component")                                                             \
  XX (7, ERROR_INIT_NULL_SINK, "cannot initialize the 'null_sink' component")  \
  XX (8, ERROR_DEINIT, "deinitialization error")                               \
  XX (9, ERROR_DEINIT_CAMERA, "cannot deinitialize the 'camera' component")    \
  XX (10, ERROR_DEINIT_IMAGE_ENCODER, "cannot deinitialize the 'image_encode' "\
      "component")                                                             \
  XX (11, ERROR_DEINIT_VIDEO_ENCODER, "cannot deinitialize the 'video_encode' "\
      "component")                                                             \
  XX (12, ERROR_DEINIT_NULL_SINK, "cannot deinitialize the 'null_sink' "       \
      "component")                                                             \
  XX (13, ERROR_CAPTURE, "error while capturing")                              \
  XX (14, ERROR_CAMERA_RUNNING, "camera is already running")                   \
  XX (15, ERROR_CAMERA_STOPPING, "camera is already being stopped")            \
  XX (16, ERROR_BAD_PARAMETER, "incorrect parameter value")                    \
  XX (17, ERROR_STILL, "still error")                                          \
  XX (18, ERROR_VIDEO, "video error")                                          \
  XX (19, ERROR_JPEG, "error configuring jpeg encoder")                        \
  XX (20, ERROR_H264, "error configuring h264 encoder")                        \
  XX (21, ERROR_LOADED, "cannot transition to the Loaded state")               \
  XX (22, ERROR_IDLE, "cannot transition to the Idle state")                   \
  XX (23, ERROR_EXECUTING, "cannot transition to the Executing state")         \
  XX (24, ERROR_FORMAT, "invalid encoding format")                             \
  XX (25, ERROR_SLEEP, "cannot sleep the thread")                              \
  XX (26, ERROR_WAKE, "cannot wake the thread")                                \
  XX (27, ERROR_LOCK, "cannot lock the thread")                                \
  XX (28, ERROR_UNLOCK, "cannot unlock the thread")

typedef enum {
  OMXCAM_FALSE,
  OMXCAM_TRUE
} omxcam_bool;

typedef enum {
  OMXCAM_FORMAT_RGB888,
  OMXCAM_FORMAT_RGBA8888,
  OMXCAM_FORMAT_YUV420,
  OMXCAM_FORMAT_JPEG,
  OMXCAM_FORMAT_H264
} omxcam_format;

typedef enum {
  OMXCAM_EXPOSURE_OFF = OMX_ExposureControlOff,
  OMXCAM_EXPOSURE_AUTO = OMX_ExposureControlAuto,
  OMXCAM_EXPOSURE_NIGHT = OMX_ExposureControlNight,
  OMXCAM_EXPOSURE_BLACK_LIGHT = OMX_ExposureControlBackLight,
  OMXCAM_EXPOSURE_SPOTLIGHT = OMX_ExposureControlSpotLight,
  OMXCAM_EXPOSURE_SPORTS = OMX_ExposureControlSports,
  OMXCAM_EXPOSURE_SNOW = OMX_ExposureControlSnow,
  OMXCAM_EXPOSURE_BEACH = OMX_ExposureControlBeach,
  OMXCAM_EXPOSURE_LARGE_APERTURE = OMX_ExposureControlLargeAperture,
  OMXCAM_EXPOSURE_SMALL_APERTURE = OMX_ExposureControlSmallAperture,
  OMXCAM_EXPOSURE_VERY_LONG = OMX_ExposureControlVeryLong,
  OMXCAM_EXPOSURE_FIXED_FPS = OMX_ExposureControlFixedFps,
  OMXCAM_EXPOSURE_NIGHT_WITH_PREVIEW = OMX_ExposureControlNightWithPreview,
  OMXCAM_EXPOSURE_ANTISHAKE = OMX_ExposureControlAntishake,
  OMXCAM_EXPOSURE_FIREWORKS = OMX_ExposureControlFireworks
} omxcam_exposure;

typedef enum {
  OMXCAM_IMAGE_FILTER_NONE = OMX_ImageFilterNone,
  OMXCAM_IMAGE_FILTER_NOISE = OMX_ImageFilterNoise,
  OMXCAM_IMAGE_FILTER_EMBOSS = OMX_ImageFilterEmboss,
  OMXCAM_IMAGE_FILTER_NEGATIVE = OMX_ImageFilterNegative,
  OMXCAM_IMAGE_FILTER_SKETCH = OMX_ImageFilterSketch,
  OMXCAM_IMAGE_FILTER_OILPAINT = OMX_ImageFilterOilPaint,
  OMXCAM_IMAGE_FILTER_HATCH = OMX_ImageFilterHatch,
  OMXCAM_IMAGE_FILTER_GPEN = OMX_ImageFilterGpen,
  OMXCAM_IMAGE_FILTER_ANTIALIAS = OMX_ImageFilterAntialias,
  OMXCAM_IMAGE_FILTER_DERING = OMX_ImageFilterDeRing,
  OMXCAM_IMAGE_FILTER_SOLARIZE = OMX_ImageFilterSolarize,
  OMXCAM_IMAGE_FILTER_WATERCOLOR = OMX_ImageFilterWatercolor,
  OMXCAM_IMAGE_FILTER_PASTEL = OMX_ImageFilterPastel,
  OMXCAM_IMAGE_FILTER_SHARPEN = OMX_ImageFilterSharpen,
  OMXCAM_IMAGE_FILTER_FILM = OMX_ImageFilterFilm,
  OMXCAM_IMAGE_FILTER_BLUR = OMX_ImageFilterBlur,
  OMXCAM_IMAGE_FILTER_SATURATION = OMX_ImageFilterSaturation,
  OMXCAM_IMAGE_FILTER_DEINTERLACE_LINE_DOUBLE =
      OMX_ImageFilterDeInterlaceLineDouble,
  OMXCAM_IMAGE_FILTER_DEINTERLACE_ADVANCED = OMX_ImageFilterDeInterlaceAdvanced,
  OMXCAM_IMAGE_FILTER_COLOUR_SWAP = OMX_ImageFilterColourSwap,
  OMXCAM_IMAGE_FILTER_WASHED_OUT = OMX_ImageFilterWashedOut,
  OMXCAM_IMAGE_FILTER_COLOUR_POINT = OMX_ImageFilterColourPoint,
  OMXCAM_IMAGE_FILTER_POSTERISE = OMX_ImageFilterPosterise,
  OMXCAM_IMAGE_FILTER_COLOUR_BALANCE = OMX_ImageFilterColourBalance,
  OMXCAM_IMAGE_FILTER_CARTOON = OMX_ImageFilterCartoon
} omxcam_image_filter;

typedef enum {
  OMXCAM_METERING_AVERAGE = OMX_MeteringModeAverage,
  OMXCAM_METERING_SPOT = OMX_MeteringModeSpot,
  OMXCAM_METERING_MATRIX = OMX_MeteringModeMatrix,
  OMXCAM_METERING_BACKLIT = OMX_MeteringModeBacklit
} omxcam_metering;

typedef enum {
  OMXCAM_MIRROR_NONE = OMX_MirrorNone,
  OMXCAM_MIRROR_VERTICAL = OMX_MirrorVertical,
  OMXCAM_MIRROR_HORIZONTAL = OMX_MirrorHorizontal,
  OMXCAM_MIRROR_BOTH = OMX_MirrorBoth
} omxcam_mirror;

typedef enum {
  OMXCAM_ROTATION_NONE = 0,
  OMXCAM_ROTATION_90 = 90,
  OMXCAM_ROTATION_180 = 180,
  OMXCAM_ROTATION_270 = 270
} omxcam_rotation;

typedef enum {
  OMXCAM_WHITE_BALANCE_OFF = OMX_WhiteBalControlOff,
  OMXCAM_WHITE_BALANCE_AUTO = OMX_WhiteBalControlAuto,
  OMXCAM_WHITE_BALANCE_SUNLIGHT = OMX_WhiteBalControlSunLight,
  OMXCAM_WHITE_BALANCE_CLOUDY = OMX_WhiteBalControlCloudy,
  OMXCAM_WHITE_BALANCE_SHADE = OMX_WhiteBalControlShade,
  OMXCAM_WHITE_BALANCE_TUNGSTEN = OMX_WhiteBalControlTungsten,
  OMXCAM_WHITE_BALANCE_FLUORESCENT = OMX_WhiteBalControlFluorescent,
  OMXCAM_WHITE_BALANCE_INCANDESCENT = OMX_WhiteBalControlIncandescent,
  OMXCAM_WHITE_BALANCE_FLASH = OMX_WhiteBalControlFlash,
  OMXCAM_WHITE_BALANCE_HORIZON = OMX_WhiteBalControlHorizon
} omxcam_white_balance;

typedef enum {
#define XX(errno, name, _) OMXCAM_ ## name = errno,
  OMXCAM_ERRNO_MAP (XX)
#undef XX
} omxcam_errno;

typedef struct {
  uint32_t width;
  uint32_t height;
  int32_t sharpness;
  int32_t contrast;
  uint32_t brightness;
  int32_t saturation;
  omxcam_bool shutter_speed_auto;
  uint32_t shutter_speed;
  omxcam_bool iso_auto;
  uint32_t iso;
  omxcam_exposure exposure;
  int32_t exposure_compensation;
  omxcam_mirror mirror;
  omxcam_rotation rotation;
  omxcam_bool color_enhancement;
  uint32_t color_u;
  uint32_t color_v;
  omxcam_bool noise_reduction;
  omxcam_metering metering;
  omxcam_white_balance white_balance;
  //The gains are used if the white balance is set to off
  uint32_t white_balance_red_gain;
  uint32_t white_balance_blue_gain;
  omxcam_image_filter image_filter;
  uint32_t roi_top;
  uint32_t roi_left;
  uint32_t roi_width;
  uint32_t roi_height;
  //Used only in video mode
  uint32_t framerate;
  //Used only in video mode
  omxcam_bool video_stabilisation;
} omxcam_camera_settings_t;

typedef struct {
  uint32_t offset_y;
  uint32_t length_y;
  uint32_t offset_u;
  uint32_t length_u;
  uint32_t offset_v;
  uint32_t length_v;
} omxcam_yuv_planes_t;

typedef struct {
  char* key;
  char* value;
} omxcam_exif_tag_t;

typedef struct {
  uint32_t quality;
  omxcam_bool exif_enable;
  omxcam_exif_tag_t* exif_tags;
  uint32_t exif_valid_tags;
  omxcam_bool ijg;
  omxcam_bool thumbnail_enable;
  uint32_t thumbnail_width;
  uint32_t thumbnail_height;
  omxcam_bool raw_bayer_enable;
} omxcam_jpeg_settings_t;

typedef struct {
  uint32_t bitrate;
  uint32_t idr_period;
} omxcam_h264_settings_t;

#define OMXCAM_COMMON_SETTINGS                                                 \
  omxcam_camera_settings_t camera;                                             \
  omxcam_format format;                                                        \
  void (*buffer_callback)(uint8_t* buffer, uint32_t length);

typedef struct {
  OMXCAM_COMMON_SETTINGS
  omxcam_jpeg_settings_t jpeg;
} omxcam_still_settings_t;

typedef struct {
  OMXCAM_COMMON_SETTINGS
  omxcam_h264_settings_t h264;
} omxcam_video_settings_t;

/*
 * Returns the string name of the given error.
 */
OMXCAM_EXTERN const char* omxcam_error_name (omxcam_errno error);

/*
 * Returns the string message of the given error.
 */
OMXCAM_EXTERN const char* omxcam_strerror (omxcam_errno error);

/*
 * Returns the last error, if any.
 */
OMXCAM_EXTERN omxcam_errno omxcam_last_error ();

/*
 * Prints to stderr the last error in this way:
 *
 * omxcam: <error_name>: <strerror>\n
 */
OMXCAM_EXTERN void omxcam_perror ();

/*
 * Returns the library version packed into a single integer. 8 bits are used for
 * each component, with the patch number stored in the 8 least significant
 * bits, e.g. version 1.2.3 returns 0x010203.
 */
OMXCAM_EXTERN uint32_t omxcam_version ();

/*
* Returns the library version number as a string, e.g. 1.2.3
*/
OMXCAM_EXTERN const char* omxcam_version_string ();

/*
 * Rounds up a number given a divisor. For example, omxcam_round(1944, 16)
 * returns 1952. It is mainly used to align values according to the OpenMAX IL
 * and the Raspberry Pi camera board.
 */
OMXCAM_EXTERN uint32_t omxcam_round (uint32_t value, uint32_t divisor);

/*
 * Given the width and height of a frame, returns the offset and length of each
 * of the yuv planes.
 * 
 * For example, a 2592x1944 yuv frame is organized as follows:
 *
 * omxcam_yuv_planes_t yuv;
 * omxcam_yuv_planes (2592, 1944, &yuv);
 * printf (
 *   "y: %d %d\n"
 *   "u: %d %d\n"
 *   "v: %d %d\n",
 *   yuv.offset_y, yuv.length_y,
 *   yuv.offset_u, yuv.length_u,
 *   yuv.offset_v, yuv.length_v
 * );
 *
 * y: 0 5059584
 * u: 5059584 1264896
 * v: 6324480 1264896
 */
OMXCAM_EXTERN void omxcam_yuv_planes (
    uint32_t width,
    uint32_t height,
    omxcam_yuv_planes_t* planes);

/*
 * Same as 'omxcam_yuv_planes()' but used to calculate the offset and length of
 * the planes of a payload buffer.
 */
OMXCAM_EXTERN void omxcam_yuv_planes_slice (
    uint32_t width,
    omxcam_yuv_planes_t* planes);

/*
 * Sets the default settings for the image capture.
 */
OMXCAM_EXTERN void omxcam_still_init (omxcam_still_settings_t* settings);

/*
 * Starts the image capture with the given settings.
 */
OMXCAM_EXTERN int omxcam_still_start (omxcam_still_settings_t* settings);

/*
 * Stops the image capture and unblocks the current thread. It is safe to use
 * from anywhere in your code. You can call it from inside the 'buffer_callback'
 * or from another thread.
 */
OMXCAM_EXTERN int omxcam_still_stop ();

/*
 * Sets the default settings for the video capture.
 */
OMXCAM_EXTERN void omxcam_video_init (omxcam_video_settings_t* settings);

/*
 * Starts the video capture with the given settings. While the video is being
 * streamed, the current thread is blocked. If you want to sleep the thread
 * forever, use the macro OMXCAM_CAPTURE_FOREVER:
 * 
 * omxcam_video_start(settings, OMXCAM_CAPTURE_FOREVER);
 */
OMXCAM_EXTERN int omxcam_video_start (
    omxcam_video_settings_t* settings,
    uint32_t ms);

/*
 * Stops the video capture and unblocks the current thread. It is safe to use
 * from anywhere in your code. You can call it from inside the 'buffer_callback'
 * or from another thread.
 */
OMXCAM_EXTERN int omxcam_video_stop ();

OMXCAM_EXTERN int omxcam_video_update_buffer_callback (
    void (*buffer_callback)(uint8_t* buffer, uint32_t length));

#ifdef __cplusplus
}
#endif

#endif