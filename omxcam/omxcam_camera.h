#ifndef OMXCAM_CAMERA_H
#define OMXCAM_CAMERA_H

#include "omxcam_omx.h"
#include "omxcam_errors.h"
#include "omxcam_core.h"

typedef enum {
  OMXCAM_ExposureOff = OMX_ExposureControlOff,
  OMXCAM_ExposureAuto = OMX_ExposureControlAuto,
  OMXCAM_ExposureNight = OMX_ExposureControlNight,
  OMXCAM_ExposureBackLight = OMX_ExposureControlBackLight,
  OMXCAM_ExposureSpotlight = OMX_ExposureControlSpotLight,
  OMXCAM_ExposureSports = OMX_ExposureControlSports,
  OMXCAM_ExposureSnow = OMX_ExposureControlSnow,
  OMXCAM_ExposureBeach = OMX_ExposureControlBeach,
  OMXCAM_ExposureLargeAperture = OMX_ExposureControlLargeAperture,
  OMXCAM_ExposureSmallAperture = OMX_ExposureControlSmallAperture,
  OMXCAM_ExposureVeryLong = OMX_ExposureControlVeryLong,
  OMXCAM_ExposureFixedFps = OMX_ExposureControlFixedFps,
  OMXCAM_ExposureNightWithPreview = OMX_ExposureControlNightWithPreview,
  OMXCAM_ExposureAntishake = OMX_ExposureControlAntishake,
  OMXCAM_ExposureFireworks = OMX_ExposureControlFireworks,
} OMXCAM_EXPOSURE;

typedef enum {
  OMXCAM_ImageFilterNone = OMX_ImageFilterNone,
  OMXCAM_ImageFilterNoise = OMX_ImageFilterNoise,
  OMXCAM_ImageFilterEmboss = OMX_ImageFilterEmboss,
  OMXCAM_ImageFilterNegative = OMX_ImageFilterNegative,
  OMXCAM_ImageFilterSketch = OMX_ImageFilterSketch,
  OMXCAM_ImageFilterOilPaint = OMX_ImageFilterOilPaint,
  OMXCAM_ImageFilterHatch = OMX_ImageFilterHatch,
  OMXCAM_ImageFilterGpen = OMX_ImageFilterGpen,
  OMXCAM_ImageFilterAntialias = OMX_ImageFilterAntialias,
  OMXCAM_ImageFilterDeRing = OMX_ImageFilterDeRing,
  OMXCAM_ImageFilterSolarize = OMX_ImageFilterSolarize,
  OMXCAM_ImageFilterWatercolor = OMX_ImageFilterWatercolor,
  OMXCAM_ImageFilterPastel = OMX_ImageFilterPastel,
  OMXCAM_ImageFilterSharpen = OMX_ImageFilterSharpen,
  OMXCAM_ImageFilterFilm = OMX_ImageFilterFilm,
  OMXCAM_ImageFilterBlur = OMX_ImageFilterBlur,
  OMXCAM_ImageFilterSaturation = OMX_ImageFilterSaturation,
  OMXCAM_ImageFilterDeInterlaceLineDouble =
      OMX_ImageFilterDeInterlaceLineDouble,
  OMXCAM_ImageFilterDeInterlaceAdvanced = OMX_ImageFilterDeInterlaceAdvanced,
  OMXCAM_ImageFilterColourSwap = OMX_ImageFilterColourSwap,
  OMXCAM_ImageFilterWashedOut = OMX_ImageFilterWashedOut,
  OMXCAM_ImageFilterColourPoint = OMX_ImageFilterColourPoint,
  OMXCAM_ImageFilterPosterise = OMX_ImageFilterPosterise,
  OMXCAM_ImageFilterColourBalance = OMX_ImageFilterColourBalance,
  OMXCAM_ImageFilterCartoon = OMX_ImageFilterCartoon,
} OMXCAM_IMAGE_FILTER;

typedef enum {
  OMXCAM_MeteringAverage = OMX_MeteringModeAverage,
  OMXCAM_MeteringSpot = OMX_MeteringModeSpot,
  OMXCAM_MeteringMatrix = OMX_MeteringModeMatrix,
} OMXCAM_METERING;

typedef enum {
  OMXCAM_MirrorNone = OMX_MirrorNone,
  OMXCAM_MirrorHorizontal = OMX_MirrorHorizontal,
  OMXCAM_MirrorVertical = OMX_MirrorVertical,
  OMXCAM_MirrorBoth = OMX_MirrorBoth,
} OMXCAM_MIRROR;

typedef enum {
  OMXCAM_RotationNone = 0,
  OMXCAM_Rotation90 = 90,
  OMXCAM_Rotation180 = 180,
  OMXCAM_Rotation270 = 270,
} OMXCAM_ROTATION;

typedef enum {
  OMXCAM_WhiteBalanceOff = OMX_WhiteBalControlOff,
  OMXCAM_WhiteBalanceAuto = OMX_WhiteBalControlAuto,
  OMXCAM_WhiteBalanceSunLight = OMX_WhiteBalControlSunLight,
  OMXCAM_WhiteBalanceCloudy = OMX_WhiteBalControlCloudy,
  OMXCAM_WhiteBalanceShade = OMX_WhiteBalControlShade,
  OMXCAM_WhiteBalanceTungsten = OMX_WhiteBalControlTungsten,
  OMXCAM_WhiteBalanceFluorescent = OMX_WhiteBalControlFluorescent,
  OMXCAM_WhiteBalanceIncandescent = OMX_WhiteBalControlIncandescent,
  OMXCAM_WhiteBalanceFlash = OMX_WhiteBalControlFlash,
  OMXCAM_WhiteBalanceHorizon = OMX_WhiteBalControlHorizon,
} OMXCAM_WHITE_BALANCE;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t sharpness;
  uint32_t contrast;
  uint32_t brightness;
  uint32_t saturation;
  OMXCAM_BOOL shutterSpeedAuto;
  uint32_t shutterSpeed;
  OMXCAM_BOOL isoAuto;
  uint32_t iso;
  OMXCAM_EXPOSURE exposure;
  int32_t exposureCompensation;
  OMXCAM_MIRROR mirror;
  OMXCAM_ROTATION rotation;
  OMXCAM_BOOL colorEnable;
  uint32_t colorU;
  uint32_t colorV;
  OMXCAM_BOOL noiseReduction;
  OMXCAM_BOOL frameStabilisation;
  OMXCAM_METERING metering;
  OMXCAM_WHITE_BALANCE whiteBalance;
  OMXCAM_IMAGE_FILTER imageFilter;
  //Used only in video mode
  uint32_t framerate;
} OMXCAM_CAMERA_SETTINGS;

int OMXCAM_loadCameraDrivers ();
int OMXCAM_setCapturePort (OMX_U32 port);
int OMXCAM_resetCapturePort (OMX_U32 port);
void OMXCAM_initCameraSettings (
    uint32_t width,
    uint32_t height,
    OMXCAM_CAMERA_SETTINGS* settings);
int OMXCAM_setCameraSettings (OMXCAM_CAMERA_SETTINGS* settings);

#endif