#include "scenelogic.h"
#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "sceneGraph.hpp"
#include <utilities/glutils.h>
#include <utilities/imageLoader.hpp>
#include <utilities/mesh.h>
#include <utilities/shader.hpp>
#include <utilities/trees.h>
#include "utilities/terrain.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include <vector>
#include "utilities/utilities.h"
#include "ui/volume_imgui.h"
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

namespace fs = std::filesystem;

// ── Scene graph ──────────────────────────────────────────────────────────────

struct TerrainChunkGpu {
    SceneNode* node = nullptr;
    GLuint vao{};
    unsigned int indexCount{};
};

SceneNode* rootNode;
SceneNode* terrainRootNode;
SceneNode* treesRootNode;
SceneNode* treeNode;
std::vector<TerrainChunkGpu> gTerrainChunks;

// ── Camera ───────────────────────────────────────────────────────────────────

glm::vec3 cameraPosition;
glm::vec3 cameraFront;
glm::vec3 cameraRight;
glm::vec3 cameraUp;

// ── Shaders ──────────────────────────────────────────────────────────────────

Tornado::Shader* shader;
Tornado::Shader* rayShader;
Tornado::Shader* volumeShader;
Tornado::Shader* compositeShader;
Tornado::Shader* billboardShader;

// ── Terrain noise cache ──────────────────────────────────────────────────────

static std::vector<float> gTerrainNoiseCache;
static int gTerrainNoiseCacheSize = 0;

// ── FBO & VAO textures ─────────────────────────────────────────────────────────────

GLuint gPosition, gAlbedo, gNormal, gAO, gDepth, blueNoiseTexture;
GLuint litSceneColor;
GLuint tornadoColor;
GLuint noiseTexture;
GLuint skyboxTexture = 0;
float gSkyboxBlend = 1.0f;
unsigned int gBuffer, litSceneFBO, tornadoFBO, screenQuadVAO, quadVBO;

// ── Shadow settings ──────────────────────────────────────────────────────────

float gShadowSoftness = 12.472f;
float gShadowMinStep = 0.266f;
int gShadowMaxSteps = 16;
int gShadowQuality = 0;

// ── Misc globals ─────────────────────────────────────────────────────────────

bool gPerformanceMode = false;
bool gUseVolumetricTornado = true;
float gLightningFlash = 0.0f;
bool  gAutoLightning = true;
float gLightningIntensity = 1.152f;
float gLightningFrequency = 0.5f;
glm::vec3 gLightningBoltStart = glm::vec3(0.0f);
glm::vec3 gLightningBoltEnd = glm::vec3(0.0f);
float gLightningBoltSeed = 0.0f;
glm::vec3 gDebrisTint = glm::vec3(0.55f, 0.50f, 0.42f);
glm::vec3 gStormTint = glm::vec3(0.27f, 0.33f, 0.30f);
float gAtmStormMix = 0.398f;

float gFogDensity = 0.00308f;
float gFogHeightFalloff = 0.045f;
glm::vec3 gTornadoTint = glm::vec3(0.745098f, 0.745098f, 0.745098f);

float gTerrainFadeStart = 300.0f;
float gTerrainFadeEnd   = 400.0f;

// ── Cumulonimbus ─────────────────────────────────────────────────────────────

bool  gEnableCb = true;
float gCbOffsetY = 13.272f;
float gCbExtentX = 176.2f;
float gCbExtentY = 20.5f;
float gCbExtentZ = 203.977f;
float gCbDensityMul = 2.008f;
float gCbNoiseScale = 0.00097f;
float gCbNoiseSpeed = 0.2f;
float gCbExtinction = 0.1617f;
float gCbCoverage = 0.486f;
float gCbDetailScale = 0.0071f;
float gCbNoiseLod = 1.98f;
float gCbEdgeSoftness = 1.548f;
glm::vec3 gCbTint = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 gCbLitCol = glm::vec3(0.97f, 0.97f, 0.99f);
glm::vec3 gCbDarkCol = glm::vec3(0.79592f, 0.826262f, 0.897059f);
glm::vec3 gCbSunEdgeCol = glm::vec3(1.00f, 0.95f, 0.85f);

// ── Terrain constants ────────────────────────────────────────────────────────

float gNearPlane = 0.01f;
float gFarPlane  = 2000.0f;
float gTerrainScaleXZ = 3.556f;
float gTerrainScaleY  = 0.978f;
constexpr int kTerrainGrid = 512;
constexpr int kTerrainTilesPerSide = 20;
constexpr float kTerrainScale = 0.52f;
constexpr float kTerrainMaxH = 13.0f;
constexpr int kNoiseVolumeDepth = 64;
constexpr float kTerrainHalfSpan = float(kTerrainGrid - 1) * kTerrainScale * 0.5f;
constexpr float kTerrainOriginX = -118.0f;
constexpr float kTerrainOriginZ = -118.0f;
constexpr float kTerrainDiffuseUvScale = 18.0f;

// ── Matrices ─────────────────────────────────────────────────────────────────

glm::mat4 projection;
glm::mat4 viewMatrix;
glm::mat4 viewProjection;

// ── Light ────────────────────────────────────────────────────────────────────

glm::vec3 sunDirection = glm::normalize(glm::vec3(0.529999f, -0.508799f, -0.678399f));
glm::vec3 lightColor = glm::vec3(1.0);

// ── Tornado system ───────────────────────────────────────────────────────────

struct TornadoSystem {
    glm::vec3 origin = glm::vec3(10.0f, 0.0f, -20.0f);
    float     height = 70.123f;
    float     radius = 25.947f;
    float     strength = 2.5f;
    float     time = 0.0f;

    void update(float dt)
    {
        time += dt;
        strength = 2.5f + std::sin(time * 0.4f) * 0.7f;

        // Here we can control the movement of the tornado.
		origin.x = 10.0f + std::sin(time * 0.17f) * 12.0f;
		origin.z = -20.0f + std::cos(time * 0.11f) * 12.0f;
    }

