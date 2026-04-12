#include "terrain.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// -----------------------------------------------------------------------------
//  NOTE: generateTerrainMeshFromNoise() and generateTerrainMeshFromNoiseSlice()
//  have been removed.  They were dead code — the tiled LOD system in
//  scenelogic.cpp calls only generateTerrainTileMesh().  Keeping them forced
//  the compiler to instantiate sampleNoiseTrilinear for an untiled path that
//  was never exercised, and bloated terrain.h's required interface.
//
//  If you ever need a quick single-mesh prototype again, the functions can be
//  restored from version control; the logic is identical to a single tile that
//  covers the whole grid (tileIx=0, tileIy=0, tilesPerSide=1).
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//  sampleNoiseTrilinear
// -----------------------------------------------------------------------------
//  Internal helper.  Performs trilinear interpolation inside a packed 3D
//  float buffer.  For terrain the depth dimension is always 1 (the noise is
//  2D but stored in the 3D-capable buffer so terrain.cpp doesn't need a
//  separate code path).  When noiseD==1 the z-interpolation collapses to a
//  single layer — no extra cost at runtime because all mesh generation is
//  done once at startup.
// -----------------------------------------------------------------------------
static inline float sampleNoiseTrilinear(const std::vector<float>& data,
    int w, int h, int d,
    float fx, float fy, float fz)
{
    if (w <= 0 || h <= 0 || d <= 0 || data.empty()) return 0.0f;

    const size_t expected = static_cast<size_t>(w)
        * static_cast<size_t>(h)
        * static_cast<size_t>(d);
    if (data.size() < expected) return 0.0f;

    fx = std::clamp(fx, 0.0f, 1.0f);
    fy = std::clamp(fy, 0.0f, 1.0f);
    fz = std::clamp(fz, 0.0f, 1.0f);

    const float x = fx * static_cast<float>(w - 1);
    const float y = fy * static_cast<float>(h - 1);
    const float z = fz * static_cast<float>(d - 1);

    const int x0 = static_cast<int>(std::floor(x));
    const int x1 = std::min(x0 + 1, w - 1);
    const int y0 = static_cast<int>(std::floor(y));
    const int y1 = std::min(y0 + 1, h - 1);
    const int z0 = static_cast<int>(std::floor(z));
    const int z1 = std::min(z0 + 1, d - 1);

    const float xd = x - static_cast<float>(x0);
    const float yd = y - static_cast<float>(y0);
    const float zd = z - static_cast<float>(z0);

    auto at = [&](int xi, int yi, int zi) -> float {
        const size_t idx = static_cast<size_t>(xi)
            + static_cast<size_t>(yi) * static_cast<size_t>(w)
            + static_cast<size_t>(zi) * static_cast<size_t>(w) * static_cast<size_t>(h);
        return (idx < data.size()) ? data[idx] : 0.0f;
        };

    const float c00 = at(x0, y0, z0) * (1 - xd) + at(x1, y0, z0) * xd;
    const float c10 = at(x0, y1, z0) * (1 - xd) + at(x1, y1, z0) * xd;
    const float c01 = at(x0, y0, z1) * (1 - xd) + at(x1, y0, z1) * xd;
    const float c11 = at(x0, y1, z1) * (1 - xd) + at(x1, y1, z1) * xd;

    const float c0 = c00 * (1 - yd) + c10 * yd;
    const float c1 = c01 * (1 - yd) + c11 * yd;

    return c0 * (1 - zd) + c1 * zd;
}


