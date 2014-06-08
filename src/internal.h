#ifndef OMXCAM_INTERNAL_H
#define OMXCAM_INTERNAL_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include <bcm_host.h>
#include <interface/vmcs_host/vc_vchi_gencmd.h>

#define omxcam__omx_struct_init(x)                                             \
  memset (&(x), 0, sizeof (x));                                                \
  (x).nSize = sizeof (x);                                                      \
  (x).nVersion.nVersion = OMX_VERSION;                                         \
  (x).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;                            \
  (x).nVersion.s.nVersionMinor = OMX_VERSION_MINOR;                            \
  (x).nVersion.s.nRevision = OMX_VERSION_REVISION;                             \
  (x).nVersion.s.nStep = OMX_VERSION_STEP

#define OMXCAM_CAMERA_NAME "OMX.broadcom.camera"
#define OMXCAM_IMAGE_ENCODE_NAME "OMX.broadcom.image_encode"
#define OMXCAM_VIDEO_ENCODE_NAME "OMX.broadcom.video_encode"
#define OMXCAM_NULL_SINK_NAME "OMX.broadcom.null_sink"

#define OMXCAM_MIN_GPU_MEM 128 //MB

#ifdef OMXCAM_DEBUG
#define omxcam__error(message, ...)                                            \
  omxcam__error_(message, __func__, __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define omxcam__error(message, ...) //Empty
#endif

/*
 * Component's event flags.
 */
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  uint32_t flags;
  int error;
} omxcam__event_t;

/*
 * Wrapper for an OpenMAX IL component.
 */
typedef struct {
  OMX_HANDLETYPE handle;
  omxcam__event_t event;
  OMX_STRING name;
  void (*buffer_callback)(uint8_t* buffer, uint32_t length);
} omxcam__component_t;

/*
 * Program's global context.
 */
typedef struct {
  omxcam__component_t camera;
  omxcam__component_t image_encode;
  omxcam__component_t video_encode;
  omxcam__component_t null_sink;
  OMX_BUFFERHEADERTYPE* output_buffer;
} omxcam__context_t;

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
} omxcam__event;

typedef enum {
  OMXCAM_STATE_LOADED = OMX_StateLoaded,
  OMXCAM_STATE_IDLE = OMX_StateIdle,
  OMXCAM_STATE_EXECUTING = OMX_StateExecuting,
  OMXCAM_STATE_PAUSE = OMX_StatePause,
  OMXCAM_STATE_WAIT_FOR_RESOURCES = OMX_StateWaitForResources,
  OMXCAM_STATE_INVALID = OMX_StateInvalid
} omxcam__state;

//Context's global variable
omxcam__context_t omxcam__ctx;

/*
 * Prints an error message to the stdout along with the file, line and function
 * name from where this function is called. It is printed if the cflag
 * OMXCAM_DEBUG is enabled. Use the omxcam__error() macro instead.
 */
void omxcam__error_ (
    const char* fmt,
    const char* fn,
    const char* file,
    int line,
    ...);

/*
 * Sets the last error.
 */
void omxcam__set_last_error (omxcam_errno error);

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
const char* omxcam__dump_OMX_COLOR_FORMATTYPE (OMX_COLOR_FORMATTYPE color);
const char* omxcam__dump_OMX_OTHER_FORMATTYPE (OMX_OTHER_FORMATTYPE format);
const char* omxcam__dump_OMX_AUDIO_CODINGTYPE (OMX_AUDIO_CODINGTYPE coding);
const char* omxcam__dump_OMX_VIDEO_CODINGTYPE (OMX_VIDEO_CODINGTYPE coding);
const char* omxcam__dump_OMX_IMAGE_CODINGTYPE (OMX_IMAGE_CODINGTYPE coding);
const char* omxcam__dump_OMX_STATETYPE (OMX_STATETYPE state);
const char* omxcam__dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);
const char* omxcam__dump_OMX_EVENTTYPE (OMX_EVENTTYPE event);
const char* omxcam__dump_OMX_INDEXTYPE (OMX_INDEXTYPE type);
void omxcam__dump_OMX_PARAM_PORTDEFINITIONTYPE (
    OMX_PARAM_PORTDEFINITIONTYPE* port);