    void upload(GLuint pid) const
    {
        glUniform3fv(glGetUniformLocation(pid, "uTornadoOrigin"), 1, glm::value_ptr(origin));
    }
};
TornadoSystem gTornadoSystem;

// ── Time & frame ─────────────────────────────────────────────────────────────

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float totalTime = 0.0f;
float fpsAccumTime = 0.0f;
int fpsAccumFrames = 0;
int frameCount = 0;

// ── Volume params ────────────────────────────────────────────────────────────

int debugViewMode = 0;
int volumeDebugMode = 0;
float gVolExtinction = 0.787f;
float gVolAlbedo = 0.734f;
float gVolAnisotropy = 0.203f;
float gVolSunIntensity = 1.797f;
float gVolAmbientIntensity = 0.364f;
float gVolDensityScale = 1.317f;
float gVolStepSize = 0.119f;
int gVolMaxSteps = 128;
float gVolLightStepSize = 0.361f;
int gVolLightSteps = 16;
float gTorHeight = 70.123f;
float gTorMaxRadius = 25.947f;
float gTorRadiusTop = 12.34f;
float gTorRadiusExp = 2.327f;
float gTorPinchStrength = 0.309f;
float gTorWobbleAmp = 4.816f;
float gTorWobbleSpeed = 7.72f;
float gTorSwayAmp = 2.718f;
float gTorShapeNoiseScaleXZ = 0.04f;
float gTorShapeNoiseScaleY = 0.011f;
float gTorShapeNoiseSpeed = 0.91f;
float gTorShapeNoiseAmount = 3.038f;
float gTorTwistBaseTop = 0.372f;
float gTorTwistBaseBottom = 0.257f;
float gTorTwistJitterAmp1 = 0.709f;
float gTorTwistJitterAmp2 = 0.505f;
float gTorTwistJitterAmp3 = 0.196f;
float gTorTwistSpeedBase = 0.0f;
float gTorTwistSpeedVar1 = 0.433f;
float gTorTwistSpeedVar2 = 0.0f;
float gTorFilamentTwist1 = 0.4f;
float gTorFilamentTwist2 = 0.569f;

// ── Debris params ────────────────────────────────────────────────────────────

float gDebrisOrbitSpeed = 8.938f;
float gDebrisInnerOrbitSpeed = 12.0f;
float gDebrisRadiusBottomAdd = 0.689f;
float gDebrisRadiusTopAdd = 9.474f;
float gDebrisRadiusMinor = 2.17f;
float gDebrisVerticalOffset = 0.014f;
float gDebrisNoise1ScaleXZ = 0.05f;
float gDebrisNoise1ScaleY = 0.015f;
float gDebrisNoise1Amp = 0.316f;
float gDebrisNoise1SpeedX = 0.294f;
float gDebrisNoise1SpeedZ = 0.43f;
float gDebrisNoise2ScaleXZ = 0.029f;
float gDebrisNoise2ScaleY = 0.012f;
float gDebrisNoise2Amp = 2.145f;
float gDebrisNoise2SpeedX = 0.115f;
float gDebrisNoise2SpeedZ = 1.0f;
float gDebrisHeightFadeMax = 20.0f;
float gDebrisGroundClip = 2.012f;
float gDebrisDensityMul = 1.352f;
float gDebrisOpacity = 0.014f;
float gDebrisAmbientMul = 0.609f;
float gDebrisSunMul = 0.0f;
float gDebrisPosOffsetX = 0.0f;
float gDebrisPosOffsetY = 0.089f;
float gDebrisPosOffsetZ = 0.004f;
float gDebrisHeightMin = -6.84f;
float gDebrisHeightMax = 20.0f;

// ── UI state ─────────────────────────────────────────────────────────────────

bool gShowImGui = true;
bool gImGuiInitialized = false;
float fov = 80.0f;

// ── Helper: build TreePlacementParams from terrain constants ─────────────────

static TreePlacementParams makeTreeParams()
{
    return {
        gTerrainScaleXZ, gTerrainScaleY,
        kTerrainOriginX, kTerrainOriginZ,
        kTerrainHalfSpan,
        float(kTerrainGrid - 1) * kTerrainScale,
        kTerrainMaxH,
        gTornadoSystem.origin
    };
}

// ── Helper: update tree scene node after generation ──────────────────────────

static void syncTreeNode()
{
    if (!treeNode) return;
    treeNode->vertexArrayObjectID = gBillboardQuadVAO;
    treeNode->VAOIndexCount = 6;
    treeNode->instanceCount = gBillboardTreeCount;
    treeNode->textureArrayID = gTreeTextureArray;
}

void regenerateTrees()
{
    if (!gTerrainNoiseCache.empty() && gTerrainNoiseCacheSize > 0) {
        generateBillboardTrees(gTerrainNoiseCache, gTerrainNoiseCacheSize, makeTreeParams());
        syncTreeNode();
    }
}

// ── Input callbacks ──────────────────────────────────────────────────────────

double lastX = windowWidth / 2;
double lastY = windowHeight / 2;
float yaw = -90.0f;
float pitch = 0;

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
#ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized) {
        volumeUiCursorPosCallback(window, xpos, ypos);
        if (volumeUiWantsMouseCapture()) return;
    }
#endif

    static bool firstMouse = true;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.02f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Scroll with mouse to change FOV (zoom in/out)
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
#ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized) {
        volumeUiScrollCallback(window, xoffset, yoffset);
        if (volumeUiWantsMouseCapture()) return;
    }
#endif
    fov -= (float)yoffset;
    fov = std::clamp(fov, 1.0f, 90.0f);
}

// ── Helper: load texture from jpg or png path ────────────────────────────────

static unsigned int tryLoadTexture(const std::string& path) {
    if (!fs::exists(path)) return 0;
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".jpg" || ext == ".jpeg") {
        JPGImage img = loadJPGFile(path);
        if (img.width == 0) return 0;
        return generateTextureBuffer(img);
    }
    PNGImage img = loadPNGFile(path);
    if (img.width == 0) return 0;
    return generateTextureBuffer(img);
}

