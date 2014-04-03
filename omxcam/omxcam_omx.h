#ifndef OMXCAM_OMX_H
#define OMXCAM_OMX_H

#include <bcm_host.h>
#include <interface/vcos/vcos.h>
#include <IL/OMX_Broadcom.h>

#define OMX_INIT_STRUCTURE(a) \
  memset (&(a), 0, sizeof (a)); \
  (a).nSize = sizeof (a); \
  (a).nVersion.nVersion = OMX_VERSION; \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

#endif