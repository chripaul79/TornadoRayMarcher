#include "ui/volume_imgui.h"

#ifdef TORNADO_ENABLE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

extern bool gShowImGui;
extern bool gImGuiInitialized;
extern bool gPerformanceMode;
extern bool gUseVolumetricTornado;
extern int volumeDebugMode;

extern float gVolExtinction;
extern float gVolAlbedo;
extern float gVolAnisotropy;
extern float gVolSunIntensity;
extern float gVolAmbientIntensity;
extern float gVolDensityScale;
extern float gVolStepSize;
extern int gVolMaxSteps;
extern float gVolLightStepSize;
extern int gVolLightSteps;

extern float gTorHeight;
extern float gTorMaxRadius;
extern float gTorRadiusTop;
extern float gTorRadiusExp;
extern float gTorPinchStrength;
extern float gTorWobbleAmp;
extern float gTorWobbleSpeed;
extern float gTorSwayAmp;
extern float gTorShapeNoiseScaleXZ;
extern float gTorShapeNoiseScaleY;
extern float gTorShapeNoiseSpeed;
extern float gTorShapeNoiseAmount;
extern float gTorTwistBaseTop;
extern float gTorTwistBaseBottom;
extern float gTorTwistJitterAmp1;
extern float gTorTwistJitterAmp2;
extern float gTorTwistJitterAmp3;
extern float gTorTwistSpeedBase;
extern float gTorTwistSpeedVar1;
extern float gTorTwistSpeedVar2;
extern float gTorFilamentTwist1;
extern float gTorFilamentTwist2;

extern float gDebrisOrbitSpeed;
extern float gDebrisInnerOrbitSpeed;
extern float gDebrisRadiusBottomAdd;
extern float gDebrisRadiusTopAdd;
extern float gDebrisRadiusMinor;
extern float gDebrisVerticalOffset;
extern float gDebrisNoise1ScaleXZ;
extern float gDebrisNoise1ScaleY;
extern float gDebrisNoise1Amp;
extern float gDebrisNoise1SpeedX;
extern float gDebrisNoise1SpeedZ;
extern float gDebrisNoise2ScaleXZ;
extern float gDebrisNoise2ScaleY;
extern float gDebrisNoise2Amp;
extern float gDebrisNoise2SpeedX;
extern float gDebrisNoise2SpeedZ;
extern float gDebrisHeightFadeMax;
extern float gDebrisGroundClip;
extern float gDebrisDensityMul;
extern float gDebrisOpacity;
extern float gDebrisAmbientMul;
extern float gDebrisSunMul;
extern float gDebrisPosOffsetX;
extern float gDebrisPosOffsetY;
extern float gDebrisPosOffsetZ;
extern float gDebrisHeightMin;
extern float gDebrisHeightMax;
extern float gLightningFlash;
extern bool  gAutoLightning;
extern float gLightningIntensity;
extern float gLightningFrequency;
extern glm::vec3 gDebrisTint;
extern float gShadowSoftness;
extern float gShadowMinStep;
extern int gShadowMaxSteps;
extern int gShadowQuality;
extern glm::vec3 gStormTint;
extern float gAtmStormMix;

extern float gFogDensity;
extern float gFogHeightFalloff;
extern float gSkyboxBlend;
extern glm::vec3 gTornadoTint;

extern float gTerrainFadeStart;
extern float gTerrainFadeEnd;

extern float gNearPlane;
extern float gFarPlane;
extern float gTerrainScaleXZ;
extern float gTerrainScaleY;

extern glm::vec3 sunDirection;
extern glm::vec3 lightColor;

#include <utilities/trees.h>
void regenerateTrees();

extern bool  gEnableCb;
extern float gCbOffsetY;
extern float gCbExtentX;
extern float gCbExtentY;
extern float gCbExtentZ;
extern float gCbDensityMul;
extern float gCbNoiseScale;
extern float gCbNoiseSpeed;
extern float gCbExtinction;
extern float gCbCoverage;
extern float gCbDetailScale;
extern float gCbNoiseLod;
extern float gCbEdgeSoftness;
extern glm::vec3 gCbTint;
extern glm::vec3 gCbLitCol;
extern glm::vec3 gCbDarkCol;
extern glm::vec3 gCbSunEdgeCol;

extern glm::vec3 cameraPosition;

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

