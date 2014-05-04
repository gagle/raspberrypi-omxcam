#include "omxcam_image_encode.h"

void OMXCAM_initJpegSettings (OMXCAM_JPEG_SETTINGS* settings){
  settings->quality = 75;
  settings->exifEnable = OMXCAM_TRUE;
  settings->exifTags = 0;
  settings->exifValidTags = 0;
  settings->ijg = OMXCAM_FALSE;
  settings->thumbnailEnable = OMXCAM_TRUE;
  settings->thumbnailWidth = 64;
  settings->thumbnailHeight = 48;
  settings->rawBayer = OMXCAM_FALSE;
}

int OMXCAM_addTag (char* key, char* value){
  int keyLength = strlen (key);
  int valueLength = strlen (value);
  
  struct {
    //These two fields need to be together
    OMX_CONFIG_METADATAITEMTYPE metadata;
    char metadataSpace[valueLength];
  } item;
  
  OMX_INIT_STRUCTURE (item.metadata);
  item.metadata.nSize = sizeof (item);
  item.metadata.eScopeMode = OMX_MetadataScopePortLevel;
  item.metadata.nScopeSpecifier = 341;
  item.metadata.eKeyCharset = OMX_MetadataCharsetASCII;
  item.metadata.nKeySizeUsed = keyLength;
  memcpy (item.metadata.nKey, key, keyLength);
  item.metadata.eValueCharset = OMX_MetadataCharsetASCII;
  item.metadata.nValueMaxSize = sizeof (item.metadataSpace);
  item.metadata.nValueSizeUsed = valueLength;
  memcpy (item.metadata.nValue, value, valueLength);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SetConfig (OMXCAM_ctx.image_encode.handle,
      OMX_IndexConfigMetadataItem, &item))){
    OMXCAM_error ("OMX_SetConfig - OMX_IndexConfigMetadataItem: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int OMXCAM_setJpegSettings (OMXCAM_JPEG_SETTINGS* settings){
  OMXCAM_trace ("Configuring '%s' settings", OMXCAM_ctx.image_encode.name);

  OMX_ERRORTYPE error;
  
  //Quality
  OMX_IMAGE_PARAM_QFACTORTYPE quality;
  OMX_INIT_STRUCTURE (quality);
  quality.nPortIndex = 341;
  quality.nQFactor = settings->quality;
  if ((error = OMX_SetParameter (OMXCAM_ctx.image_encode.handle,
      OMX_IndexParamQFactor, &quality))){
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamQFactor: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Disable EXIF tags
  OMX_CONFIG_BOOLEANTYPE exif;
  OMX_INIT_STRUCTURE (exif);
  exif.bEnabled = settings->exifEnable ? OMXCAM_FALSE : OMXCAM_TRUE;
  if ((error = OMX_SetParameter (OMXCAM_ctx.image_encode.handle,
      OMX_IndexParamBrcmDisableEXIF, &exif))){
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamBrcmDisableEXIF: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Bayer data
  if (settings->rawBayer){
    char dummy[] = "dummy";
    struct {
      //These two fields need to be together
      OMX_PARAM_CONTENTURITYPE uri;
      char space[5];
    } raw;
    OMX_INIT_STRUCTURE (raw.uri);
    raw.uri.nSize = sizeof (raw);
    memcpy (raw.uri.contentURI, dummy, 5);
    if ((error = OMX_SetConfig (OMXCAM_ctx.camera.handle,
        OMX_IndexConfigCaptureRawImageURI, &raw))){
      OMXCAM_error ("OMX_SetConfig - OMX_IndexConfigCaptureRawImageURI: %s",
          OMXCAM_dump_OMX_ERRORTYPE (error));
      return -1;
    }
  }
  
  //Enable IJG table
  OMX_PARAM_IJGSCALINGTYPE ijg;
  OMX_INIT_STRUCTURE (ijg);
  ijg.nPortIndex = 341;
  ijg.bEnabled = settings->ijg;
  if ((error = OMX_SetParameter (OMXCAM_ctx.image_encode.handle,
      OMX_IndexParamBrcmEnableIJGTableScaling, &ijg))){
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamBrcmEnableIJGTableScaling: "
        "%s", OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  if (!settings->exifEnable) return 0;
  
  //EXIF tags
  //See firmware/documentation/ilcomponents/image_decode.html for valid keys
  //See http://www.media.mit.edu/pia/Research/deepview/exif.html#IFD0Tags
  //for valid keys and their description
  char timestamp[20];
  time_t now;
  struct tm ts;
  time (&now);
  ts = *localtime (&now);
  strftime (timestamp, sizeof (timestamp), "%Y:%m:%d %H:%M:%S", &ts);
  
  if (OMXCAM_addTag ("EXIF.DateTimeOriginal", timestamp)) return -1;
  if (OMXCAM_addTag ("EXIF.DateTimeDigitized", timestamp)) return -1;
  if (OMXCAM_addTag ("IFD0.DateTime", timestamp)) return -1;
  
  int i;
  for (i=0; i<settings->exifValidTags; i++){
    if (OMXCAM_addTag (settings->exifTags[i].key, settings->exifTags[i].value)){
      return -1;
    }
  }
  
  //Thumbnail
  OMX_PARAM_BRCMTHUMBNAILTYPE thumbnail;
  OMX_INIT_STRUCTURE (thumbnail);
  thumbnail.bEnable = settings->thumbnailEnable;
  thumbnail.bUsePreview = OMX_FALSE;
  thumbnail.nWidth = settings->thumbnailWidth;
  thumbnail.nHeight = settings->thumbnailHeight;
  if ((error = OMX_SetParameter (OMXCAM_ctx.image_encode.handle,
      OMX_IndexParamBrcmThumbnail, &thumbnail))){
    OMXCAM_error ("OMX_SetParameter - OMX_IndexParamBrcmThumbnail: %s",
        OMXCAM_dump_OMX_ERRORTYPE (error));
    return -1;
  }

  return 0;
}