// ── Initialization ───────────────────────────────────────────────────────────

void initTornado(GLFWwindow* window)
{
    resetVolumeGuiDefaults();
    loadVolumeGuiSettings();
    initImGui(window);

	glfwSetInputMode(window, GLFW_CURSOR, gShowImGui ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
#ifdef TORNADO_ENABLE_IMGUI
    glfwSetMouseButtonCallback(window, volumeUiMouseButtonCallback);
    glfwSetKeyCallback(window, volumeUiKeyCallback);
    glfwSetCharCallback(window, volumeUiCharCallback);
#endif

    // Compile shaders
    shader = new Tornado::Shader();
    shader->makeBasicShader("res/shaders/simple.vert", "res/shaders/simple.frag");

	rayShader = new Tornado::Shader();
	rayShader->makeBasicShader("res/shaders/ray.vert", "res/shaders/ray.frag");

    volumeShader = new Tornado::Shader();
    volumeShader->makeBasicShader("res/shaders/ray.vert", "res/shaders/volume_tornado.frag");

    compositeShader = new Tornado::Shader();
    compositeShader->makeBasicShader("res/shaders/ray.vert", "res/shaders/composite_final.frag");

    billboardShader = new Tornado::Shader();
    billboardShader->makeBasicShader("res/shaders/billboard.vert", "res/shaders/billboard.frag");

    // ── G-buffer FBO ─────────────────────────────────────────────────────────

	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// Helper to reduce code duplication when creating G-buffer textures and attaching to FBO.
    auto makeGBufferTex = [](GLuint& tex, GLenum internalFmt, GLenum fmt, GLenum type, GLenum attachment) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, windowWidth, windowHeight, 0, fmt, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, tex, 0);
    };

    makeGBufferTex(gPosition, GL_RGBA16F, GL_RGBA, GL_FLOAT,         GL_COLOR_ATTACHMENT0);
    makeGBufferTex(gNormal,   GL_RGBA16F, GL_RGBA, GL_FLOAT,         GL_COLOR_ATTACHMENT1);
    makeGBufferTex(gAlbedo,   GL_RGBA,    GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2);
    makeGBufferTex(gAO,       GL_R16F,    GL_RED,  GL_FLOAT,         GL_COLOR_ATTACHMENT3);

    GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);

    makeGBufferTex(gDepth, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "G-buffer framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── Post-process FBOs ────────────────────────────────────────────────────

    auto makeColorFBO = [](GLuint& fbo, GLuint& tex, int w, int h) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "FBO not complete!" << std::endl;
    };

    makeColorFBO(litSceneFBO,   litSceneColor,   windowWidth, windowHeight);
    makeColorFBO(tornadoFBO,    tornadoColor,     std::max(1, windowWidth / 2), std::max(1, windowHeight / 2));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ── Skybox -──────────────────────────────────────────────────────────────

    auto makeSkyboxFacePaths = [](const fs::path& dir) -> std::vector<fs::path> {
        static const char* names[6] = { "right", "left", "top", "bottom", "front", "back" };
        std::vector<fs::path> out;
        out.reserve(6);
        for (const char* n : names) {
            fs::path jpg = dir / (std::string(n) + ".jpg");
            fs::path png = dir / (std::string(n) + ".png");
            out.push_back(fs::exists(jpg) ? jpg : (fs::exists(png) ? png : jpg));
        }
        return out;
    };


    const fs::path skyboxDir = fs::u8path(PROJECT_ROOT) / "res" / "textures" / "skybox";
    skyboxTexture = loadCubemap(makeSkyboxFacePaths(skyboxDir));
    if (skyboxTexture == 0)
        skyboxTexture = loadCubemap(makeSkyboxFacePaths(fs::current_path() / "res" / "textures" / "skybox"));
  
    // ── Noise generation ─────────────────────────────────────────────────────

	auto start = std::chrono::steady_clock::now();

	const int NOISE_VOL_SIZE = 256; // 128 -> 256 increases cloud detail but also VRAM usage and generation time. 256+ not recommended.
    const int TERRAIN_NOISE_SIZE = 1024;

    auto noiseData = generateCloudData(NOISE_VOL_SIZE, NOISE_VOL_SIZE, NOISE_VOL_SIZE, 1337, 0.04f);
    noiseTexture = uploadCloudNoiseTexture(noiseData, NOISE_VOL_SIZE, NOISE_VOL_SIZE, NOISE_VOL_SIZE);

    gTerrainNoiseCache = generateTerrainNoiseData2D(TERRAIN_NOISE_SIZE);
    gTerrainNoiseCacheSize = TERRAIN_NOISE_SIZE;
    (void)uploadTerrainNoiseTexture2D(gTerrainNoiseCache, TERRAIN_NOISE_SIZE);

	auto end = std::chrono::steady_clock::now();

    const int noiseSlice = std::clamp(NOISE_VOL_SIZE / 2, 0, NOISE_VOL_SIZE - 1);
	saveNoiseSlice(noiseData, NOISE_VOL_SIZE, noiseSlice, "noise_slice.png");
    saveNoiseSlice2D(gTerrainNoiseCache, TERRAIN_NOISE_SIZE, "terrain_noise_slice.png");

    constexpr int BLUE_NOISE_W = 1024;
    constexpr int BLUE_NOISE_H = 1024;
    auto blueNoiseData = generateBlueNoiseData2D(BLUE_NOISE_W, BLUE_NOISE_H, 1.2f, 42u);
    blueNoiseTexture = uploadBlueNoiseTexture2D(blueNoiseData, BLUE_NOISE_W, BLUE_NOISE_H);
    saveNoiseImage2D01(blueNoiseData, BLUE_NOISE_W, BLUE_NOISE_H, "blue_noise.png");

    std::cout << "Noise generation took "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms"
        << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── Scene setup ──────────────────────────────────────────────────────────

    shader->activate();

    rootNode = createSceneNode();
    terrainRootNode = createSceneNode();

    rootNode->children.push_back(terrainRootNode);

    treesRootNode = createSceneNode();
    treesRootNode->vertexArrayObjectID = -1;
    rootNode->children.push_back(treesRootNode);


    // Textures

    JPGImage terrainDiffuseImage = loadJPGFile("res/textures/aerial_grass_rock_diff_4k.jpg");
    unsigned int terrainDiffuseID = generateTextureBuffer(terrainDiffuseImage);

    // Normal map
    unsigned int terrainNormalID = 0;
    for (const auto& suffix : {"nor_gl_4k", "nor_4k", "nrm_4k", "normal_4k"}) {
        for (const auto& ext : {".png", ".jpg"}) {
            terrainNormalID = tryLoadTexture(std::string("res/textures/aerial_grass_rock_") + suffix + ext);
            if (terrainNormalID != 0) break;
        }
        if (terrainNormalID != 0) break;
    }

    // Roughness map (falls back to inverting specular)
    unsigned int terrainRoughnessID = 0;
    bool terrainInvertRoughness = false;
    for (const auto& suffix : {"rough_4k", "roughness_4k"}) {
        for (const auto& ext : {".png", ".jpg"}) {
            terrainRoughnessID = tryLoadTexture(std::string("res/textures/aerial_grass_rock_") + suffix + ext);
            if (terrainRoughnessID != 0) break;
        }
        if (terrainRoughnessID != 0) break;
    }
    if (terrainRoughnessID == 0) {
        terrainRoughnessID = tryLoadTexture("res/textures/aerial_grass_rock_spec_4k.png");
        if (terrainRoughnessID != 0) terrainInvertRoughness = true;
    }

    if (terrainNormalID != 0) std::cout << "[Terrain] Normal map loaded.\n";
    if (terrainRoughnessID != 0) std::cout << "[Terrain] " << (terrainInvertRoughness ? "Spec" : "Roughness") << " map loaded.\n";

    terrainRootNode->position = glm::vec3(kTerrainOriginX, 0.0f, kTerrainOriginZ);

    // ── Terrain chunks ───────────────────────────────────────────────────────

    constexpr int kTerrainRes = 96;
    const float chunkSpan = float(kTerrainGrid - 1) * kTerrainScale / float(kTerrainTilesPerSide);
    gTerrainChunks.clear();
    gTerrainChunks.reserve(kTerrainTilesPerSide * kTerrainTilesPerSide);

    for (int ty = 0; ty < kTerrainTilesPerSide; ++ty) {
        for (int tx = 0; tx < kTerrainTilesPerSide; ++tx) {
            TerrainChunkGpu ch;
            ch.node = createSceneNode();
            ch.node->position = glm::vec3(float(tx) * chunkSpan, 0.0f, float(ty) * chunkSpan);
            ch.node->textureID = terrainDiffuseID;
            ch.node->normalMapTextureID = terrainNormalID;
            ch.node->roughnessMapTextureID = terrainRoughnessID;
            ch.node->invertRoughness = terrainInvertRoughness;
            ch.node->diffuseUvScale = kTerrainDiffuseUvScale;
            ch.node->materialKind = 1;
            Mesh tile = generateTerrainTileMesh(
                kTerrainGrid, kTerrainGrid,
                kTerrainTilesPerSide, tx, ty,
                kTerrainRes, kTerrainRes,
                TERRAIN_NOISE_SIZE, TERRAIN_NOISE_SIZE, 1,
                gTerrainNoiseCache, kTerrainScale, kTerrainMaxH, 0.0f);
            ch.vao = generateBuffer(tile);
            ch.indexCount = static_cast<unsigned int>(tile.indices.size());
            ch.node->vertexArrayObjectID = ch.vao;
            ch.node->VAOIndexCount = ch.indexCount;
            gTerrainChunks.push_back(ch);
            terrainRootNode->children.push_back(ch.node);
        }
    }

    // ── Camera ───────────────────────────────────────────────────────────────

    cameraPosition = glm::vec3(0.0f, 17.0f, 50.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

    // ── Billboard trees (GEOMETRY_2D node) ───────────────────────────────────

    generateBillboardTrees(gTerrainNoiseCache, gTerrainNoiseCacheSize, makeTreeParams());

    treeNode = createSceneNode();
    treeNode->nodeType = GEOMETRY_2D;
    treeNode->useOrthoMvp = false;
    syncTreeNode();
    treesRootNode->children.push_back(treeNode);
}

