#include "utilities.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "pds_Bridson.hpp"

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace {
std::string resolveNoiseOutputPath(const char* filename)
{
    namespace fs = std::filesystem;

    fs::path input = filename ? fs::u8path(filename) : fs::u8path("noise.png");
    if (input.is_absolute() || input.has_parent_path()) {
        if (input.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(input.parent_path(), ec);
        }
        return input.string();
    }

    const fs::path outDir = fs::u8path(PROJECT_ROOT) / "res" / "textures" / "generated";
    std::error_code ec;
    fs::create_directories(outDir, ec);
    return (outDir / input).string();
}
} // namespace

static inline float remap(float val,
    float inMin, float inMax,
    float outMin, float outMax)
{
    float t = (val - inMin) / (inMax - inMin + 1e-6f);
    t = std::max(0.0f, std::min(1.0f, t));
    return outMin + t * (outMax - outMin);
}

std::vector<float> generateCloudData(
    int   width = 128,
    int   height = 128,
    int   depth = 128,
    int   seed = 1337,
    float scale = 0.025f)
{
    // Low-frequency FBm – large-scale structure for volumetric / SDF noise
    FastNoiseLite perlin;
    perlin.SetSeed(seed);
    perlin.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    perlin.SetFractalType(FastNoiseLite::FractalType_FBm);
    perlin.SetFractalOctaves(4);
    perlin.SetFractalLacunarity(2.0f);
    perlin.SetFractalGain(0.5f);
    perlin.SetFrequency(scale);

    // Mid-frequency Worley – cell centres will be bright, edges dark
    FastNoiseLite worleyMid;
    worleyMid.SetSeed(seed + 1);
    worleyMid.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    worleyMid.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    worleyMid.SetCellularJitter(1.0f);
    worleyMid.SetFractalType(FastNoiseLite::FractalType_FBm);
    worleyMid.SetFractalOctaves(3);
    worleyMid.SetFractalLacunarity(2.0f);
    worleyMid.SetFractalGain(0.5f);
    worleyMid.SetFrequency(scale * 3.0f);

    // High-frequency Worley – fine detail
    FastNoiseLite worleyFine;
    worleyFine.SetSeed(seed + 2);
    worleyFine.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    worleyFine.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
    worleyFine.SetCellularJitter(1.0f);
    worleyFine.SetFractalType(FastNoiseLite::FractalType_FBm);
    worleyFine.SetFractalOctaves(2);
    worleyFine.SetFractalLacunarity(2.0f);
    worleyFine.SetFractalGain(0.5f);
    worleyFine.SetFrequency(scale * 8.0f);

    std::vector<float> volume(
        static_cast<size_t>(width) * height * depth, 0.0f);

    // Pass 1 — fill volume, fully parallel, no reduction needed
#pragma omp parallel for collapse(3) schedule(static)
    for (int z = 0; z < depth; ++z)
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                float fx = (static_cast<float>(x) / width) * 6.28318f;
                float fy = (static_cast<float>(y) / height) * 6.28318f;
                float fz = (static_cast<float>(z) / depth) * 6.28318f;

                float s1x = std::cos(fx), s1y = std::sin(fx);
                float s2x = std::cos(fy), s2y = std::sin(fy);
                float s3x = std::cos(fz), s3y = std::sin(fz);

                float n1 = perlin.GetNoise(s1x * 50.0f, s1y * 50.0f, s2x * 50.0f);
                float n2 = perlin.GetNoise(s2y * 50.0f, s3x * 50.0f, s3y * 50.0f);
                float p = ((n1 + n2) * 0.5f + 1.0f) * 0.5f;

                float wMid = 1.0f - std::max(0.0f, std::min(1.0f,
                    worleyMid.GetNoise(s1x * 100.0f, s1y * 100.0f, s2x * 100.0f)));
                float wFine = 1.0f - std::max(0.0f, std::min(1.0f,
                    worleyFine.GetNoise(s3x * 200.0f, s3y * 200.0f, s2y * 200.0f)));

                float base = remap(p, 1.0f - wMid * 1.4f, 1.0f, 0.0f, 1.0f);
                float n = base * 0.60f + wMid * 0.25f + wFine * 0.15f;

                volume[x + width * (y + height * z)] = n;
            }

        // Pass 2 — find min/max without any OpenMP reduction
        auto [itMin, itMax] = std::minmax_element(volume.begin(), volume.end());
        const float minVal = *itMin;
        const float maxVal = *itMax;
        const float range = maxVal - minVal + 1e-6f;

        // Pass 3 — normalise, fully parallel
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < static_cast<int>(volume.size()); ++i)
        {
            float v = (volume[i] - minVal) / range;  // [0, 1]
            volume[i] = v * 2.0f - 1.0f;             // [-1, 1]
        }

    return volume;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Upload tornado noise as a GL_TEXTURE_3D (GL_R16F)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Format notes
