#include "imageLoader.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Original source: https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
PNGImage loadPNGFile(std::string fileName)
{
	std::vector<unsigned char> png;
	std::vector<unsigned char> pixels; //the raw pixels
	unsigned int width{}, height{};

	std::cout << "Loading texture: " << fileName << std::endl;

	//load and decode
	unsigned error = lodepng::load_file(png, fileName);
	if (!error) error = lodepng::decode(pixels, width, height, png);

	//if there's an error, display it
	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

	// Unfortunately, images usually have their origin at the top left.
	// OpenGL instead defines the origin to be on the _bottom_ left instead, so
	// here's the world's most inefficient way to flip the image vertically.

	// You're welcome :)

	unsigned int widthBytes = 4 * width;

	for (unsigned int row = 0; row < (height / 2); row++) {
		for (unsigned int col = 0; col < widthBytes; col++) {
			std::swap(pixels[row * widthBytes + col], pixels[(height - 1 - row) * widthBytes + col]);
		}
	}

	PNGImage image;
	image.width = width;
	image.height = height;
	image.pixels = pixels;

	return image;
}

JPGImage loadJPGFile(std::string fileName)
{
	std::cout << "Loading texture: " << fileName << std::endl;

	int width = 0;
	int height = 0;
	int channelsInFile = 0;
	// Request 4 channels so layout matches PNG / glTexImage2D(GL_RGBA, ...)
	unsigned char* data = stbi_load(fileName.c_str(), &width, &height, &channelsInFile, STBI_rgb_alpha);

	JPGImage image{};
	if (!data) {
		std::cout << "stb_image failed: " << fileName << " — " << stbi_failure_reason() << std::endl;
		return image;
	}

	if (width <= 0 || height <= 0) {
		stbi_image_free(data);
		return image;
	}

	image.width = static_cast<unsigned int>(width);
	image.height = static_cast<unsigned int>(height);
	const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
	image.pixels.assign(data, data + byteCount);
	stbi_image_free(data);

	unsigned int widthBytes = 4 * image.width;
	for (unsigned int row = 0; row < (image.height / 2); row++) {
		for (unsigned int col = 0; col < widthBytes; col++) {
			std::swap(image.pixels[row * widthBytes + col], image.pixels[(image.height - 1 - row) * widthBytes + col]);
		}
	}

	return image;
}