void shutdownTornado()
{
    saveVolumeGuiSettings();
    shutdownImGui();
}

// ── Frame update ─────────────────────────────────────────────────────────────

void updateFrame(GLFWwindow* window)
{
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    gTornadoSystem.update(deltaTime);

    // Lightning
    if (gAutoLightning) {
        static float nextFlashTime = 0.0f;
        static float flashDecay = 0.0f;

        if (totalTime >= nextFlashTime) {
            flashDecay = gLightningIntensity * (0.6f + 0.4f * std::sin(totalTime * 137.3f));
            float gap = (1.0f / std::max(gLightningFrequency, 0.001f))
                      * (0.4f + 0.6f * (0.5f + 0.5f * std::sin(totalTime * 7.13f)));
            nextFlashTime = totalTime + gap;

            float seed = totalTime * 31.71f;
            auto hashF = [](float n) { return std::fmod(std::sin(n) * 43758.5453f, 1.0f); };
            float offX = (hashF(seed) - 0.5f) * 40.0f;
            float offZ = (hashF(seed + 7.7f) - 0.5f) * 40.0f;
            glm::vec3 torOrig = gTornadoSystem.origin;
            gLightningBoltStart = torOrig + glm::vec3(offX, gTorHeight * 1.3f + hashF(seed + 3.3f) * gTorHeight * 0.25f, offZ);
            gLightningBoltEnd = torOrig + glm::vec3(offX * 0.6f, 0.0f, offZ * 0.6f);
            gLightningBoltSeed = std::fmod(std::abs(seed), 1000.0f);
        }
        flashDecay *= std::exp(-12.0f * deltaTime);
        gLightningFlash = flashDecay;
    }

    // FPS counter
    fpsAccumTime += deltaTime;
    fpsAccumFrames += 1;
    if (fpsAccumTime >= 0.25f) {
        float fps = float(fpsAccumFrames) / std::max(fpsAccumTime, 1.0e-6f);
        float ms = 1000.0f / std::max(fps, 1.0e-6f);
        glfwSetWindowTitle(window, fmt::format("TornadoSceneProject | {:.1f} FPS | {:.2f} ms", fps, ms).c_str());
        fpsAccumTime = 0.0f;
        fpsAccumFrames = 0;
    }

    processInput(window);

    projection = glm::perspective(glm::radians(fov), float(windowWidth) / float(windowHeight), gNearPlane, gFarPlane);
    cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
    viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
    viewProjection = projection * viewMatrix;

    if (terrainRootNode) {
        terrainRootNode->scale = glm::vec3(gTerrainScaleXZ, gTerrainScaleY, gTerrainScaleXZ);
        const float cx = kTerrainOriginX + kTerrainHalfSpan;
        const float cz = kTerrainOriginZ + kTerrainHalfSpan;
        terrainRootNode->position = glm::vec3(
            cx - kTerrainHalfSpan * gTerrainScaleXZ, 0.0f,
            cz - kTerrainHalfSpan * gTerrainScaleXZ);
    }

    updateNodeTransformations(rootNode, glm::mat4(1.0f), viewProjection, viewMatrix);
}