void resetVolumeGuiDefaults()
{
    gVolExtinction = 0.55f;
    gVolAlbedo = 0.92f;
    gVolAnisotropy = 0.20f;
    gVolSunIntensity = 2.0f;
    gVolAmbientIntensity = 0.9f;
    gVolDensityScale = 1.0f;
    gVolStepSize = 0.16f;
    gVolMaxSteps = 112;
    gVolLightStepSize = 2.0f;
    gVolLightSteps = 8;
    gTorHeight = 60.0f;
    gTorMaxRadius = 25.0f;
    gTorRadiusTop = 15.0f;
    gTorRadiusExp = 2.5f;
    gTorPinchStrength = 0.3f;
    gTorWobbleAmp = 4.0f;
    gTorWobbleSpeed = 2.4f;
    gTorSwayAmp = 1.0f;
    gTorShapeNoiseScaleXZ = 0.032f;
    gTorShapeNoiseScaleY = 0.009f;
    gTorShapeNoiseSpeed = 0.12f;
    gTorShapeNoiseAmount = 2.4f;
    gTorTwistBaseTop = 0.12f;
    gTorTwistBaseBottom = 0.02f;
    gTorTwistJitterAmp1 = 0.3f;
    gTorTwistJitterAmp2 = 0.1f;
    gTorTwistJitterAmp3 = 0.05f;
    gTorTwistSpeedBase = 32.0f;
    gTorTwistSpeedVar1 = 3.0f;
    gTorTwistSpeedVar2 = 1.5f;
    gTorFilamentTwist1 = 0.4f;
    gTorFilamentTwist2 = 0.6f;
    gDebrisOrbitSpeed = 4.5f;
    gDebrisInnerOrbitSpeed = 2.8f;
    gDebrisRadiusBottomAdd = 0.8f;
    gDebrisRadiusTopAdd = 1.8f;
    gDebrisRadiusMinor = 2.2f;
    gDebrisVerticalOffset = 0.4f;
    gDebrisNoise1ScaleXZ = 0.12f;
    gDebrisNoise1ScaleY = 0.08f;
    gDebrisNoise1Amp = 1.8f;
    gDebrisNoise1SpeedX = 0.06f;
    gDebrisNoise1SpeedZ = 0.04f;
    gDebrisNoise2ScaleXZ = 0.20f;
    gDebrisNoise2ScaleY = 0.14f;
    gDebrisNoise2Amp = 0.9f;
    gDebrisNoise2SpeedX = 0.05f;
    gDebrisNoise2SpeedZ = 0.03f;
    gDebrisHeightFadeMax = 5.0f;
    gDebrisGroundClip = 1.0f;
    gDebrisDensityMul = 1.4f;
    gDebrisOpacity = 0.95f;
    gDebrisAmbientMul = 0.9f;
    gDebrisSunMul = 0.8f;
    gDebrisPosOffsetX = 0.0f;
    gDebrisPosOffsetY = 0.0f;
    gDebrisPosOffsetZ = 0.0f;
    gDebrisHeightMin = -1.0f;
    gDebrisHeightMax = 3.5f;
    gLightningFlash = 0.0f;
    gAutoLightning = true;
    gLightningIntensity = 3.0f;
    gLightningFrequency = 0.12f;
    gDebrisTint = glm::vec3(0.55f, 0.50f, 0.42f);
    gShadowSoftness = 12.0f;
    gShadowMinStep = 0.25f;
    gShadowMaxSteps = 32;
    gShadowQuality = 0;
    gStormTint = glm::vec3(0.27f, 0.33f, 0.30f);
    gAtmStormMix = 0.82f;
    gSkyboxBlend = 1.0f;
    gFogDensity = 0.0065f;
    gFogHeightFalloff = 0.045f;
    gTornadoTint = glm::vec3(1.0f, 1.0f, 1.0f);
    gNearPlane = 0.1f;
    gFarPlane = 600.0f;
    gTerrainScaleXZ = 1.0f;
    gTerrainScaleY = 1.0f;
    sunDirection = glm::normalize(glm::vec3(0.25f, -0.24f, -0.32f));
    lightColor = glm::vec3(1.0f);
    gTerrainFadeStart = 120.0f;
    gTerrainFadeEnd = 220.0f;
    gEnableTrees = true;
    gTreeDensity = 400;
    gTreeMinSize = 2.5f;
    gTreeMaxSize = 6.0f;
    gTreeExcludeRadius = 25.0f;
    gTreeWidthMin = 0.4f;
    gTreeWidthMax = 0.7f;
    gTreeYOffset = 0.0f;
    gTreeAlphaCutoff = 0.3f;
    gTreeAO = 0.85f;
    gEnableCb = true;
    gCbOffsetY = 15.0f;
    gCbExtentX = 120.0f;
    gCbExtentY = 12.0f;
    gCbExtentZ = 120.0f;
    gCbDensityMul = 2.5f;
    gCbNoiseScale = 0.0022f;
    gCbNoiseSpeed = 0.03f;
    gCbExtinction = 0.06f;
    gCbCoverage = 0.55f;
    gCbDetailScale = 0.0055f;
    gCbNoiseLod = 2.5f;
    gCbEdgeSoftness = 0.22f;
    gCbTint = glm::vec3(1.0f, 1.0f, 1.0f);
    gCbLitCol = glm::vec3(0.97f, 0.97f, 0.99f);
    gCbDarkCol = glm::vec3(0.55f, 0.58f, 0.65f);
    gCbSunEdgeCol = glm::vec3(1.00f, 0.95f, 0.85f);
    volumeDebugMode = 0;
}

static std::filesystem::path getVolumeGuiSettingsPath()
{
    return std::filesystem::u8path(PROJECT_ROOT) / "res" / "config" / "volume_gui_settings.txt";
}