//  ─────────────
//  GL_R16F  (16-bit float, ~3.3 decimal digits of precision) eliminates the
//  visible quantisation "banding" that GL_R8 produces in the volume shader.
//  GL_R32F is overkill and doubles the VRAM cost for no visible benefit.
//
//  Filtering
//  ─────────
//  GL_LINEAR_MIPMAP_LINEAR (trilinear): required to avoid blocky artefacts
//  when the tornado SDF samples the texture at an oblique angle or when the
//  camera is close.  glGenerateMipmap does the mip-chain in one call.
//
//  Wrapping
//  ────────
//  GL_REPEAT on all three axes so the shader can tile the texture over a
//  larger world-space region without hitting the edge.
//
GLuint uploadCloudNoiseTexture(const std::vector<float>& data, int width, int height, int depth)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    glTexImage3D(
        GL_TEXTURE_3D,
        0,              // base mip level
        GL_R32F,        // internal format — 32-bit float, no banding
        width, height, depth,
        0,
        GL_RED,         // single channel
        GL_FLOAT,       // source data type (std::vector<float>)
        data.data()
    );

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Repeat so the volume shader can tile without edge clamping artefacts.
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_3D);

    glBindTexture(GL_TEXTURE_3D, 0);
    return tex;
}


// ─────────────────────────────────────────────────────────────────────────────
//  TERRAIN  —  2D height noise  (1024² single-channel R16F)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Problem with the previous setup
//  ────────────────────────────────
//  FastNoiseLite's default frequency is 0.01.  With raw texel coordinates
//  (0..1023), that gives  1024 × 0.01 ≈ 10.2 noise cycles across the texture.
//  The world span is  512 × 0.52 ≈ 266 units, so each hill is only ~26 units
//  wide.  With kTerrainMaxH = 13, the rise-over-run ≈ 0.50 → ~27° average
//  slope — steep, dramatic, unrealistic.
//
//  Fix
//  ────
//  Lower frequency: 0.003 → 1024 × 0.003 ≈ 3 base cycles → hills ~88 units
//  wide.  With kTerrainMaxH = 8 (see scenelogic.cpp note below), slopes drop
//  to ~10°.  Adding six FBm octaves with a conservative gain of 0.38 (instead
//  of the default 0.5) keeps high-frequency roughness subtle — you get gentle
//  rolling hills with fine texture, not jagged ridges.
//
//  ⚠  Also change in scenelogic.cpp:
//       constexpr float kTerrainMaxH = 8.0f;   // was 13.0f
//  The noise shape alone won't fully fix the drama; the height scale matters
//  equally.  8 units is much more natural for this world scale.
//
std::vector<float> generateTerrainNoiseData2D(int size)
{
    std::vector<float> data(static_cast<size_t>(size) * static_cast<size_t>(size));

    FastNoiseLite noise;
    noise.SetSeed(42);
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    //  ── Frequency ────────────────────────────────────────────────────────────
    //  0.003 → ~3 primary hill cycles across 1024 texels.
    //  World span ≈ 266 units → each primary hill ≈ 88 units wide.
    //  Increase toward 0.005 if you want more hills; decrease toward 0.002
    //  for an even flatter, more open landscape.
    noise.SetFrequency(0.003f);

    //  ── FBm octaves ──────────────────────────────────────────────────────────
    //  6 octaves: base shape + 5 detail layers.
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);

    //  ── Lacunarity ───────────────────────────────────────────────────────────
    //  2.0 is standard: each octave doubles the frequency.
    noise.SetFractalLacunarity(2.0f);

    //  ── Gain ─────────────────────────────────────────────────────────────────
    //  0.38 (vs default 0.5) means each successive octave contributes less.
    //  The large-scale shape dominates; fine detail is a gentle overlay rather
    //  than a competing force.  This is the main anti-drama knob.
    noise.SetFractalGain(0.38f);

    //  No weighted strength (0.0): keep the distribution symmetric so we get
    //  both rounded hills and shallow valleys, not spiky peaks.
    noise.SetFractalWeightedStrength(0.0f);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            //  Feed raw texel indices — frequency scaling handles the rest.
            const float val = noise.GetNoise(static_cast<float>(x), static_cast<float>(y));
            data[static_cast<size_t>(y) * static_cast<size_t>(size) + static_cast<size_t>(x)] = val;
        }
    }

    return data;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Upload terrain noise as GL_TEXTURE_2D (GL_R16F)
// ─────────────────────────────────────────────────────────────────────────────
GLuint uploadTerrainNoiseTexture2D(const std::vector<float>& data, int size)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R16F,
        size, size,
        0,
        GL_RED,
        GL_FLOAT,
        data.data()
    );

    // Trilinear for the terrain: avoids visible transitions when the CPU-side
    // terrain.cpp code samples between texels during mesh generation, and also
    // when (if) the terrain texture is used directly in the shader.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp terrain to border — no wrapping desired on a finite heightfield.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