// ── Input processing ─────────────────────────────────────────────────────────

void processInput(GLFWwindow* window)
{
    static bool volumeDebugLatch = false;
    static bool volumePrintLatch = false;
    static bool uiToggleLatch = false;

    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        if (!uiToggleLatch) {
            gShowImGui = !gShowImGui;
            glfwSetInputMode(window, GLFW_CURSOR, gShowImGui ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            uiToggleLatch = true;
        }
    } else {
        uiToggleLatch = false;
    }

    #ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized && gShowImGui && volumeUiWantsKeyboardCapture()) return;
    #endif

    float cameraSpeed = 3.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPosition += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPosition -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) debugViewMode = 0;
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) debugViewMode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) debugViewMode = 2;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) debugViewMode = 3;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) debugViewMode = 4;
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) debugViewMode = 5;
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) debugViewMode = 6;

    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
        if (!volumeDebugLatch) {
            volumeDebugMode = (volumeDebugMode + 1) % 8;
            volumeDebugLatch = true;
        }
    } else {
        volumeDebugLatch = false;
    }

    // Clamp all tunables
    gVolExtinction = std::clamp(gVolExtinction, 0.05f, 2.5f);
    gVolAlbedo = std::clamp(gVolAlbedo, 0.0f, 1.0f);
    gVolAnisotropy = std::clamp(gVolAnisotropy, -0.90f, 0.90f);
    gVolSunIntensity = std::clamp(gVolSunIntensity, 0.0f, 8.0f);
    gVolAmbientIntensity = std::clamp(gVolAmbientIntensity, 0.0f, 4.0f);
    gVolDensityScale = std::clamp(gVolDensityScale, 0.1f, 4.0f);
    gVolStepSize = std::clamp(gVolStepSize, 0.04f, 1.0f);
    gVolMaxSteps = std::clamp(gVolMaxSteps, 8, 128);
    gVolLightStepSize = std::clamp(gVolLightStepSize, 0.05f, 8.0f);
    gVolLightSteps = std::clamp(gVolLightSteps, 1, 64);
    gTorHeight = std::clamp(gTorHeight, 10.0f, 120.0f);
    gTorMaxRadius = std::clamp(gTorMaxRadius, 5.0f, 50.0f);

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        if (!volumePrintLatch) {
            std::cout << "ext=" << gVolExtinction << " alb=" << gVolAlbedo
                      << " g=" << gVolAnisotropy << " sun=" << gVolSunIntensity << std::endl;
            volumePrintLatch = true;
        }
    } else {
        volumePrintLatch = false;
    }
}

// ── Node transform traversal ─────────────────────────────────────────────────

void updateNodeTransformations(SceneNode* node, glm::mat4 parentModel, glm::mat4 viewProjection, glm::mat4 viewMatrix)
{
    glm::mat4 transformationMatrix =
        glm::translate(node->position)
        * glm::translate(node->referencePoint)
        * glm::rotate(node->rotation.y, glm::vec3(0, 1, 0))
        * glm::rotate(node->rotation.x, glm::vec3(1, 0, 0))
        * glm::rotate(node->rotation.z, glm::vec3(0, 0, 1))
        * glm::scale(node->scale)
        * glm::translate(-node->referencePoint);

    node->modelMatrix = parentModel * transformationMatrix;
    node->viewModelMatrix = viewMatrix * node->modelMatrix;
    node->normalMatrix = glm::transpose(glm::inverse(glm::mat3(node->viewModelMatrix)));
    node->currentTransformationMatrix = viewProjection * node->modelMatrix;

    if (node->nodeType == GEOMETRY_2D && node->useOrthoMvp) {
        glm::mat4 ortho = glm::ortho(0.0f, float(windowWidth), 0.0f, float(windowHeight), 0.0f, 100.0f);
        node->currentTransformationMatrix = ortho * node->modelMatrix;
    }

    for (SceneNode* child : node->children)
        updateNodeTransformations(child, node->modelMatrix, viewProjection, viewMatrix);
}

// ── Render scene graph ───────────────────────────────────────────────────────

