#pragma once
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"


static void ReadEXR(float **out_rgba, unsigned int *width, unsigned int *height, const char *filename, const char **err)
{
    int w;
    int h;
    LoadEXR(out_rgba, &w, &h, filename, err);
    *width = w;
    *height = h;
}

static void WriteEXR(const float *data, const int width, const int height,
                   const int components, const int save_as_fp16,
                   const char *filename, const char **err)
{
    SaveEXR(data, width, height, components, save_as_fp16, filename, err);
}