// -----------------------------------------------------------------------------
//  generateTerrainTileMesh
// -----------------------------------------------------------------------------
//
//  Generates one LOD level for one tile of the terrain grid.
//
//  LOD system summary (three static meshes per tile, swapped by updateTerrainLOD)
//  -------------------------------------------------------------------
//  lodRes[0] = 96  verts/side  — near  (d < kTerrainLodNearDist = 85 m)
//  lodRes[1] = 48  verts/side  — mid   (85–175 m)
//  lodRes[2] = 24  verts/side  — far   (d > kTerrainLodFarDist  = 175 m)
//
//  All three meshes are baked once at startup in initTornado().  updateTerrainLOD
//  just swaps the active VAO handle on each chunk node - zero per-frame cost.
//
//  Normal computation
//  ------------------
//  Normals are derived from the *global* UV gradient (step size = one texel in
//  the full-resolution noise grid, regardless of the tile's LOD resolution).
//  This keeps the lighting seamless across LOD boundaries and tile edges -
//  there are no "chessboard" normal discontinuities.
//
Mesh generateTerrainTileMesh(int fullGridW, int fullGridH,
    int tilesPerSide, int tileIx, int tileIy,
    int lodVertsU, int lodVertsV,
    int noiseW, int noiseH, int noiseD,
    const std::vector<float>& noiseData,
    float scale, float maxHeight, float zSliceNorm)
{
    Mesh m;
    if (fullGridW <= 1 || fullGridH <= 1 || lodVertsU <= 1 || lodVertsV <= 1 || tilesPerSide < 1)
        return m;

    const size_t expectedNoiseSize = static_cast<size_t>(std::max(noiseW, 0))
        * static_cast<size_t>(std::max(noiseH, 0))
        * static_cast<size_t>(std::max(noiseD, 0));
    if (expectedNoiseSize == 0 || noiseData.size() < expectedNoiseSize)
        return m;

    // UV range of this tile in normalised [0,1] space.
    const float uMin = float(tileIx) / float(tilesPerSide);
    const float uMax = (tileIx + 1 >= tilesPerSide) ? 1.0f : float(tileIx + 1) / float(tilesPerSide);
    const float vMin = float(tileIy) / float(tilesPerSide);
    const float vMax = (tileIy + 1 >= tilesPerSide) ? 1.0f : float(tileIy + 1) / float(tilesPerSide);

    // World-space origin of this tile (vertices are stored tile-local so the
    // scene node's position handles the global offset).
    const float lx0 = uMin * float(fullGridW - 1) * scale;
    const float lz0 = vMin * float(fullGridH - 1) * scale;

    m.vertices.reserve(static_cast<size_t>(lodVertsU) * lodVertsV);
    m.normals.resize(static_cast<size_t>(lodVertsU) * lodVertsV);
    m.textureCoordinates.reserve(static_cast<size_t>(lodVertsU) * lodVertsV);
    std::vector<float> heights(static_cast<size_t>(lodVertsU) * lodVertsV);

    // -- Vertices ----------------------------------------------------------------
    for (int y = 0; y < lodVertsV; ++y) {
        for (int x = 0; x < lodVertsU; ++x) {
            const float fu = float(x) / float(lodVertsU - 1);
            const float fv = float(y) / float(lodVertsV - 1);
            const float u = uMin + (uMax - uMin) * fu;
            const float v = vMin + (vMax - vMin) * fv;

            const float n = sampleNoiseTrilinear(noiseData, noiseW, noiseH, noiseD, u, v, zSliceNorm);
            // Remap [-1, 1] -> [0, maxHeight].
            const float h = ((n + 1.0f) * 0.5f) * maxHeight;
            heights[static_cast<size_t>(y * lodVertsU + x)] = h;

            const float lx = u * float(fullGridW - 1) * scale;
            const float lz = v * float(fullGridH - 1) * scale;
            m.vertices.emplace_back(lx - lx0, h, lz - lz0);
            m.textureCoordinates.emplace_back(u, v);
        }
    }

    // -- Indices ----------------------------------------------------------------
    for (int y = 0; y < lodVertsV - 1; ++y) {
        for (int x = 0; x < lodVertsU - 1; ++x) {
            const unsigned i = static_cast<unsigned>(y * lodVertsU + x);
            m.indices.push_back(i);
            m.indices.push_back(i + static_cast<unsigned>(lodVertsU));
            m.indices.push_back(i + 1);

            m.indices.push_back(i + 1);
            m.indices.push_back(i + static_cast<unsigned>(lodVertsU));
            m.indices.push_back(i + static_cast<unsigned>(lodVertsU) + 1);
        }
    }

    // -- Normals (global-UV gradient, LOD-independent) --------------------------
    // Step size = one texel in the full-resolution noise, expressed in UV space.
    // Using the full-res step means the gradient magnitude is consistent across
    // all LOD levels, so lighting doesn't shift when a tile pops to a coarser mesh.
    const float du = 1.0f / float(std::max(1, fullGridW - 1));
    const float dv = 1.0f / float(std::max(1, fullGridH - 1));

    for (int y = 0; y < lodVertsV; ++y) {
        for (int x = 0; x < lodVertsU; ++x) {
            const float fu = float(x) / float(lodVertsU - 1);
            const float fv = float(y) / float(lodVertsV - 1);
            const float u = uMin + (uMax - uMin) * fu;
            const float v = vMin + (vMax - vMin) * fv;

            // Central difference in global UV space.
            const float uL = std::clamp(u - du, 0.0f, 1.0f);
            const float uR = std::clamp(u + du, 0.0f, 1.0f);
            const float vD = std::clamp(v - dv, 0.0f, 1.0f);
            const float vU = std::clamp(v + dv, 0.0f, 1.0f);

            auto sampleH = [&](float su, float sv) -> float {
                return ((sampleNoiseTrilinear(noiseData, noiseW, noiseH, noiseD,
                    su, sv, zSliceNorm) + 1.0f) * 0.5f) * maxHeight;
                };

            const float hL = sampleH(uL, v);
            const float hR = sampleH(uR, v);
            const float hD = sampleH(u, vD);
            const float hU = sampleH(u, vU);

            const float dx = std::max((uR - uL) * float(fullGridW - 1) * scale, 1e-5f);
            const float dz = std::max((vU - vD) * float(fullGridH - 1) * scale, 1e-5f);

            const glm::vec3 nm = glm::normalize(glm::vec3(
                -(hR - hL) / dx,
                1.0f,
                -(hU - hD) / dz
            ));
            m.normals[static_cast<size_t>(y * lodVertsU + x)] = nm;
        }
    }

    return m;
}