void renderNode(SceneNode* node)
{
    switch (node->nodeType) {
    case GEOMETRY:
        glUniformMatrix4fv(glGetUniformLocation(shader->get(), "MVP"), 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->get(), "modelMatrix"), 1, GL_FALSE, glm::value_ptr(node->modelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader->get(), "normalMatrix"), 1, GL_FALSE, glm::value_ptr(node->normalMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->get(), "viewModelMatrix"), 1, GL_FALSE, glm::value_ptr(node->viewModelMatrix));
        glUniform1f(glGetUniformLocation(shader->get(), "diffuseUvScale"), node->diffuseUvScale);
        glUniform1i(glGetUniformLocation(shader->get(), "materialKind"), node->materialKind);
        glUniform1f(glGetUniformLocation(shader->get(), "defaultRoughness"), 0.5f);

        if (node->textureID != (unsigned)-1) {
            glUniform1i(glGetUniformLocation(shader->get(), "useDiffuse"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, node->textureID);
        } else {
            glUniform1i(glGetUniformLocation(shader->get(), "useDiffuse"), 0);
        }

        if (node->normalMapTextureID != 0) {
            glUniform1i(glGetUniformLocation(shader->get(), "useNormalMap"), 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, node->normalMapTextureID);
        } else {
            glUniform1i(glGetUniformLocation(shader->get(), "useNormalMap"), 0);
        }

        if (node->roughnessMapTextureID != 0) {
            glUniform1i(glGetUniformLocation(shader->get(), "useRoughnessMap"), 1);
            glUniform1i(glGetUniformLocation(shader->get(), "invertRoughness"), node->invertRoughness ? 1 : 0);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, node->roughnessMapTextureID);
        } else {
            glUniform1i(glGetUniformLocation(shader->get(), "useRoughnessMap"), 0);
        }

        if (node->vertexArrayObjectID != -1) {
            glBindVertexArray(node->vertexArrayObjectID);
            glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
        }
        break;

    case GEOMETRY_2D:
        if (gEnableTrees && node->instanceCount > 0 && node->textureArrayID != 0) {
            shader->deactivate();
            billboardShader->activate();
            GLuint bp = billboardShader->get();
            glUniformMatrix4fv(glGetUniformLocation(bp, "VP"), 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
            glUniformMatrix4fv(glGetUniformLocation(bp, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
            glUniform3fv(glGetUniformLocation(bp, "cameraPos"), 1, glm::value_ptr(cameraPosition));
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D_ARRAY, node->textureArrayID);
            glUniform1i(glGetUniformLocation(bp, "treeTex"), 4);
            glUniform1f(glGetUniformLocation(bp, "alphaCutoff"), gTreeAlphaCutoff);
            glUniform1f(glGetUniformLocation(bp, "treeAO"), gTreeAO);
            glDisable(GL_CULL_FACE);
            glBindVertexArray(node->vertexArrayObjectID);
            glDrawElementsInstanced(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr, node->instanceCount);
            glBindVertexArray(0);
            glActiveTexture(GL_TEXTURE0);
            billboardShader->deactivate();
            shader->activate();
        }
        break;

    default:
        break;
    }

    for (SceneNode* child : node->children)
        renderNode(child);
}

// ── Helper: upload all tornado uniforms to a shader ──────────────────────────

static void uploadTornadoUniforms(GLuint pid)
{
    glUniform1f(glGetUniformLocation(pid, "uTornadoHeight"), gTorHeight);
    glUniform1f(glGetUniformLocation(pid, "uRadiusTop"), gTorRadiusTop);
    glUniform1f(glGetUniformLocation(pid, "uRadiusExp"), gTorRadiusExp);
    glUniform1f(glGetUniformLocation(pid, "uPinchStrength"), gTorPinchStrength);
    glUniform1f(glGetUniformLocation(pid, "uShapeNoiseScaleXZ"), gTorShapeNoiseScaleXZ);
    glUniform1f(glGetUniformLocation(pid, "uShapeNoiseScaleY"), gTorShapeNoiseScaleY);
    glUniform1f(glGetUniformLocation(pid, "uShapeNoiseSpeed"), gTorShapeNoiseSpeed);
    glUniform1f(glGetUniformLocation(pid, "uShapeNoiseAmount"), gTorShapeNoiseAmount);
    glUniform1f(glGetUniformLocation(pid, "uWobbleAmp"), gTorWobbleAmp);
    glUniform1f(glGetUniformLocation(pid, "uWobbleSpeed"), gTorWobbleSpeed);
    glUniform1f(glGetUniformLocation(pid, "uSwayAmp"), gTorSwayAmp);
    glUniform1f(glGetUniformLocation(pid, "uTwistBaseTop"), gTorTwistBaseTop);
    glUniform1f(glGetUniformLocation(pid, "uTwistBaseBottom"), gTorTwistBaseBottom);
    glUniform1f(glGetUniformLocation(pid, "uTwistJitterAmp1"), gTorTwistJitterAmp1);
    glUniform1f(glGetUniformLocation(pid, "uTwistJitterAmp2"), gTorTwistJitterAmp2);
    glUniform1f(glGetUniformLocation(pid, "uTwistJitterAmp3"), gTorTwistJitterAmp3);
    glUniform1f(glGetUniformLocation(pid, "uTwistSpeedBase"), gTorTwistSpeedBase);
    glUniform1f(glGetUniformLocation(pid, "uTwistSpeedVar1"), gTorTwistSpeedVar1);
    glUniform1f(glGetUniformLocation(pid, "uTwistSpeedVar2"), gTorTwistSpeedVar2);
    glUniform1f(glGetUniformLocation(pid, "uFilamentTwist1"), gTorFilamentTwist1);
    glUniform1f(glGetUniformLocation(pid, "uFilamentTwist2"), gTorFilamentTwist2);
}

static void uploadDebrisUniforms(GLuint pid)
{
    glUniform1f(glGetUniformLocation(pid, "uDebrisOrbitSpeed"), gDebrisOrbitSpeed);
    glUniform1f(glGetUniformLocation(pid, "uDebrisInnerOrbitSpeed"), gDebrisInnerOrbitSpeed);
    glUniform1f(glGetUniformLocation(pid, "uDebrisRadiusBottomAdd"), gDebrisRadiusBottomAdd);
    glUniform1f(glGetUniformLocation(pid, "uDebrisRadiusTopAdd"), gDebrisRadiusTopAdd);
    glUniform1f(glGetUniformLocation(pid, "uDebrisRadiusMinor"), gDebrisRadiusMinor);
    glUniform1f(glGetUniformLocation(pid, "uDebrisVerticalOffset"), gDebrisVerticalOffset);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise1ScaleXZ"), gDebrisNoise1ScaleXZ);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise1ScaleY"), gDebrisNoise1ScaleY);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise1Amp"), gDebrisNoise1Amp);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise1SpeedX"), gDebrisNoise1SpeedX);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise1SpeedZ"), gDebrisNoise1SpeedZ);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise2ScaleXZ"), gDebrisNoise2ScaleXZ);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise2ScaleY"), gDebrisNoise2ScaleY);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise2Amp"), gDebrisNoise2Amp);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise2SpeedX"), gDebrisNoise2SpeedX);
    glUniform1f(glGetUniformLocation(pid, "uDebrisNoise2SpeedZ"), gDebrisNoise2SpeedZ);
    glUniform1f(glGetUniformLocation(pid, "uDebrisHeightFadeMax"), gDebrisHeightFadeMax);
    glUniform1f(glGetUniformLocation(pid, "uDebrisGroundClip"), gDebrisGroundClip);
    glUniform1f(glGetUniformLocation(pid, "uDebrisDensityMul"), gDebrisDensityMul);
    glUniform1f(glGetUniformLocation(pid, "uDebrisOpacity"), gDebrisOpacity);
    glUniform1f(glGetUniformLocation(pid, "uDebrisAmbientMul"), gDebrisAmbientMul);
    glUniform1f(glGetUniformLocation(pid, "uDebrisSunMul"), gDebrisSunMul);
    glUniform3f(glGetUniformLocation(pid, "uDebrisPosOffset"), gDebrisPosOffsetX, gDebrisPosOffsetY, gDebrisPosOffsetZ);
    glUniform1f(glGetUniformLocation(pid, "uDebrisHeightMin"), gDebrisHeightMin);
    glUniform1f(glGetUniformLocation(pid, "uDebrisHeightMax"), gDebrisHeightMax);
}

