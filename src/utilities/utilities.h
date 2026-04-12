#include "FastNoiseLite.h"
#include <cstdint>
#include <glad/glad.h>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

GLuint uploadCloudNoiseTexture(const std::vector<float>& data, int width, int height, int depth);

std::vector<float> generateCloudData(int width, int height, int depth, int seed, float scale);
std::vector<float> generateTerrainNoiseData2D(int size);
GLuint uploadTerrainNoiseTexture2D(const std::vector<float>& data, int size);
std::vector<float> generateBlueNoiseData2D(int width, int height, float minDist, std::uint32_t seed);
GLuint uploadBlueNoiseTexture2D(const std::vector<float>& data, int width, int height);

GLuint loadCubemap(const std::vector<std::filesystem::path>& faces);

void saveNoiseSlice(const std::vector<float>& data, int size, int sliceZ, const char* filename);
void saveNoiseSlice2D(const std::vector<float>& data, int size, const char* filename);
void saveNoiseImage2D01(const std::vector<float>& data, int width, int height, const char* filename);