void saveVolumeGuiSettings()
{
    const std::filesystem::path p = getVolumeGuiSettingsPath();
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);

    std::ofstream out(p, std::ios::trunc);
    if (!out.is_open()) return;

    out << "gPerformanceMode=" << (gPerformanceMode ? 1 : 0) << "\n";
    out << "gUseVolumetricTornado=" << (gUseVolumetricTornado ? 1 : 0) << "\n";
    out << "gVolExtinction=" << gVolExtinction << "\n";
    out << "gVolAlbedo=" << gVolAlbedo << "\n";
    out << "gVolAnisotropy=" << gVolAnisotropy << "\n";
    out << "gVolSunIntensity=" << gVolSunIntensity << "\n";
    out << "gVolAmbientIntensity=" << gVolAmbientIntensity << "\n";
    out << "gVolDensityScale=" << gVolDensityScale << "\n";
    out << "gVolStepSize=" << gVolStepSize << "\n";
    out << "gVolMaxSteps=" << gVolMaxSteps << "\n";
    out << "gVolLightStepSize=" << gVolLightStepSize << "\n";
    out << "gVolLightSteps=" << gVolLightSteps << "\n";
    out << "gTorHeight=" << gTorHeight << "\n";
    out << "gTorMaxRadius=" << gTorMaxRadius << "\n";
    out << "gTorRadiusTop=" << gTorRadiusTop << "\n";
    out << "gTorRadiusExp=" << gTorRadiusExp << "\n";
    out << "gTorPinchStrength=" << gTorPinchStrength << "\n";
    out << "gTorWobbleAmp=" << gTorWobbleAmp << "\n";
    out << "gTorWobbleSpeed=" << gTorWobbleSpeed << "\n";
    out << "gTorSwayAmp=" << gTorSwayAmp << "\n";
    out << "gTorShapeNoiseScaleXZ=" << gTorShapeNoiseScaleXZ << "\n";
    out << "gTorShapeNoiseScaleY=" << gTorShapeNoiseScaleY << "\n";
    out << "gTorShapeNoiseSpeed=" << gTorShapeNoiseSpeed << "\n";
    out << "gTorShapeNoiseAmount=" << gTorShapeNoiseAmount << "\n";
    out << "gTorTwistBaseTop=" << gTorTwistBaseTop << "\n";
    out << "gTorTwistBaseBottom=" << gTorTwistBaseBottom << "\n";
    out << "gTorTwistJitterAmp1=" << gTorTwistJitterAmp1 << "\n";
    out << "gTorTwistJitterAmp2=" << gTorTwistJitterAmp2 << "\n";
    out << "gTorTwistJitterAmp3=" << gTorTwistJitterAmp3 << "\n";
    out << "gTorTwistSpeedBase=" << gTorTwistSpeedBase << "\n";
    out << "gTorTwistSpeedVar1=" << gTorTwistSpeedVar1 << "\n";
    out << "gTorTwistSpeedVar2=" << gTorTwistSpeedVar2 << "\n";
    out << "gTorFilamentTwist1=" << gTorFilamentTwist1 << "\n";
    out << "gTorFilamentTwist2=" << gTorFilamentTwist2 << "\n";
    out << "gDebrisOrbitSpeed=" << gDebrisOrbitSpeed << "\n";
    out << "gDebrisInnerOrbitSpeed=" << gDebrisInnerOrbitSpeed << "\n";
    out << "gDebrisRadiusBottomAdd=" << gDebrisRadiusBottomAdd << "\n";
    out << "gDebrisRadiusTopAdd=" << gDebrisRadiusTopAdd << "\n";
    out << "gDebrisRadiusMinor=" << gDebrisRadiusMinor << "\n";
    out << "gDebrisVerticalOffset=" << gDebrisVerticalOffset << "\n";
    out << "gDebrisNoise1ScaleXZ=" << gDebrisNoise1ScaleXZ << "\n";
    out << "gDebrisNoise1ScaleY=" << gDebrisNoise1ScaleY << "\n";
    out << "gDebrisNoise1Amp=" << gDebrisNoise1Amp << "\n";
    out << "gDebrisNoise1SpeedX=" << gDebrisNoise1SpeedX << "\n";
    out << "gDebrisNoise1SpeedZ=" << gDebrisNoise1SpeedZ << "\n";
    out << "gDebrisNoise2ScaleXZ=" << gDebrisNoise2ScaleXZ << "\n";
    out << "gDebrisNoise2ScaleY=" << gDebrisNoise2ScaleY << "\n";
    out << "gDebrisNoise2Amp=" << gDebrisNoise2Amp << "\n";
    out << "gDebrisNoise2SpeedX=" << gDebrisNoise2SpeedX << "\n";
    out << "gDebrisNoise2SpeedZ=" << gDebrisNoise2SpeedZ << "\n";
    out << "gDebrisHeightFadeMax=" << gDebrisHeightFadeMax << "\n";
    out << "gDebrisGroundClip=" << gDebrisGroundClip << "\n";
    out << "gDebrisDensityMul=" << gDebrisDensityMul << "\n";
    out << "gDebrisOpacity=" << gDebrisOpacity << "\n";
    out << "gDebrisAmbientMul=" << gDebrisAmbientMul << "\n";
    out << "gDebrisSunMul=" << gDebrisSunMul << "\n";
    out << "gDebrisPosOffsetX=" << gDebrisPosOffsetX << "\n";
    out << "gDebrisPosOffsetY=" << gDebrisPosOffsetY << "\n";
    out << "gDebrisPosOffsetZ=" << gDebrisPosOffsetZ << "\n";
    out << "gDebrisHeightMin=" << gDebrisHeightMin << "\n";
    out << "gDebrisHeightMax=" << gDebrisHeightMax << "\n";
    out << "gLightningFlash=" << gLightningFlash << "\n";
    out << "gAutoLightning=" << (gAutoLightning ? 1 : 0) << "\n";
    out << "gLightningIntensity=" << gLightningIntensity << "\n";
    out << "gLightningFrequency=" << gLightningFrequency << "\n";
    out << "gDebrisTintX=" << gDebrisTint.x << "\n";
    out << "gDebrisTintY=" << gDebrisTint.y << "\n";
    out << "gDebrisTintZ=" << gDebrisTint.z << "\n";
    out << "gShadowSoftness=" << gShadowSoftness << "\n";
    out << "gShadowMinStep=" << gShadowMinStep << "\n";
    out << "gShadowMaxSteps=" << gShadowMaxSteps << "\n";
    out << "gShadowQuality=" << gShadowQuality << "\n";
    out << "gStormTintX=" << gStormTint.x << "\n";
    out << "gStormTintY=" << gStormTint.y << "\n";
    out << "gStormTintZ=" << gStormTint.z << "\n";
    out << "gAtmStormMix=" << gAtmStormMix << "\n";
    out << "gFogDensity=" << gFogDensity << "\n";
    out << "gFogHeightFalloff=" << gFogHeightFalloff << "\n";
    out << "gSkyboxBlend=" << gSkyboxBlend << "\n";
    out << "gTornadoTintX=" << gTornadoTint.x << "\n";
    out << "gTornadoTintY=" << gTornadoTint.y << "\n";
    out << "gTornadoTintZ=" << gTornadoTint.z << "\n";
    out << "gNearPlane=" << gNearPlane << "\n";
    out << "gFarPlane=" << gFarPlane << "\n";
    out << "gTerrainScaleXZ=" << gTerrainScaleXZ << "\n";
    out << "gTerrainScaleY=" << gTerrainScaleY << "\n";
    out << "sunDirX=" << sunDirection.x << "\n";
    out << "sunDirY=" << sunDirection.y << "\n";
    out << "sunDirZ=" << sunDirection.z << "\n";
    out << "lightColorR=" << lightColor.r << "\n";
    out << "lightColorG=" << lightColor.g << "\n";
    out << "lightColorB=" << lightColor.b << "\n";
    out << "gTerrainFadeStart=" << gTerrainFadeStart << "\n";
    out << "gTerrainFadeEnd=" << gTerrainFadeEnd << "\n";
    out << "gEnableTrees=" << (gEnableTrees ? 1 : 0) << "\n";
    out << "gTreeDensity=" << gTreeDensity << "\n";
    out << "gTreeMinSize=" << gTreeMinSize << "\n";
    out << "gTreeMaxSize=" << gTreeMaxSize << "\n";
    out << "gTreeExcludeRadius=" << gTreeExcludeRadius << "\n";
    out << "gTreeWidthMin=" << gTreeWidthMin << "\n";
    out << "gTreeWidthMax=" << gTreeWidthMax << "\n";
    out << "gTreeYOffset=" << gTreeYOffset << "\n";
    out << "gTreeAlphaCutoff=" << gTreeAlphaCutoff << "\n";
    out << "gTreeAO=" << gTreeAO << "\n";
    out << "gEnableCb=" << (gEnableCb ? 1 : 0) << "\n";
    out << "gCbOffsetY=" << gCbOffsetY << "\n";
    out << "gCbExtentX=" << gCbExtentX << "\n";
    out << "gCbExtentY=" << gCbExtentY << "\n";
    out << "gCbExtentZ=" << gCbExtentZ << "\n";
    out << "gCbDensityMul=" << gCbDensityMul << "\n";
    out << "gCbNoiseScale=" << gCbNoiseScale << "\n";
    out << "gCbNoiseSpeed=" << gCbNoiseSpeed << "\n";
    out << "gCbExtinction=" << gCbExtinction << "\n";
    out << "gCbCoverage=" << gCbCoverage << "\n";
    out << "gCbDetailScale=" << gCbDetailScale << "\n";
    out << "gCbNoiseLod=" << gCbNoiseLod << "\n";
    out << "gCbEdgeSoftness=" << gCbEdgeSoftness << "\n";
    out << "gCbTintR=" << gCbTint.r << "\n";
    out << "gCbTintG=" << gCbTint.g << "\n";
    out << "gCbTintB=" << gCbTint.b << "\n";
    out << "gCbLitColR=" << gCbLitCol.r << "\n";
    out << "gCbLitColG=" << gCbLitCol.g << "\n";
    out << "gCbLitColB=" << gCbLitCol.b << "\n";
    out << "gCbDarkColR=" << gCbDarkCol.r << "\n";
    out << "gCbDarkColG=" << gCbDarkCol.g << "\n";
    out << "gCbDarkColB=" << gCbDarkCol.b << "\n";
    out << "gCbSunEdgeColR=" << gCbSunEdgeCol.r << "\n";
    out << "gCbSunEdgeColG=" << gCbSunEdgeCol.g << "\n";
    out << "gCbSunEdgeColB=" << gCbSunEdgeCol.b << "\n";
    out << "volumeDebugMode=" << volumeDebugMode << "\n";
}

