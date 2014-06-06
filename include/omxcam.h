#ifndef OMXCAM_H
#define OMXCAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include <bcm_host.h>
#include <IL/OMX_Broadcom.h>

#include "omxcam_version.h"

#if __GNUC__ >= 4
# define OMXCAM_EXTERN __attribute__ ((visibility ("default")))
#else
# define OMXCAM_EXTERN //Empty
#endif

#define OMXCAM_INIT_STRUCTURE(x) \
  memset (&(x), 0, sizeof (x)); \
  (x).nSize = sizeof (x); \
  (x).nVersion.nVersion = OMX_VERSION; \
  (x).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (x).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (x).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (x).nVersion.s.nStep = OMX_VERSION_STEP

#define OMXCAM_CAMERA_NAME "OMX.broadcom.camera"
#define OMXCAM_IMAGE_ENCODE_NAME "OMX.broadcom.image_encode"
#define OMXCAM_VIDEO_ENCODE_NAME "OMX.broadcom.video_encode"
#define OMXCAM_NULL_SINK_NAME "OMX.broadcom.null_sink"

//Handy way to sleep forever while recording a video
#define OMXCAM_CAPTURE_FOREVER 0

//Error definitions, expand if necessary
#define OMXCAM_ERRNO_MAP(XX) \
  XX (0, ERROR_NONE, "success") \
  XX (1, ERROR_INIT_CAMERA, "cannot initialize the 'camera' component") \
  XX (2, ERROR_INIT_IMAGE_ENCODER, "cannot initialize the 'image_encode' component") \
  XX (3, ERROR_INIT_VIDEO_ENCODER, "cannot initialize the 'video_encode' component") \
  XX (4, ERROR_INIT_NULL_SINK, "cannot initialize the 'null_sink' component") \
  XX (5, ERROR_DRIVERS, "cannot load the camera drivers") \
  XX (6, ERROR_DEINIT_CAMERA, "cannot deinitialize the 'camera' component") \
  XX (7, ERROR_DEINIT_IMAGE_ENCODER, "cannot deinitialize the 'image_encode' component") \
  XX (8, ERROR_DEINIT_VIDEO_ENCODER, "cannot deinitialize the 'video_encode' component") \
  XX (9, ERROR_DEINIT_NULL_SINK, "cannot deinitialize the 'null_sink' component") \
  XX (10, ERROR_CAPTURE, "error while capturing") \
  XX (11, ERROR_STILL, "still error") \
  XX (12, ERROR_VIDEO, "video error") \
  XX (13, ERROR_JPEG, "error configuring jpeg encoder") \
  XX (14, ERROR_H264, "error configuring h264 encoder") \
  XX (15, ERROR_BAD_PARAMETER, "incorrect parameter value") \
  XX (16, ERROR_LOADED, "cannot transition to the Loaded state") \
  XX (17, ERROR_IDLE, "cannot transition to the Idle state") \
  XX (18, ERROR_EXECUTING, "cannot transition to the Executing state") \
  XX (19, ERROR_FORMAT, "invalid encoding format") \
  XX (20, ERROR_SLEEP, "cannot sleep the thread") \
  XX (21, ERROR_WAKE, "cannot wake the thread") \
  XX (22, ERROR_LOCK, "cannot lock the thread") \
  XX (23, ERROR_UNLOCK, "cannot unlock the thread")

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

#ifdef OMXCAM_DEBUG
#define omxcam_error(message, ...) \
  omxcam_error_(message, __func__, __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define omxcam_error(message, ...) //Empty
#endif

/*
 * Prints an error message to the stdout along with the file, line and function
 * name from where this function is called. It is printed if the cflag
 * OMXCAM_DEBUG is enabled. Use the omxcam_error() macro instead.
 */
void omxcam_error_ (
    const char* fmt,
    const char* fn,
    const char* file,
    int line,
    ...);

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
 * Sets the last error.
 */
void omxcam_set_last_error (omxcam_errno error);
/*
 * Prints to stderr the last error in this way:
 *
 * omxcam <error_name>: <strerror>\n
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
 * Component's event flags.
 */
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint32_t flags;
  int error;
} omxcam_event_t;

