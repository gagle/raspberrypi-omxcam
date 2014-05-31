#include "omxcam.h"

void omxcam_jpeg_init (omxcam_jpeg_settings_t* settings){
  settings->quality = 75;
  settings->exif_enable = OMXCAM_TRUE;
  settings->exif_tags = 0;
  settings->exif_valid_tags = 0;
  settings->ijg = OMXCAM_FALSE;
  settings->thumbnail_enable = OMXCAM_TRUE;
  settings->thumbnail_width = 64;
  settings->thumbnail_height = 48;
  settings->raw_bayer_enable = OMXCAM_FALSE;
}

int omxcam_jpeg_add_tag (char* key, char* value){
  int key_length = strlen (key);
  int value_length = strlen (value);
  
  struct {
    //These two fields need to be together
    OMX_CONFIG_METADATAITEMTYPE metadata_st;
    char metadata_padding[value_length];
  } item;
  
  OMXCAM_INIT_STRUCTURE (item.metadata_st);
  item.metadata_st.nSize = sizeof (item);
  item.metadata_st.eScopeMode = OMX_MetadataScopePortLevel;
  item.metadata_st.nScopeSpecifier = 341;
  item.metadata_st.eKeyCharset = OMX_MetadataCharsetASCII;
  item.metadata_st.nKeySizeUsed = key_length;
  memcpy (item.metadata_st.nKey, key, key_length);
  item.metadata_st.eValueCharset = OMX_MetadataCharsetASCII;
  item.metadata_st.nValueMaxSize = sizeof (item.metadata_padding);
  item.metadata_st.nValueSizeUsed = value_length;
  memcpy (item.metadata_st.nValue, value, value_length);
  
  OMX_ERRORTYPE error;
  
  if ((error = OMX_SetConfig (omxcam_ctx.image_encode.handle,
      OMX_IndexConfigMetadataItem, &item))){
    omxcam_error ("OMX_SetConfig - OMX_IndexConfigMetadataItem: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  return 0;
}

int omxcam_jpeg_configure_omx (omxcam_jpeg_settings_t* settings){
  omxcam_trace ("configuring '%s' settings", omxcam_ctx.image_encode.name);

  OMX_ERRORTYPE error;
  
  //Quality
  OMX_IMAGE_PARAM_QFACTORTYPE quality_st;
  OMXCAM_INIT_STRUCTURE (quality_st);
  quality_st.nPortIndex = 341;
  quality_st.nQFactor = settings->quality;
  if ((error = OMX_SetParameter (omxcam_ctx.image_encode.handle,
      OMX_IndexParamQFactor, &quality_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamQFactor: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Disable EXIF tags
  OMX_CONFIG_BOOLEANTYPE exif_st;
  OMXCAM_INIT_STRUCTURE (exif_st);
  exif_st.bEnabled = settings->exif_enable ? OMXCAM_FALSE : OMXCAM_TRUE;
  if ((error = OMX_SetParameter (omxcam_ctx.image_encode.handle,
      OMX_IndexParamBrcmDisableEXIF, &exif_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamBrcmDisableEXIF: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  //Bayer data
  if (settings->raw_bayer_enable){
    //The filename is not relevant
    char dummy[] = "dummy";
    struct {
      //These two fields need to be together
      OMX_PARAM_CONTENTURITYPE uri_st;
      char padding[5];
    } raw;
    OMXCAM_INIT_STRUCTURE (raw.uri_st);
    raw.uri_st.nSize = sizeof (raw);
    memcpy (raw.uri_st.contentURI, dummy, 5);
    if ((error = OMX_SetConfig (omxcam_ctx.camera.handle,
        OMX_IndexConfigCaptureRawImageURI, &raw))){
      omxcam_error ("OMX_SetConfig - OMX_IndexConfigCaptureRawImageURI: %s",
          omxcam_dump_OMX_ERRORTYPE (error));
      return -1;
    }
  }
  
  //Enable IJG table
  OMX_PARAM_IJGSCALINGTYPE ijg_st;
  OMXCAM_INIT_STRUCTURE (ijg_st);
  ijg_st.nPortIndex = 341;
  ijg_st.bEnabled = settings->ijg;
  if ((error = OMX_SetParameter (omxcam_ctx.image_encode.handle,
      OMX_IndexParamBrcmEnableIJGTableScaling, &ijg_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamBrcmEnableIJGTableScaling: "
        "%s", omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }
  
  if (!settings->exif_enable) return 0;
  
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
  
  if (omxcam_jpeg_add_tag ("EXIF.DateTimeOriginal", timestamp)) return -1;
  if (omxcam_jpeg_add_tag ("EXIF.DateTimeDigitized", timestamp)) return -1;
  if (omxcam_jpeg_add_tag ("IFD0.DateTime", timestamp)) return -1;
  
  uint32_t i;
  for (i=0; i<settings->exif_valid_tags; i++){
    if (omxcam_jpeg_add_tag (settings->exif_tags[i].key,
        settings->exif_tags[i].value)){
      return -1;
    }
  }
  
  //Thumbnail
  OMX_PARAM_BRCMTHUMBNAILTYPE thumbnail_st;
  OMXCAM_INIT_STRUCTURE (thumbnail_st);
  thumbnail_st.bEnable = settings->thumbnail_enable;
  thumbnail_st.bUsePreview = OMX_FALSE;
  thumbnail_st.nWidth = settings->thumbnail_width;
  thumbnail_st.nHeight = settings->thumbnail_height;
  if ((error = OMX_SetParameter (omxcam_ctx.image_encode.handle,
      OMX_IndexParamBrcmThumbnail, &thumbnail_st))){
    omxcam_error ("OMX_SetParameter - OMX_IndexParamBrcmThumbnail: %s",
        omxcam_dump_OMX_ERRORTYPE (error));
    return -1;
  }

  return 0;
}