void loadVolumeGuiSettings()
{
    const std::filesystem::path p = getVolumeGuiSettingsPath();
    std::ifstream in(p);
    if (!in.is_open()) return;

    using Setter = std::function<void(const std::string&)>;
    auto sf = [](float& dst) -> Setter { return [&dst](const std::string& v) { dst = std::stof(v); }; };
    auto si = [](int& dst)   -> Setter { return [&dst](const std::string& v) { dst = std::stoi(v); }; };
    auto sb = [](bool& dst)  -> Setter { return [&dst](const std::string& v) { dst = (std::stoi(v) != 0); }; };

    const std::unordered_map<std::string, Setter> dispatch = {
        {"gPerformanceMode",       sb(gPerformanceMode)},
        {"gUseVolumetricTornado",  sb(gUseVolumetricTornado)},
        {"gVolExtinction",         sf(gVolExtinction)},
        {"gVolAlbedo",             sf(gVolAlbedo)},
        {"gVolAnisotropy",         sf(gVolAnisotropy)},
        {"gVolSunIntensity",       sf(gVolSunIntensity)},
        {"gVolAmbientIntensity",   sf(gVolAmbientIntensity)},
        {"gVolDensityScale",       sf(gVolDensityScale)},
        {"gVolStepSize",           sf(gVolStepSize)},
        {"gVolMaxSteps",           si(gVolMaxSteps)},
        {"gVolLightStepSize",      sf(gVolLightStepSize)},
        {"gVolLightSteps",         si(gVolLightSteps)},
        {"gTorHeight",             sf(gTorHeight)},
        {"gTorMaxRadius",          sf(gTorMaxRadius)},
        {"gTorRadiusTop",          sf(gTorRadiusTop)},
        {"gTorRadiusExp",          sf(gTorRadiusExp)},
        {"gTorPinchStrength",      sf(gTorPinchStrength)},
        {"gTorWobbleAmp",          sf(gTorWobbleAmp)},
        {"gTorWobbleSpeed",        sf(gTorWobbleSpeed)},
        {"gTorSwayAmp",            sf(gTorSwayAmp)},
        {"gTorShapeNoiseScaleXZ",  sf(gTorShapeNoiseScaleXZ)},
        {"gTorShapeNoiseScaleY",   sf(gTorShapeNoiseScaleY)},
        {"gTorShapeNoiseSpeed",    sf(gTorShapeNoiseSpeed)},
        {"gTorShapeNoiseAmount",   sf(gTorShapeNoiseAmount)},
        {"gTorTwistBaseTop",       sf(gTorTwistBaseTop)},
        {"gTorTwistBaseBottom",    sf(gTorTwistBaseBottom)},
        {"gTorTwistJitterAmp1",    sf(gTorTwistJitterAmp1)},
        {"gTorTwistJitterAmp2",    sf(gTorTwistJitterAmp2)},
        {"gTorTwistJitterAmp3",    sf(gTorTwistJitterAmp3)},
        {"gTorTwistSpeedBase",     sf(gTorTwistSpeedBase)},
        {"gTorTwistSpeedVar1",     sf(gTorTwistSpeedVar1)},
        {"gTorTwistSpeedVar2",     sf(gTorTwistSpeedVar2)},
        {"gTorFilamentTwist1",     sf(gTorFilamentTwist1)},
        {"gTorFilamentTwist2",     sf(gTorFilamentTwist2)},
        {"gDebrisOrbitSpeed",      sf(gDebrisOrbitSpeed)},
        {"gDebrisInnerOrbitSpeed", sf(gDebrisInnerOrbitSpeed)},
        {"gDebrisRadiusBottomAdd", sf(gDebrisRadiusBottomAdd)},
        {"gDebrisRadiusTopAdd",    sf(gDebrisRadiusTopAdd)},
        {"gDebrisRadiusMajor",     sf(gDebrisRadiusBottomAdd)},
        {"gDebrisRadiusMinor",     sf(gDebrisRadiusMinor)},
        {"gDebrisVerticalOffset",  sf(gDebrisVerticalOffset)},
        {"gDebrisNoise1ScaleXZ",   sf(gDebrisNoise1ScaleXZ)},
        {"gDebrisNoise1ScaleY",    sf(gDebrisNoise1ScaleY)},
        {"gDebrisNoise1Amp",       sf(gDebrisNoise1Amp)},
        {"gDebrisNoise1SpeedX",    sf(gDebrisNoise1SpeedX)},
        {"gDebrisNoise1SpeedZ",    sf(gDebrisNoise1SpeedZ)},
        {"gDebrisNoise2ScaleXZ",   sf(gDebrisNoise2ScaleXZ)},
        {"gDebrisNoise2ScaleY",    sf(gDebrisNoise2ScaleY)},
        {"gDebrisNoise2Amp",       sf(gDebrisNoise2Amp)},
        {"gDebrisNoise2SpeedX",    sf(gDebrisNoise2SpeedX)},
        {"gDebrisNoise2SpeedZ",    sf(gDebrisNoise2SpeedZ)},
        {"gDebrisHeightFadeMax",   sf(gDebrisHeightFadeMax)},
        {"gDebrisGroundClip",      sf(gDebrisGroundClip)},
        {"gDebrisDensityMul",      sf(gDebrisDensityMul)},
        {"gDebrisOpacity",         sf(gDebrisOpacity)},
        {"gDebrisAmbientMul",      sf(gDebrisAmbientMul)},
        {"gDebrisSunMul",          sf(gDebrisSunMul)},
        {"gDebrisPosOffsetX",      sf(gDebrisPosOffsetX)},
        {"gDebrisPosOffsetY",      sf(gDebrisPosOffsetY)},
        {"gDebrisPosOffsetZ",      sf(gDebrisPosOffsetZ)},
        {"gDebrisHeightMin",       sf(gDebrisHeightMin)},
        {"gDebrisHeightMax",       sf(gDebrisHeightMax)},
        {"gLightningFlash",        sf(gLightningFlash)},
        {"gAutoLightning",         sb(gAutoLightning)},
        {"gLightningIntensity",    sf(gLightningIntensity)},
        {"gLightningFrequency",    sf(gLightningFrequency)},
        {"gDebrisTintX",           sf(gDebrisTint.x)},
        {"gDebrisTintY",           sf(gDebrisTint.y)},
        {"gDebrisTintZ",           sf(gDebrisTint.z)},
        {"gShadowSoftness",        sf(gShadowSoftness)},
        {"gShadowMinStep",         sf(gShadowMinStep)},
        {"gShadowMaxSteps",        si(gShadowMaxSteps)},
        {"gShadowQuality",         si(gShadowQuality)},
        {"gStormTintX",            sf(gStormTint.x)},
        {"gStormTintY",            sf(gStormTint.y)},
        {"gStormTintZ",            sf(gStormTint.z)},
        {"gAtmStormMix",           sf(gAtmStormMix)},
        {"gFogDensity",            sf(gFogDensity)},
        {"gFogHeightFalloff",      sf(gFogHeightFalloff)},
        {"gSkyboxBlend",           sf(gSkyboxBlend)},
        {"gTornadoTintX",          sf(gTornadoTint.x)},
        {"gTornadoTintY",          sf(gTornadoTint.y)},
        {"gTornadoTintZ",          sf(gTornadoTint.z)},
        {"gNearPlane",             sf(gNearPlane)},
        {"gFarPlane",              sf(gFarPlane)},
        {"gTerrainScaleXZ",        sf(gTerrainScaleXZ)},
        {"gTerrainScaleY",         sf(gTerrainScaleY)},
        {"gTerrainScale",          sf(gTerrainScaleXZ)},
        {"sunDirX",                sf(sunDirection.x)},
        {"sunDirY",                sf(sunDirection.y)},
        {"sunDirZ",                sf(sunDirection.z)},
        {"lightColorR",            sf(lightColor.r)},
        {"lightColorG",            sf(lightColor.g)},
        {"lightColorB",            sf(lightColor.b)},
        {"gTerrainFadeStart",      sf(gTerrainFadeStart)},
        {"gTerrainFadeEnd",        sf(gTerrainFadeEnd)},
        {"gEnableTrees",           sb(gEnableTrees)},
        {"gTreeDensity",           si(gTreeDensity)},
        {"gTreeMinSize",           sf(gTreeMinSize)},
        {"gTreeMaxSize",           sf(gTreeMaxSize)},
        {"gTreeExcludeRadius",     sf(gTreeExcludeRadius)},
        {"gTreeWidthMin",          sf(gTreeWidthMin)},
        {"gTreeWidthMax",          sf(gTreeWidthMax)},
        {"gTreeYOffset",           sf(gTreeYOffset)},
        {"gTreeAlphaCutoff",       sf(gTreeAlphaCutoff)},
        {"gTreeAO",                sf(gTreeAO)},
        {"gEnableCb",              sb(gEnableCb)},
        {"gCbOffsetY",             sf(gCbOffsetY)},
        {"gCbExtentX",             sf(gCbExtentX)},
        {"gCbExtentY",             sf(gCbExtentY)},
        {"gCbExtentZ",             sf(gCbExtentZ)},
        {"gCbDensityMul",          sf(gCbDensityMul)},
        {"gCbNoiseScale",          sf(gCbNoiseScale)},
        {"gCbNoiseSpeed",          sf(gCbNoiseSpeed)},
        {"gCbExtinction",          sf(gCbExtinction)},
        {"gCbCoverage",            sf(gCbCoverage)},
        {"gCbDetailScale",         sf(gCbDetailScale)},
        {"gCbNoiseLod",            sf(gCbNoiseLod)},
        {"gCbEdgeSoftness",        sf(gCbEdgeSoftness)},
        {"gCbTintR",               sf(gCbTint.r)},
        {"gCbTintG",               sf(gCbTint.g)},
        {"gCbTintB",               sf(gCbTint.b)},
        {"gCbLitColR",             sf(gCbLitCol.r)},
        {"gCbLitColG",             sf(gCbLitCol.g)},
        {"gCbLitColB",             sf(gCbLitCol.b)},
        {"gCbDarkColR",            sf(gCbDarkCol.r)},
        {"gCbDarkColG",            sf(gCbDarkCol.g)},
        {"gCbDarkColB",            sf(gCbDarkCol.b)},
        {"gCbSunEdgeColR",         sf(gCbSunEdgeCol.r)},
        {"gCbSunEdgeColG",         sf(gCbSunEdgeCol.g)},
        {"gCbSunEdgeColB",         sf(gCbSunEdgeCol.b)},
        {"volumeDebugMode",        si(volumeDebugMode)},
    };

    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        auto it = dispatch.find(key);
        if (it != dispatch.end()) it->second(val);
    }
}

