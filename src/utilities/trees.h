#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/vec3.hpp>

// ── Tree billboard globals ───────────────────────────────────────────────────

extern GLuint gBillboardQuadVAO;
extern GLuint gBillboardInstanceVBO;
extern GLuint gTreeTextureArray;
extern int    gTreeTextureLayerCount;
extern int    gBillboardTreeCount;

extern bool   gEnableTrees;
extern int    gTreeDensity;
extern float  gTreeMinSize;
extern float  gTreeMaxSize;
extern float  gTreeExcludeRadius;
extern float  gTreeWidthMin;
extern float  gTreeWidthMax;
extern float  gTreeYOffset;
extern float  gTreeAlphaCutoff;
extern float  gTreeAO;

// Terrain info needed for tree placement
struct TreePlacementParams {
    float scaleXZ, scaleY;
    float originX, originZ;
    float halfSpan, fullSpan;
    float maxH;
    glm::vec3 tornadoOrigin;
};

void loadTreeTextureArray();

void generateBillboardTrees(const std::vector<float>& terrainNoise,
                            int noiseSize,
                            const TreePlacementParams& terrain);