/*
 * Wrapper for an OpenMAX IL component.
 */
typedef struct {
  OMX_HANDLETYPE handle;
  omxcam_event_t event;
  OMX_STRING name;
  void (*buffer_callback)(uint8_t* buffer, uint32_t length);
} omxcam_component_t;

/*
 * Program's global context.
 */
typedef struct {
  omxcam_component_t camera;
  omxcam_component_t image_encode;
  omxcam_component_t video_encode;
  omxcam_component_t null_sink;
  OMX_BUFFERHEADERTYPE* output_buffer;
} omxcam_context_t;

//Context's global variable
omxcam_context_t omxcam_ctx;

/*
 * Enumeration with a mapping between all the OpenMAX IL events and a unique
 * number. The id allows them to be bitwise-or'ed, e.g.
 * OMXCAM_EVENT_ERROR | OMXCAM_EVENT_PORT_ENABLE == 0x3
 */
typedef enum {
  OMXCAM_EVENT_ERROR = 0x1,
  OMXCAM_EVENT_PORT_ENABLE = 0x2,
  OMXCAM_EVENT_PORT_DISABLE = 0x4,
  OMXCAM_EVENT_STATE_SET = 0x8,
  OMXCAM_EVENT_FLUSH = 0x10,
  OMXCAM_EVENT_MARK_BUFFER = 0x20,
  OMXCAM_EVENT_MARK = 0x40,
  OMXCAM_EVENT_PORT_SETTINGS_CHANGED = 0x80,
  OMXCAM_EVENT_PARAM_OR_CONFIG_CHANGED = 0x100,
  OMXCAM_EVENT_BUFFER_FLAG = 0x200,
  OMXCAM_EVENT_RESOURCES_ACQUIRED = 0x400,
  OMXCAM_EVENT_DYNAMIC_RESOURCES_AVAILABLE = 0x800,
  OMXCAM_EVENT_FILL_BUFFER_DONE = 0x1000
} omxcam_event;

typedef enum {
  OMXCAM_STATE_LOADED = OMX_StateLoaded,
  OMXCAM_STATE_IDLE = OMX_StateIdle,
  OMXCAM_STATE_EXECUTING = OMX_StateExecuting,
  OMXCAM_STATE_PAUSE = OMX_StatePause,
  OMXCAM_STATE_WAIT_FOR_RESOURCES = OMX_StateWaitForResources,
  OMXCAM_STATE_INVALID = OMX_StateInvalid
} omxcam_state;

/*
 * OpenMAX IL event handlers.
 */
OMX_ERRORTYPE event_handler (
    OMX_IN OMX_HANDLETYPE comp,
    OMX_IN OMX_PTR app_data,
    OMX_IN OMX_EVENTTYPE event,
    OMX_IN OMX_U32 data1,
    OMX_IN OMX_U32 data2,
    OMX_IN OMX_PTR event_data);
OMX_ERRORTYPE fill_buffer_done (
    OMX_IN OMX_HANDLETYPE comp,
    OMX_IN OMX_PTR app_data,
    OMX_IN OMX_BUFFERHEADERTYPE* buffer);

/*
 * OpenMAX IL miscellaneous dump functions.
 */
const char* omxcam_dump_OMX_COLOR_FORMATTYPE (OMX_COLOR_FORMATTYPE color);
const char* omxcam_dump_OMX_OTHER_FORMATTYPE (OMX_OTHER_FORMATTYPE format);
const char* omxcam_dump_OMX_AUDIO_CODINGTYPE (OMX_AUDIO_CODINGTYPE coding);
const char* omxcam_dump_OMX_VIDEO_CODINGTYPE (OMX_VIDEO_CODINGTYPE coding);
const char* omxcam_dump_OMX_IMAGE_CODINGTYPE (OMX_IMAGE_CODINGTYPE coding);
const char* omxcam_dump_OMX_STATETYPE (OMX_STATETYPE state);
const char* omxcam_dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);
const char* omxcam_dump_OMX_EVENTTYPE (OMX_EVENTTYPE event);
const char* omxcam_dump_OMX_INDEXTYPE (OMX_INDEXTYPE type);
void omxcam_dump_OMX_PARAM_PORTDEFINITIONTYPE (
    OMX_PARAM_PORTDEFINITIONTYPE* port);