void omxcam__dump_OMX_IMAGE_PARAM_PORTFORMATTYPE (
    OMX_IMAGE_PARAM_PORTFORMATTYPE* port);
void omxcam__dump_OMX_BUFFERHEADERTYPE (OMX_BUFFERHEADERTYPE* header);

/*
 * Prints a debug message to the stdout. It is printed if the cflag OMXCAM_DEBUG
 * is enabled. 
 */
void omxcam__trace (const char* fmt, ...);

/*
 * Sets an error originated from the EventHandler.
 */
void omxcam__event_error (omxcam__component_t* component);
/*
 * Creates the event flags handler for the component.
 */
int omxcam__event_create (omxcam__component_t* component);
/*
 * Destroys the event flags handler for the component.
 */
int omxcam__event_destroy (omxcam__component_t* component);
/*
 * Sets some events.
 * 
 * Unlocks the current thread if there are events waiting to the given events.
 */
int omxcam__event_wake (omxcam__component_t* component, omxcam__event event);
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
int omxcam__event_wait (
    omxcam__component_t* component,
    omxcam__event events,
    omxcam__event* current_events);

/*
 * Enables and disables the port of a component.
 */
int omxcam__component_port_enable (
    omxcam__component_t* component,
    uint32_t port);
int omxcam__component_port_disable (
    omxcam__component_t* component,
    uint32_t port);

/*
 * Initializes a component. All its ports are enabled and the OpenMAX IL event
 * handlers are configured, plus other things.
 */
int omxcam__component_init (omxcam__component_t* component);
/*
 * Deinitializes a component. All its ports are disabled, plus other things.
 */
int omxcam__component_deinit (omxcam__component_t* component);

/*
 * Changes the state of a component.
 */
int omxcam__component_change_state (
    omxcam__component_t* component,
    omxcam__state state);

/*
 * Allocates and frees an OpenMAX buffer for the given port.
 */
int omxcam__buffer_alloc (omxcam__component_t* component, uint32_t port);
int omxcam__buffer_free (omxcam__component_t* component, uint32_t port);

/*
 * Initializes and deinitializes OpenMAX IL. They must be the first and last
 * api calls.
 */
int omxcam__init ();
int omxcam__deinit ();

/*
 * Loads the camera drivers. After this call the red led is turned on and the
 * OpenMAX IL layer is ready to be configured.
 */
int omxcam__camera_load_drivers ();

/*
 * Checks if the camera is ready to be used. It checks the available gpu memory
 * and whether it is supported and detected.
 */
int omxcam__camera_check ();

/*
 * The camera needs to know which output port is going to be used to consume the
 * data. The capture port must be enabled just before the buffers are going to
 * be consumed, that is, when the state of the camera and all the enabled
 * components are OMX_StateExecuting.
 * 
 * video = 71
 * still = 72
 */
int omxcam__camera_capture_port_set (uint32_t port);
int omxcam__camera_capture_port_reset (uint32_t port);

/*
 * Sets the default settings for the camera.
 */
void omxcam__camera_init (
    omxcam_camera_settings_t* settings,
    uint32_t width,
    uint32_t height);

/*
 * Configures the OpenMAX IL camera component.
 */
int omxcam__camera_configure_omx (
    omxcam_camera_settings_t* settings,
    int video);

/*
 * Sets the default settings for the jpeg encoder.
 */
void omxcam__jpeg_init (omxcam_jpeg_settings_t* settings);
/*
 * Adds an exif tag to the jpeg metadata.
 */
int omxcam__jpeg_add_tag (char* key, char* value);
/*
 * Configures the OpenMAX IL image_encode component with the jpeg settings.
 */
int omxcam__jpeg_configure_omx (omxcam_jpeg_settings_t* settings);

/*
 * Sets the default settings for the h264 encoder.
 */
void omxcam__h264_init (omxcam_h264_settings_t* settings);
/*
 * Configures the OpenMAX IL video_encode component with the h264 settings.
 */
int omxcam__h264_configure_omx (omxcam_h264_settings_t* settings);

#endif