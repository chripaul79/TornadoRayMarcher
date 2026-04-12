#pragma once

#include "mesh.h"
#include <vector>

// Terrain mesh from 3D noise volume; noiseData is x-major (x + y*noiseW + z*noiseW*noiseH).
Mesh generateTerrainMeshFromNoise(int width, int height,
    int noiseW, int noiseH, int noiseD,
    const std::vector<float>& noiseData,
    float scale, float maxHeight, float zSlice);

// Same, but z slice from integer zIndex (0 .. noiseD-1).
Mesh generateTerrainMeshFromNoiseSlice(int width, int height,
    int noiseW, int noiseH, int noiseD,
    const std::vector<float>& noiseData,
    float scale, float maxHeight, int zIndex);

// One tile: sub-rect of the heightfield in uv [0,1]²; vertices are tile-local (origin = corner).
Mesh generateTerrainTileMesh(int fullGridW, int fullGridH,
    int tilesPerSide, int tileIx, int tileIy,
    int lodVertsU, int lodVertsV,
    int noiseW, int noiseH, int noiseD,
    const std::vector<float>& noiseData,
    float scale, float maxHeight, float zSliceNorm);
