#ifndef OMXCAM_DUMP_H
#define OMXCAM_DUMP_H

#include <stdio.h>
#include <string.h>

#include "omxcam_omx.h"

const char* OMXCAM_dump_OMX_COLOR_FORMATTYPE (OMX_COLOR_FORMATTYPE colorFormat);
const char* OMXCAM_dump_OMX_OTHER_FORMATTYPE (OMX_OTHER_FORMATTYPE otherFormat);
const char* OMXCAM_dump_OMX_AUDIO_CODINGTYPE (OMX_AUDIO_CODINGTYPE audioCoding);
const char* OMXCAM_dump_OMX_VIDEO_CODINGTYPE (OMX_VIDEO_CODINGTYPE videoCoding);
void OMXCAM_dump_OMX_PARAM_PORTDEFINITIONTYPE (
    OMX_PARAM_PORTDEFINITIONTYPE* portDefinition);
void OMXCAM_dump_OMX_IMAGE_PARAM_PORTFORMATTYPE (
    OMX_IMAGE_PARAM_PORTFORMATTYPE* imagePortFormat);
const char* OMXCAM_dump_OMX_STATETYPE (OMX_STATETYPE state);
const char* OMXCAM_dump_OMX_ERRORTYPE (OMX_ERRORTYPE error);
const char* OMXCAM_dump_OMX_EVENTTYPE (OMX_EVENTTYPE event);
void OMXCAM_dump_OMX_BUFFERHEADERTYPE (OMX_BUFFERHEADERTYPE* header);

#endif