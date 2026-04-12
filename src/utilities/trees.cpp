#include "utilities/trees.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <vector>

#include <glad/glad.h>
#include <utilities/imageLoader.hpp>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace fs = std::filesystem;

// ── Tree billboard globals ───────────────────────────────────────────────────

GLuint gBillboardQuadVAO = 0;
GLuint gBillboardInstanceVBO = 0;
GLuint gTreeTextureArray = 0;
int    gTreeTextureLayerCount = 0;
int    gBillboardTreeCount = 0;

bool   gEnableTrees = true;
int    gTreeDensity = 400;
float  gTreeMinSize = 2.5f;
float  gTreeMaxSize = 6.0f;
float  gTreeExcludeRadius = 25.0f;
float  gTreeWidthMin = 0.4f;
float  gTreeWidthMax = 0.7f;
float  gTreeYOffset = 0.0f;
float  gTreeAlphaCutoff = 0.3f;
float  gTreeAO = 0.85f;

// ── Texture loading ──────────────────────────────────────────────────────────

void loadTreeTextureArray()
{
    if (gTreeTextureArray != 0) return;

    const fs::path treeDir = fs::u8path(PROJECT_ROOT) / "res" / "textures" / "trees";
    std::vector<fs::path> files;
    if (fs::exists(treeDir) && fs::is_directory(treeDir)) {
        for (auto& entry : fs::directory_iterator(treeDir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
    }

    if (files.empty()) {
        std::cout << "[Trees] No textures in " << treeDir.string() << "\n";
        gTreeTextureLayerCount = 0;
        return;
    }

    struct LoadedImg { unsigned int w, h; std::vector<unsigned char> px; };
    std::vector<LoadedImg> images;
    unsigned int maxW = 0, maxH = 0;

    for (auto& f : files) {
        auto ext = f.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        LoadedImg img;
        if (ext == ".png") {
            PNGImage pi = loadPNGFile(f.string());
            img = { pi.width, pi.height, std::move(pi.pixels) };
        } else {
            JPGImage ji = loadJPGFile(f.string());
            img = { ji.width, ji.height, std::move(ji.pixels) };
        }
        if (img.w == 0 || img.h == 0 || img.px.empty()) continue;
        maxW = std::max(maxW, img.w);
        maxH = std::max(maxH, img.h);
        images.push_back(std::move(img));
    }

    if (images.empty()) {
        std::cout << "[Trees] All tree textures failed to load.\n";
        gTreeTextureLayerCount = 0;
        return;
    }

    // Premultiply alpha to avoid mipmap halo artifacts
    auto premultiply = [](std::vector<unsigned char>& px, unsigned w, unsigned h) {
        for (size_t i = 0; i < size_t(w) * h; ++i) {
            float a = px[i * 4 + 3] / 255.0f;
            px[i * 4 + 0] = (unsigned char)(px[i * 4 + 0] * a);
            px[i * 4 + 1] = (unsigned char)(px[i * 4 + 1] * a);
            px[i * 4 + 2] = (unsigned char)(px[i * 4 + 2] * a);
        }
    };

    glGenTextures(1, &gTreeTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, gTreeTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
                 maxW, maxH, int(images.size()),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    std::vector<unsigned char> resized(size_t(maxW) * maxH * 4, 0);
    for (int i = 0; i < int(images.size()); ++i) {
        auto& img = images[i];
        premultiply(img.px, img.w, img.h);
        if (img.w == maxW && img.h == maxH) {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            0, 0, i, maxW, maxH, 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, img.px.data());
        } else {
            std::fill(resized.begin(), resized.end(), (unsigned char)0);
            for (unsigned y = 0; y < img.h && y < maxH; ++y)
                for (unsigned x = 0; x < img.w && x < maxW; ++x)
                    for (int c = 0; c < 4; ++c)
                        resized[(y * maxW + x) * 4 + c] = img.px[(y * img.w + x) * 4 + c];
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            0, 0, i, maxW, maxH, 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, resized.data());
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    gTreeTextureLayerCount = int(images.size());
    std::cout << "[Trees] Loaded " << gTreeTextureLayerCount << " textures ("
              << maxW << "x" << maxH << ")\n";
}

// ── Tree placement ───────────────────────────────────────────────────────────

void generateBillboardTrees(const std::vector<float>& terrainNoise,
                            int noiseSize,
                            const TreePlacementParams& t)
{
    loadTreeTextureArray();

    // Create instanced quad VAO on first call
    if (gBillboardQuadVAO == 0) {
        float quadVerts[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
        unsigned int quadIdx[] = { 0, 1, 2, 0, 2, 3 };

        GLuint quadVBO, quadEBO;
        glGenVertexArrays(1, &gBillboardQuadVAO);
        glGenBuffers(1, &quadVBO);
        glGenBuffers(1, &quadEBO);

        glBindVertexArray(gBillboardQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIdx), quadIdx, GL_STATIC_DRAW);

        glGenBuffers(1, &gBillboardInstanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, gBillboardInstanceVBO);

        // loc 1: world pos (3 floats), loc 2: height/width/layer/pad (4 floats)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glVertexAttribDivisor(1, 1);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribDivisor(2, 1);

        glBindVertexArray(0);
    }

    if (gTreeTextureLayerCount == 0) {
        gBillboardTreeCount = 0;
        return;
    }

    const float cx = t.originX + t.halfSpan;
    const float cz = t.originZ + t.halfSpan;

    const float worldMinX = cx - t.halfSpan * t.scaleXZ + 5.0f;
    const float worldMaxX = cx + t.halfSpan * t.scaleXZ - 5.0f;
    const float worldMinZ = cz - t.halfSpan * t.scaleXZ + 5.0f;
    const float worldMaxZ = cz + t.halfSpan * t.scaleXZ - 5.0f;

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> distX(worldMinX, worldMaxX);
    std::uniform_real_distribution<float> distZ(worldMinZ, worldMaxZ);
    std::uniform_real_distribution<float> distSize(gTreeMinSize, gTreeMaxSize);
    std::uniform_real_distribution<float> distWR(gTreeWidthMin, std::max(gTreeWidthMin, gTreeWidthMax));
    std::uniform_int_distribution<int>    distTex(0, std::max(gTreeTextureLayerCount - 1, 0));

    struct TreeInstance {
        float x, y, z;
        float height, widthRatio, texLayer, pad;
    };
    std::vector<TreeInstance> trees;
    trees.reserve(gTreeDensity);

    for (int i = 0; i < gTreeDensity * 4 && int(trees.size()) < gTreeDensity; ++i) {
        float wx = distX(rng);
        float wz = distZ(rng);

        // Skip trees too close to tornado
        float ddx = wx - t.tornadoOrigin.x;
        float ddz = wz - t.tornadoOrigin.z;
        if (std::sqrt(ddx * ddx + ddz * ddz) < gTreeExcludeRadius) continue;

        // Sample terrain height via bilinear lookup
        float localU = (wx - (cx - t.halfSpan * t.scaleXZ)) / (t.fullSpan * t.scaleXZ);
        float localV = (wz - (cz - t.halfSpan * t.scaleXZ)) / (t.fullSpan * t.scaleXZ);
        localU = std::clamp(localU, 0.0f, 1.0f);
        localV = std::clamp(localV, 0.0f, 1.0f);

        float fx = localU * float(noiseSize - 1);
        float fy = localV * float(noiseSize - 1);
        int x0 = std::clamp(int(fx), 0, noiseSize - 2);
        int y0 = std::clamp(int(fy), 0, noiseSize - 2);
        float dx = fx - float(x0);
        float dy = fy - float(y0);
        auto at = [&](int xi, int yi) {
            return terrainNoise[size_t(yi) * size_t(noiseSize) + size_t(xi)];
        };
        float n = at(x0, y0) * (1 - dx) * (1 - dy)
                + at(x0 + 1, y0) * dx * (1 - dy)
                + at(x0, y0 + 1) * (1 - dx) * dy
                + at(x0 + 1, y0 + 1) * dx * dy;
        float wy = ((n + 1.0f) * 0.5f) * t.maxH * t.scaleY + gTreeYOffset;

        float h  = distSize(rng);
        float wr = distWR(rng);
        float tl = float(distTex(rng));
        trees.push_back({ wx, wy, wz, h, wr, tl, 0.0f });
    }

    gBillboardTreeCount = int(trees.size());

    glBindBuffer(GL_ARRAY_BUFFER, gBillboardInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 trees.size() * sizeof(TreeInstance),
                 trees.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