void initImGui(GLFWwindow* window)
{
#ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 450");
    gImGuiInitialized = true;
    std::cout << "ImGui initialized. Press F1 to toggle panel." << std::endl;
#else
    (void)window;
    gImGuiInitialized = false;
#endif
}

void drawImGuiOverlay()
{
#ifdef TORNADO_ENABLE_IMGUI
    if (!gImGuiInitialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (gShowImGui) {
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0f, 520.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Scene Controls");
        ImGui::Text("F1 toggles this panel");

        if (ImGui::Button("Reset defaults")) resetVolumeGuiDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Save")) saveVolumeGuiSettings();
        ImGui::SameLine();
        if (ImGui::Button("Load")) loadVolumeGuiSettings();

        ImGui::Separator();
        ImGui::Checkbox("Performance mode", &gPerformanceMode);
        ImGui::SliderInt("Debug mode", &volumeDebugMode, 0, 7);

        if (ImGui::CollapsingHeader("Camera & Scene")) {
            ImGui::DragFloat3("Camera position", &cameraPosition.x, 0.5f);
            ImGui::SliderFloat("Near plane", &gNearPlane, 0.01f, 10.0f, "%.3f");
            ImGui::SliderFloat("Far plane", &gFarPlane, 50.0f, 2000.0f);
            ImGui::SliderFloat("Terrain extent (XZ)", &gTerrainScaleXZ, 0.1f, 5.0f);
            ImGui::SliderFloat("Terrain height (Y)", &gTerrainScaleY, 0.1f, 5.0f);
        }

        if (ImGui::CollapsingHeader("Lighting")) {
            ImGui::DragFloat3("Sun direction", &sunDirection.x, 0.01f, -1.0f, 1.0f);
            if (ImGui::IsItemDeactivatedAfterEdit())
                sunDirection = glm::normalize(sunDirection);
            ImGui::ColorEdit3("Light color", &lightColor.x);
            ImGui::Text("Sun dir: (%.2f, %.2f, %.2f)", sunDirection.x, sunDirection.y, sunDirection.z);
        }

        if (ImGui::CollapsingHeader("Billboard Trees")) {
            ImGui::Checkbox("Enable trees", &gEnableTrees);
            bool changed = false;
            changed |= ImGui::SliderInt("Tree count", &gTreeDensity, 0, 8000);
            changed |= ImGui::SliderFloat("Min height", &gTreeMinSize, 0.5f, 20.0f);
            changed |= ImGui::SliderFloat("Max height", &gTreeMaxSize, 1.0f, 40.0f);
            changed |= ImGui::SliderFloat("Min width ratio", &gTreeWidthMin, 0.1f, 2.0f);
            changed |= ImGui::SliderFloat("Max width ratio", &gTreeWidthMax, 0.1f, 2.0f);
            changed |= ImGui::SliderFloat("Y offset", &gTreeYOffset, -10.0f, 10.0f);
            changed |= ImGui::SliderFloat("Exclude radius (tornado)", &gTreeExcludeRadius, 0.0f, 200.0f);
            if (changed || ImGui::Button("Regenerate Trees")) {
                regenerateTrees();
            }
            ImGui::Separator();
            ImGui::SliderFloat("Alpha cutoff", &gTreeAlphaCutoff, 0.01f, 0.9f);
            ImGui::SliderFloat("Tree AO", &gTreeAO, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Volumetric Rendering")) {
            ImGui::Checkbox("Enable volumetric tornado", &gUseVolumetricTornado);
            ImGui::ColorEdit3("Tornado tint", &gTornadoTint.x);
            ImGui::SliderFloat("Extinction", &gVolExtinction, 0.05f, 2.5f);
            ImGui::SliderFloat("Albedo", &gVolAlbedo, 0.0f, 1.0f);
            ImGui::SliderFloat("Anisotropy", &gVolAnisotropy, -0.9f, 0.9f);
            ImGui::SliderFloat("Sun intensity", &gVolSunIntensity, 0.0f, 8.0f);
            ImGui::SliderFloat("Ambient intensity", &gVolAmbientIntensity, 0.0f, 4.0f);
            ImGui::SliderFloat("Density scale", &gVolDensityScale, 0.1f, 4.0f);
            ImGui::SliderFloat("Step size", &gVolStepSize, 0.04f, 1.0f);
            ImGui::SliderInt("Max steps", &gVolMaxSteps, 8, 128);
            ImGui::SliderFloat("Light step size", &gVolLightStepSize, 0.05f, 8.0f);
            ImGui::SliderInt("Light steps", &gVolLightSteps, 1, 64);
        }

        if (ImGui::CollapsingHeader("Tornado Shape")) {
            ImGui::SliderFloat("Height", &gTorHeight, 10.0f, 120.0f);
            ImGui::SliderFloat("Max bounds radius", &gTorMaxRadius, 5.0f, 50.0f);
            ImGui::SliderFloat("Top radius", &gTorRadiusTop, 2.0f, 30.0f);
            ImGui::SliderFloat("Radius exponent", &gTorRadiusExp, 0.5f, 5.0f);
            ImGui::SliderFloat("Pinch strength", &gTorPinchStrength, 0.0f, 0.9f);
            ImGui::SliderFloat("Wobble amplitude", &gTorWobbleAmp, 0.0f, 10.0f);
            ImGui::SliderFloat("Wobble speed", &gTorWobbleSpeed, 0.0f, 8.0f);
            ImGui::SliderFloat("Sway amplitude", &gTorSwayAmp, 0.0f, 3.0f);

            if (ImGui::TreeNode("Noise")) {
                ImGui::SliderFloat("Shape noise XZ", &gTorShapeNoiseScaleXZ, 0.001f, 0.2f);
                ImGui::SliderFloat("Shape noise Y", &gTorShapeNoiseScaleY, 0.001f, 0.05f);
                ImGui::SliderFloat("Shape noise speed", &gTorShapeNoiseSpeed, 0.0f, 1.0f);
                ImGui::SliderFloat("Shape noise amount", &gTorShapeNoiseAmount, 0.0f, 6.0f);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Twist")) {
                ImGui::SliderFloat("Twist base top", &gTorTwistBaseTop, 0.0f, 0.6f);
                ImGui::SliderFloat("Twist base bottom", &gTorTwistBaseBottom, 0.0f, 0.3f);
                ImGui::SliderFloat("Twist jitter amp1", &gTorTwistJitterAmp1, 0.0f, 1.5f);
                ImGui::SliderFloat("Twist jitter amp2", &gTorTwistJitterAmp2, 0.0f, 1.0f);
                ImGui::SliderFloat("Twist jitter amp3", &gTorTwistJitterAmp3, 0.0f, 0.6f);
                ImGui::SliderFloat("Twist speed base", &gTorTwistSpeedBase, 0.0f, 80.0f);
                ImGui::SliderFloat("Twist speed var1", &gTorTwistSpeedVar1, 0.0f, 12.0f);
                ImGui::SliderFloat("Twist speed var2", &gTorTwistSpeedVar2, 0.0f, 12.0f);
                ImGui::SliderFloat("Filament twist 1", &gTorFilamentTwist1, 0.0f, 2.0f);
                ImGui::SliderFloat("Filament twist 2", &gTorFilamentTwist2, 0.0f, 2.0f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Debris")) {
            ImGui::ColorEdit3("Debris tint", &gDebrisTint.x);
            ImGui::SliderFloat("Orbit speed", &gDebrisOrbitSpeed, 0.0f, 12.0f);
            ImGui::SliderFloat("Inner orbit speed", &gDebrisInnerOrbitSpeed, 0.0f, 12.0f);
            ImGui::SliderFloat("Radius bottom add", &gDebrisRadiusBottomAdd, 0.0f, 12.0f);
            ImGui::SliderFloat("Radius top add", &gDebrisRadiusTopAdd, 0.0f, 12.0f);
            ImGui::SliderFloat("Shell thickness", &gDebrisRadiusMinor, 0.05f, 6.0f);
            ImGui::SliderFloat("Vertical offset", &gDebrisVerticalOffset, -3.0f, 3.0f);
            ImGui::SliderFloat("Opacity", &gDebrisOpacity, 0.0f, 1.0f);
            ImGui::SliderFloat("Density mul", &gDebrisDensityMul, 0.1f, 4.0f);
            ImGui::SliderFloat("Ambient mul", &gDebrisAmbientMul, 0.0f, 2.0f);
            ImGui::SliderFloat("Sun mul", &gDebrisSunMul, 0.0f, 2.0f);
            ImGui::SliderFloat("Height min", &gDebrisHeightMin, -20.0f, 20.0f);
            ImGui::SliderFloat("Height max", &gDebrisHeightMax, -20.0f, 20.0f);
            ImGui::SliderFloat("Height fade", &gDebrisHeightFadeMax, 0.1f, 20.0f);
            ImGui::SliderFloat("Ground clip", &gDebrisGroundClip, 0.0f, 6.0f);

            if (ImGui::TreeNode("Position offset")) {
                ImGui::SliderFloat("Local pos X", &gDebrisPosOffsetX, -10.0f, 10.0f);
                ImGui::SliderFloat("Local pos Y", &gDebrisPosOffsetY, -6.0f, 6.0f);
                ImGui::SliderFloat("Local pos Z", &gDebrisPosOffsetZ, -10.0f, 10.0f);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Noise layers")) {
                ImGui::SliderFloat("Noise1 scale XZ", &gDebrisNoise1ScaleXZ, 0.01f, 0.6f);
                ImGui::SliderFloat("Noise1 scale Y", &gDebrisNoise1ScaleY, 0.01f, 0.6f);
                ImGui::SliderFloat("Noise1 amp", &gDebrisNoise1Amp, 0.0f, 4.0f);
                ImGui::SliderFloat("Noise1 speed X", &gDebrisNoise1SpeedX, 0.0f, 1.0f);
                ImGui::SliderFloat("Noise1 speed Z", &gDebrisNoise1SpeedZ, 0.0f, 1.0f);
                ImGui::Separator();
                ImGui::SliderFloat("Noise2 scale XZ", &gDebrisNoise2ScaleXZ, 0.01f, 0.8f);
                ImGui::SliderFloat("Noise2 scale Y", &gDebrisNoise2ScaleY, 0.01f, 0.8f);
                ImGui::SliderFloat("Noise2 amp", &gDebrisNoise2Amp, 0.0f, 3.0f);
                ImGui::SliderFloat("Noise2 speed X", &gDebrisNoise2SpeedX, 0.0f, 1.0f);
                ImGui::SliderFloat("Noise2 speed Z", &gDebrisNoise2SpeedZ, 0.0f, 1.0f);
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Storm & lightning")) {
            ImGui::SliderFloat("Storm mix", &gAtmStormMix, 0.0f, 1.0f);
            ImGui::ColorEdit3("Storm tint", &gStormTint.x);
            ImGui::Separator();
            ImGui::Checkbox("Auto lightning", &gAutoLightning);
            if (gAutoLightning) {
                ImGui::SliderFloat("Lightning intensity", &gLightningIntensity, 0.5f, 8.0f);
                ImGui::SliderFloat("Lightning frequency", &gLightningFrequency, 0.02f, 0.5f);
            } else {
                ImGui::SliderFloat("Lightning flash", &gLightningFlash, 0.0f, 8.0f);
            }
        }

        if (ImGui::CollapsingHeader("Cumulonimbus Cloud")) {
            ImGui::Checkbox("Enable cloud", &gEnableCb);
            ImGui::SliderFloat("Y offset (above tornado)", &gCbOffsetY, -20.0f, 60.0f);
            ImGui::SliderFloat("Extent X", &gCbExtentX, 20.0f, 300.0f);
            ImGui::SliderFloat("Extent Y (thickness)", &gCbExtentY, 2.0f, 40.0f);
            ImGui::SliderFloat("Extent Z", &gCbExtentZ, 20.0f, 300.0f);
            ImGui::SliderFloat("Density", &gCbDensityMul, 0.1f, 8.0f);
            ImGui::SliderFloat("Coverage", &gCbCoverage, 0.0f, 1.0f);
            ImGui::SliderFloat("Extinction", &gCbExtinction, 0.005f, 0.3f, "%.4f");
            ImGui::SliderFloat("Noise scale (lower = larger blobs)", &gCbNoiseScale, 0.0004f, 0.02f, "%.5f");
            ImGui::SliderFloat("Detail scale", &gCbDetailScale, 0.001f, 0.03f, "%.5f");
            ImGui::SliderFloat("Noise LOD (higher = softer)", &gCbNoiseLod, 0.0f, 5.5f, "%.2f");
            ImGui::SliderFloat("Coverage edge softness", &gCbEdgeSoftness, 0.05f, 2.0f, "%.3f");
            ImGui::SliderFloat("Noise speed", &gCbNoiseSpeed, 0.0f, 0.2f, "%.4f");
            ImGui::ColorEdit3("Cloud tint", &gCbTint.x);
            ImGui::ColorEdit3("Lit color (top)", &gCbLitCol.x);
            ImGui::ColorEdit3("Dark color (base)", &gCbDarkCol.x);
            ImGui::ColorEdit3("Sun edge color", &gCbSunEdgeCol.x);
        }

        if (ImGui::CollapsingHeader("Fog & Sky Blending")) {
            ImGui::SliderFloat("Skybox blend", &gSkyboxBlend, 0.0f, 1.0f);
            ImGui::SliderFloat("Fog density", &gFogDensity, 0.0f, 0.05f, "%.5f");
            ImGui::SliderFloat("Fog height falloff", &gFogHeightFalloff, 0.0f, 0.2f, "%.4f");
            ImGui::SliderFloat("Terrain fade start", &gTerrainFadeStart, 10.0f, 300.0f);
            ImGui::SliderFloat("Terrain fade end", &gTerrainFadeEnd, 10.0f, 400.0f);
        }

        if (ImGui::CollapsingHeader("Shadows")) {
            ImGui::SliderFloat("Softness", &gShadowSoftness, 4.0f, 24.0f);
            ImGui::SliderFloat("Min step (world)", &gShadowMinStep, 0.05f, 1.5f, "%.3f");
            ImGui::SliderInt("Max steps", &gShadowMaxSteps, 4, 32);
            gShadowMaxSteps = std::max(4, std::min(32, gShadowMaxSteps));
            ImGui::Combo("Quality", &gShadowQuality, "Soft penumbra\0Hard binary\0");
            gShadowQuality = std::max(0, std::min(1, gShadowQuality));
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

void shutdownImGui()
{
#ifdef TORNADO_ENABLE_IMGUI
    if (!gImGuiInitialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    gImGuiInitialized = false;
#endif
}

void volumeUiMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
#ifdef TORNADO_ENABLE_IMGUI
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#else
    (void)window; (void)button; (void)action; (void)mods;
#endif
}

void volumeUiKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#ifdef TORNADO_ENABLE_IMGUI
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
#else
    (void)window; (void)key; (void)scancode; (void)action; (void)mods;
#endif
}

void volumeUiCharCallback(GLFWwindow* window, unsigned int c)
{
#ifdef TORNADO_ENABLE_IMGUI
    ImGui_ImplGlfw_CharCallback(window, c);
#else
    (void)window; (void)c;
#endif
}

void volumeUiCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
#ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized) {
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    }
#else
    (void)window; (void)xpos; (void)ypos;
#endif
}

void volumeUiScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
#ifdef TORNADO_ENABLE_IMGUI
    if (gImGuiInitialized) {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }
#else
    (void)window; (void)xoffset; (void)yoffset;
#endif
}

bool volumeUiWantsMouseCapture()
{
#ifdef TORNADO_ENABLE_IMGUI
    return gImGuiInitialized && ImGui::GetIO().WantCaptureMouse;
#else
    return false;
#endif
}

bool volumeUiWantsKeyboardCapture()
{
#ifdef TORNADO_ENABLE_IMGUI
    return gImGuiInitialized && ImGui::GetIO().WantCaptureKeyboard;
#else
    return false;
#endif
}