void omxcam_dump_OMX_IMAGE_PARAM_PORTFORMATTYPE (
    OMX_IMAGE_PARAM_PORTFORMATTYPE* port);
void omxcam_dump_OMX_BUFFERHEADERTYPE (OMX_BUFFERHEADERTYPE* header);

/*
 * Prints a debug message to the stdout. It is printed if the cflag OMXCAM_DEBUG
 * is enabled. 
 */
void omxcam_trace (const char* fmt, ...);

/*
 * Sets an error originated from the EventHandler.
 */
void omxcam_event_error (omxcam_component_t* component);
/*
 * Creates the event flags handler for the component.
 */
int omxcam_event_create (omxcam_component_t* component);
/*
 * Destroys the event flags handler for the component.
 */
int omxcam_event_destroy (omxcam_component_t* component);
/*
 * Sets some events.
 * 
 * Unlocks the current thread if there are events waiting to the given events.
 */
int omxcam_event_wake (omxcam_component_t* component, omxcam_event event);
/*
 * Retrieve some events.
 *
 * Waits until the specified events have been set. For example, if
 * OMXCAM_EVENT_BUFFER_FLAG | OMXCAM_EVENT_MARK are passed, the thread will be
 * locked until it is woken up with the OMXCAM_EVENT_BUFFER_FLAG or
 * OMXCAM_EVENT_MARK events. When unlocked, the current events set are returned
 * in the 'current_events' pointer (null is allowed).
 *
 * OMXCAM_EVENT_ERROR is automatically handled.
 */
int omxcam_event_wait (
    omxcam_component_t* component,
    omxcam_event events,
    omxcam_event* current_events);

/*
 * Enables or disables the port of a component.
 */
int omxcam_component_port_enable (omxcam_component_t* component, uint32_t port);
int omxcam_component_port_disable (
    omxcam_component_t* component,
    uint32_t port);

/*
 * Initializes a component. All its ports are enabled and the OpenMAX IL event
 * handlers are configured, plus other things.
 */
int omxcam_component_init (omxcam_component_t* component);
/*
 * Deinitializes a component. All its ports are disabled, plus other things.
 */
int omxcam_component_deinit (omxcam_component_t* component);

/*
 * Changes the state of a component.
 */
int omxcam_component_change_state (
    omxcam_component_t* component,
    omxcam_state state);

/*
 * Allocates and frees an OpenMAX buffer for a given port for data streaming.
 */
int omxcam_buffer_alloc (omxcam_component_t* component, uint32_t port);
int omxcam_buffer_free (omxcam_component_t* component, uint32_t port);

/*
 * Initializes and deinitializes OpenMAX IL. They must be the first and last
 * functions to call.
 */
int omxcam_init ();
int omxcam_deinit ();

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t sharpness;
  uint32_t contrast;
  uint32_t brightness;
  uint32_t saturation;
  omxcam_bool shutter_speed_auto;
  uint32_t shutter_speed;
  omxcam_bool iso_auto;
  uint32_t iso;
  omxcam_exposure exposure;
  int32_t exposure_compensation;
  omxcam_mirror mirror;
  omxcam_rotation rotation;
  omxcam_bool color_enable;
  uint32_t color_u;
  uint32_t color_v;
  omxcam_bool noise_reduction_enable;
  omxcam_bool frame_stabilisation_enable;
  omxcam_metering metering;
  omxcam_white_balance white_balance;
  //The gains are used if white balance is set to off
  float white_balance_red_gain;
  float white_balance_blue_gain;
  omxcam_image_filter image_filter;
  //Used only in video mode
  uint32_t framerate;
} omxcam_camera_settings_t;

/*
 * Loads the camera drivers. After this call the red led is turned on and the
 * OpenMAX IL layer is ready to be configured.
 */
int omxcam_camera_load_drivers ();