// ── Render frame ─────────────────────────────────────────────────────────────

void renderFrame(GLFWwindow* window) {
	int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

	frameCount += 1;
    constexpr float kTimeScale = 0.1f;
    totalTime += deltaTime * kTimeScale;

    // Pass 1: G-buffer
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const float aoClear = 1.0f;
    glClearBufferfv(GL_COLOR, 3, &aoClear);

    shader->activate();
    renderNode(rootNode);
    shader->deactivate();

    glm::mat4 invVP = glm::inverse(viewProjection);

    // Pass 2: deferred lighting
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindFramebuffer(GL_FRAMEBUFFER, litSceneFBO);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    rayShader->activate();
    GLuint rp = rayShader->get();
    glm::vec3 sunDirView = glm::normalize(glm::mat3(viewMatrix) * sunDirection);
    glUniform3fv(glGetUniformLocation(rp, "light.direction"), 1, glm::value_ptr(sunDirView));
    glUniform3fv(glGetUniformLocation(rp, "light.color"), 1, glm::value_ptr(lightColor));
    glUniform1i(glGetUniformLocation(rp, "debugView"), debugViewMode);
    glm::mat4 invView = glm::inverse(viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(rp, "invView"), 1, GL_FALSE, glm::value_ptr(invView));
    glUniform3fv(glGetUniformLocation(rp, "sunDirectionWorld"), 1, glm::value_ptr(sunDirection));
    glUniform1f(glGetUniformLocation(rp, "uTime"), totalTime);
    glUniform1f(glGetUniformLocation(rp, "u_shadowSoftness"), gShadowSoftness);
    glUniform1f(glGetUniformLocation(rp, "u_shadowMinStep"), gShadowMinStep);
    glUniform1i(glGetUniformLocation(rp, "u_shadowMaxSteps"), std::clamp(gShadowMaxSteps, 4, 32));
    glUniform1i(glGetUniformLocation(rp, "u_shadowQuality"), gShadowQuality);
    glUniform3fv(glGetUniformLocation(rp, "uTornadoOrigin"), 1, glm::value_ptr(gTornadoSystem.origin));
    uploadTornadoUniforms(rp);
    uploadDebrisUniforms(rp);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, gAO);
    glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_3D, noiseTexture);

    renderQuad();
    rayShader->deactivate();

    // Pass 3: tornado volume (half-res)
    const int tornadoW = std::max(1, windowWidth / 2);
    const int tornadoH = std::max(1, windowHeight / 2);
    glBindFramebuffer(GL_FRAMEBUFFER, tornadoFBO);
    glViewport(0, 0, tornadoW, tornadoH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    volumeShader->activate();
    GLuint vp = volumeShader->get();
    glUniformMatrix4fv(glGetUniformLocation(vp, "invVP"), 1, GL_FALSE, glm::value_ptr(invVP));
    glUniform3fv(glGetUniformLocation(vp, "cameraPos"), 1, glm::value_ptr(cameraPosition));
    glUniform3fv(glGetUniformLocation(vp, "sunDirectionWorld"), 1, glm::value_ptr(sunDirection));
    glUniform1f(glGetUniformLocation(vp, "uTime"), totalTime);
    glUniform1i(glGetUniformLocation(vp, "uMaxSteps"), gPerformanceMode ? std::min(gVolMaxSteps, 64) : gVolMaxSteps);
    glUniform1i(glGetUniformLocation(vp, "uEnableVolume"), gUseVolumetricTornado ? 1 : 0);
    glUniform1f(glGetUniformLocation(vp, "uExtinction"), gVolExtinction);
    glUniform1f(glGetUniformLocation(vp, "uAlbedo"), gVolAlbedo);
    glUniform1f(glGetUniformLocation(vp, "uAnisotropy"), gVolAnisotropy);
    glUniform1f(glGetUniformLocation(vp, "uSunIntensity"), gVolSunIntensity);
    glUniform1f(glGetUniformLocation(vp, "uAmbientIntensity"), gVolAmbientIntensity);
    glUniform1f(glGetUniformLocation(vp, "uDensityScale"), gVolDensityScale);
    glUniform1f(glGetUniformLocation(vp, "uStepSize"), gVolStepSize);
    glUniform1f(glGetUniformLocation(vp, "uLightStepSize"), gVolLightStepSize);
    glUniform1i(glGetUniformLocation(vp, "uLightSteps"), gVolLightSteps);
    gTornadoSystem.upload(vp);
    glUniform1f(glGetUniformLocation(vp, "uTornadoMaxRadius"), gTorMaxRadius);
    uploadTornadoUniforms(vp);
    uploadDebrisUniforms(vp);
    glUniform3fv(glGetUniformLocation(vp, "uTornadoTint"), 1, glm::value_ptr(gTornadoTint));
    glUniform3fv(glGetUniformLocation(vp, "uDebrisTint"), 1, glm::value_ptr(gDebrisTint));

    // Cumulonimbus
    glUniform1i(glGetUniformLocation(vp, "uEnableCb"), gEnableCb ? 1 : 0);
    {
        glm::vec3 cbCenter = gTornadoSystem.origin + glm::vec3(0.0f, gTorHeight + gCbOffsetY, 0.0f);
        glm::vec3 cbExtent(gCbExtentX, gCbExtentY, gCbExtentZ);
        glUniform3fv(glGetUniformLocation(vp, "uCbCenter"), 1, glm::value_ptr(cbCenter));
        glUniform3fv(glGetUniformLocation(vp, "uCbExtent"), 1, glm::value_ptr(cbExtent));
    }
    glUniform1f(glGetUniformLocation(vp, "uCbDensityMul"), gCbDensityMul);
    glUniform1f(glGetUniformLocation(vp, "uCbNoiseScale"), gCbNoiseScale);
    glUniform1f(glGetUniformLocation(vp, "uCbNoiseSpeed"), gCbNoiseSpeed);
    glUniform1f(glGetUniformLocation(vp, "uCbExtinction"), gCbExtinction);
    glUniform1f(glGetUniformLocation(vp, "uCbCoverage"), gCbCoverage);
    glUniform1f(glGetUniformLocation(vp, "uCbDetailScale"), gCbDetailScale);
    glUniform1f(glGetUniformLocation(vp, "uCbNoiseLod"), gCbNoiseLod);
    glUniform1f(glGetUniformLocation(vp, "uCbEdgeSoftness"), gCbEdgeSoftness);
    glUniform3fv(glGetUniformLocation(vp, "uCbTint"), 1, glm::value_ptr(gCbTint));
    glUniform3fv(glGetUniformLocation(vp, "uCbLitCol"), 1, glm::value_ptr(gCbLitCol));
    glUniform3fv(glGetUniformLocation(vp, "uCbDarkCol"), 1, glm::value_ptr(gCbDarkCol));
    glUniform3fv(glGetUniformLocation(vp, "uCbSunEdgeCol"), 1, glm::value_ptr(gCbSunEdgeCol));

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, litSceneColor);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_3D, noiseTexture);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, blueNoiseTexture);
    renderQuad();
    volumeShader->deactivate();

    // Pass 4: final composite
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    compositeShader->activate();
    GLuint cp = compositeShader->get();
    glUniformMatrix4fv(glGetUniformLocation(cp, "invVP"), 1, GL_FALSE, glm::value_ptr(invVP));
    glUniform3fv(glGetUniformLocation(cp, "cameraPos"), 1, glm::value_ptr(cameraPosition));
    glUniform3fv(glGetUniformLocation(cp, "sunDirectionWorld"), 1, glm::value_ptr(sunDirection));
    glUniform2f(glGetUniformLocation(cp, "fullResolution"), float(windowWidth), float(windowHeight));
    glUniform1f(glGetUniformLocation(cp, "fogDensity"), gFogDensity);
    glUniform1f(glGetUniformLocation(cp, "fogHeightFalloff"), gFogHeightFalloff);
    glUniform1f(glGetUniformLocation(cp, "atmStormMix"), gAtmStormMix);
    glUniform3fv(glGetUniformLocation(cp, "stormTint"), 1, glm::value_ptr(gStormTint));
    glUniform1f(glGetUniformLocation(cp, "uSkyboxBlend"), gSkyboxBlend);
    glUniform1f(glGetUniformLocation(cp, "terrainFadeStart"), gTerrainFadeStart);
    glUniform1f(glGetUniformLocation(cp, "terrainFadeEnd"), gTerrainFadeEnd);
    glUniform1f(glGetUniformLocation(cp, "lightningFlash"), gLightningFlash);
    glUniformMatrix4fv(glGetUniformLocation(cp, "VP"), 1, GL_FALSE, glm::value_ptr(viewProjection));
    glUniform3fv(glGetUniformLocation(cp, "lightningBoltStart"), 1, glm::value_ptr(gLightningBoltStart));
    glUniform3fv(glGetUniformLocation(cp, "lightningBoltEnd"), 1, glm::value_ptr(gLightningBoltEnd));
    glUniform1f(glGetUniformLocation(cp, "lightningBoltSeed"), gLightningBoltSeed);
    glUniform1i(glGetUniformLocation(cp, "sceneColor"), 0);
    glUniform1i(glGetUniformLocation(cp, "gDepth"), 1);
    glUniform1i(glGetUniformLocation(cp, "tornadoBuffer"), 2);
    glUniform1i(glGetUniformLocation(cp, "skybox"), 3);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, litSceneColor);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, gDepth);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tornadoColor);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    renderQuad();
    compositeShader->deactivate();

    drawImGuiOverlay();

    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
}

// ── Screen quad ──────────────────────────────────────────────────────────────

void renderQuad() {
    if (screenQuadVAO == 0) {
        // https://github.com/JoeyDeVries/LearnOpenGL
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &screenQuadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(screenQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(screenQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
