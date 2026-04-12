#pragma once

#include "lodepng.h"
#include <vector>
#include <string>

typedef struct PNGImage {
	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> pixels;
} PNGImage;

typedef struct JPGImage {
	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> pixels;
} JPGImage;

PNGImage loadPNGFile(std::string fileName);
JPGImage loadJPGFile(std::string fileName);