std::vector<float> generateBlueNoiseData2D(int width, int height, float minDist, std::uint32_t seed)
{
    if (width <= 0 || height <= 0) return {};

    blue_noise::bridson_2d::config cfg;
    cfg.w = static_cast<float>(width);
    cfg.h = static_cast<float>(height);
    cfg.min_dist = std::max(0.001f, minDist);
    cfg.k_max_attempts = 30;

    std::mt19937 rng(seed);
    struct Point {
        float x;
        float y;
    };
    const auto points = blue_noise::bridson_2d::poisson_disc_sampling<Point>(cfg, rng);

    std::vector<float> data(static_cast<size_t>(width) * static_cast<size_t>(height), 0.0f);
    for (const auto& p : points) {
        const int ix = static_cast<int>(p.x);
        const int iy = static_cast<int>(p.y);
        if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
            data[static_cast<size_t>(iy) * static_cast<size_t>(width) + static_cast<size_t>(ix)] = 1.0f;
        }
    }

    return data;
}

GLuint uploadBlueNoiseTexture2D(const std::vector<float>& data, int width, int height)
{
    if (width <= 0 || height <= 0 || data.size() < static_cast<size_t>(width) * static_cast<size_t>(height)) {
        return 0;
    }

    std::vector<std::uint8_t> texelData(static_cast<size_t>(width) * static_cast<size_t>(height), 0u);
    for (size_t i = 0; i < texelData.size(); ++i) {
        const float v = std::clamp(data[i], 0.0f, 1.0f);
        texelData[i] = static_cast<std::uint8_t>(v * 255.0f + 0.5f);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, texelData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

// CubeMap Loader — open via std::filesystem::path so Unicode paths (e.g. non-ASCII folders) work on Windows.

namespace {

unsigned char* stbiLoadFilePath(const std::filesystem::path& path, int* w, int* h, int* comp)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return nullptr;
    const auto sz = file.tellg();
    if (sz <= 0)
        return nullptr;
    file.seekg(0);
    std::vector<unsigned char> buf(static_cast<size_t>(sz));
    if (!file.read(reinterpret_cast<char*>(buf.data()), sz))
        return nullptr;
    return stbi_load_from_memory(buf.data(), static_cast<int>(buf.size()), w, h, comp, 0);
}

} // namespace

GLuint loadCubemap(const std::vector<std::filesystem::path>& faces)
{
    if (faces.size() < 6) {
        std::cout << "loadCubemap: need 6 face paths, got " << faces.size() << std::endl;
        return 0;
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width = 0, height = 0, nrChannels = 0;
    bool ok = true;
    for (unsigned int i = 0; i < 6; i++) {
        unsigned char* data = stbiLoadFilePath(faces[i], &width, &height, &nrChannels);
        if (!data) {
            std::cout << "Cubemap failed: " << faces[i].string() << std::endl;
            ok = false;
            break;
        }
        GLenum internal = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    if (!ok) {
        glDeleteTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return 0;
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}


// ─────────────────────────────────────────────────────────────────────────────
//  Debug helpers
// ─────────────────────────────────────────────────────────────────────────────

void saveNoiseSlice(const std::vector<float>& data, int size, int sliceZ, const char* filename)
{
    const size_t offset = static_cast<size_t>(sliceZ) * static_cast<size_t>(size) * static_cast<size_t>(size);
    if (offset + static_cast<size_t>(size) * size > data.size()) return;

    std::vector<uint8_t> img(static_cast<size_t>(size) * size);
    for (size_t i = 0; i < static_cast<size_t>(size) * size; ++i) {
        const float v = std::clamp((data[offset + i] + 1.0f) * 0.5f, 0.0f, 1.0f);
        img[i] = static_cast<uint8_t>(v * 255.0f);
    }
    const std::string outPath = resolveNoiseOutputPath(filename);
    stbi_write_png(outPath.c_str(), size, size, 1, img.data(), size);
}

void saveNoiseSlice2D(const std::vector<float>& data, int size, const char* filename)
{
    if (data.size() < static_cast<size_t>(size) * size) return;

    std::vector<uint8_t> img(static_cast<size_t>(size) * size);
    for (size_t i = 0; i < static_cast<size_t>(size) * size; ++i) {
        const float v = std::clamp((data[i] + 1.0f) * 0.5f, 0.0f, 1.0f);
        img[i] = static_cast<uint8_t>(v * 255.0f);
    }
    const std::string outPath = resolveNoiseOutputPath(filename);
    stbi_write_png(outPath.c_str(), size, size, 1, img.data(), size);
}

void saveNoiseImage2D01(const std::vector<float>& data, int width, int height, const char* filename)
{
    if (width <= 0 || height <= 0) return;
    if (data.size() < static_cast<size_t>(width) * static_cast<size_t>(height)) return;

    std::vector<uint8_t> img(static_cast<size_t>(width) * static_cast<size_t>(height));
    for (size_t i = 0; i < img.size(); ++i) {
        const float v = std::clamp(data[i], 0.0f, 1.0f);
        img[i] = static_cast<uint8_t>(v * 255.0f);
    }

    const std::string outPath = resolveNoiseOutputPath(filename);
    stbi_write_png(outPath.c_str(), width, height, 1, img.data(), width);
}