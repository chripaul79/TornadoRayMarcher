#pragma once

#include "mesh.h"
#include "imageLoader.hpp"

unsigned int generateBuffer(Mesh &mesh);

unsigned int generateTextureBuffer(PNGImage image);
unsigned int generateTextureBuffer(JPGImage image);