#ifndef BENCH_H
#define BENCH_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "omxcam.h"

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t frames;
  uint32_t ms;
  void (*on_ready)();
  void (*on_stop)();
} bench_t;

int h264 (bench_t* req);
int h264_npt (bench_t* req);
int jpeg (bench_t* req);

int rgb_video (bench_t* req);
int rgb_still (bench_t* req);

int yuv_video (bench_t* req);
int yuv_video_npt (bench_t* req);
int yuv_still (bench_t* req);

#endif