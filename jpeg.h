#ifndef __jpeg_h__
#define __jpeg_h__

#include <vector>

void read_jpeg_header(const char *filename, unsigned& width, unsigned& height);
void read_jpeg(const char *filename, std::vector<unsigned char>& image, unsigned width, unsigned height);
void write_jpeg(char *filename, unsigned char *image, int width, int height);

#endif //__jpeg_h__