/*
 * The camera needs to know which output port is going to be used to consume the
 * data. The capture port must be enabled just before the buffers are going to
 * be consumed, that is, when the state of the camera and all the enabled
 * components are OMX_StateExecuting.
 * 
 * video = 71
 * still = 72
 */
int omxcam_camera_capture_port_set (uint32_t port);
int omxcam_camera_capture_port_reset (uint32_t port);

/*
 * Sets the default settings for the camera.
 */
void omxcam_camera_init (
    omxcam_camera_settings_t* settings,
    uint32_t width,
    uint32_t height);

/*
 * Configures the OpenMAX IL camera component.
 */
int omxcam_camera_configure_omx (omxcam_camera_settings_t* settings, int video);

/*
 * Rounds up a number given a divisor. For example: omxcam_round(1944, 16)
 * returns 1952. It is mainly used to align values according to the OpenMAX IL
 * and the Raspberry Pi restrictions.
 */
OMXCAM_EXTERN uint32_t omxcam_round (uint32_t value, uint32_t divisor);

typedef struct {
  uint32_t offset_y;
  uint32_t length_y;
  uint32_t offset_u;
  uint32_t length_u;
  uint32_t offset_v;
  uint32_t length_v;
} omxcam_yuv_planes_t;

/*
 * Given the width and height of a frame, returns the offset and length of each
 * of the yuv planes.
 */
OMXCAM_EXTERN void omxcam_yuv_planes (
    omxcam_yuv_planes_t* planes,
    uint32_t width,
    uint32_t height);
/*
 * Same as 'omxcam_yuv_planes()' but used to calculate the offset and length of
 * the planes of a payload buffer.
 */
OMXCAM_EXTERN void omxcam_yuv_planes_slice (
    omxcam_yuv_planes_t* planes,
    uint32_t width);

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

/*
 * Sets the default settings for the jpeg encoder.
 */
void omxcam_jpeg_init (omxcam_jpeg_settings_t* settings);
/*
 * Adds an exif tag to the jpeg metadata.
 */
int omxcam_jpeg_add_tag (char* key, char* value);
/*
 * Configures the OpenMAX IL image_encode component with the jpeg settings.
 */
int omxcam_jpeg_configure_omx (omxcam_jpeg_settings_t* settings);

typedef struct {
  uint32_t bitrate;
  uint32_t idr_period;
} omxcam_h264_settings_t;

/*
 * Sets the default settings for the h264 encoder.
 */
void omxcam_h264_init (omxcam_h264_settings_t* settings);
/*
 * Configures the OpenMAX IL video_encode component with the h264 settings.
 */
int omxcam_h264_configure_omx (omxcam_h264_settings_t* settings);

#define OMXCAM_COMMON_SETTINGS \
  omxcam_camera_settings_t camera; \
  omxcam_format format; \
  void (*buffer_callback)(uint8_t* buffer, uint32_t length); \

typedef struct {
  OMXCAM_COMMON_SETTINGS
  omxcam_jpeg_settings_t jpeg;
} omxcam_still_settings_t;

/*
 * Sets the default settings for the still capture.
 */
OMXCAM_EXTERN void omxcam_still_init (omxcam_still_settings_t* settings);
/*
 * Starts capturing a still with the given settings.
 */
OMXCAM_EXTERN int omxcam_still_start (omxcam_still_settings_t* settings);
/*
 * Stops the still capture.
 */
OMXCAM_EXTERN int omxcam_still_stop ();

typedef struct {
  OMXCAM_COMMON_SETTINGS
  omxcam_h264_settings_t h264;
} omxcam_video_settings_t;

/*
 * Sets the default settings for the video capture.
 */
OMXCAM_EXTERN void omxcam_video_init (omxcam_video_settings_t* settings);
/*
 * Starts capturing a video with the given settings.
 */
OMXCAM_EXTERN int omxcam_video_start (
    omxcam_video_settings_t* settings,
    uint32_t ms);
/*
 * Stops the video capture.
 */
OMXCAM_EXTERN int omxcam_video_stop ();

#ifdef __cplusplus
}
#endif

#endif