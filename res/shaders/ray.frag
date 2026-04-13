#version 450 core

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D aoTexture;
layout(binding = 4) uniform sampler2D gDepth;

layout(binding = 5) uniform sampler3D noiseTex;
uniform float uTime;
uniform vec3 uTornadoOrigin;
uniform float u_shadowSoftness;
uniform float u_shadowMinStep;
uniform int u_shadowMaxSteps;
uniform int u_shadowQuality;
uniform float uDebrisOpacity;

#include "tornado_density.glsl"
#include "tornado_debris.glsl"

#define MAX_SHADOW_STEPS 32
#define SHADOW_BIAS 0.05

uniform mat4 invView;

struct Light {
    vec3 direction;
    vec3 color;
};

uniform Light light;
uniform int debugView;
uniform vec3 sunDirectionWorld;

float mapShadowCombined(vec3 p)
{
    float dCloud = mapShadow(p);
    if (uDebrisOpacity < 0.001)
        return dCloud;
    if (!debrisInBounds(p))
        return dCloud;

    // Outward bias so shadow isn’t fully opaque inside the debris shell, we basically fake light leakage for the debris
    float relax = max(uDebrisRadiusMinor, 0.2) * 1.35;
    float dDeb = debrisShellSDF(p) + relax;
    return min(dCloud, dDeb);
}

float tornadoShadow(vec3 surfacePos, vec3 lightDir) {
    vec3 rd = normalize(lightDir);
    int stepLimit = clamp(u_shadowMaxSteps, 4, MAX_SHADOW_STEPS);
    float minStep = max(u_shadowMinStep, 1e-4);

    float penumbra = 1.0;
    vec3 p = surfacePos + rd * SHADOW_BIAS;
    float t = SHADOW_BIAS;
    for (int i = 0; i < MAX_SHADOW_STEPS; ++i) {
        if (i >= stepLimit)
            break;
        float d = mapShadowCombined(p);
        if (d <= 0.0)
            return 0.0;
        if (u_shadowQuality == 0)
            penumbra = min(penumbra, u_shadowSoftness * d / max(t, 1e-4)); // Inigo Quilez soft shadow technique
        float stepLen = max(d, minStep);
        p += rd * stepLen;
        t += stepLen;
    }
    if (u_shadowQuality != 0)
        return 1.0;
    return clamp(penumbra, 0.0, 1.0);
}

vec3 getSkyColor(vec3 viewDir) {
    float t = clamp(viewDir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 horizon = vec3(0.74, 0.83, 0.95);
    vec3 zenith = vec3(0.36, 0.60, 0.90);
    return mix(horizon, zenith, t);
}

void main() {
    float depth = texture(gDepth, TexCoords).r;
    vec3 fragPosV = texture(gPosition, TexCoords).xyz;
    vec4 normalSample = texture(gNormal, TexCoords);
    vec3 normalV = normalize(normalSample.xyz);
    float roughness = normalSample.w;
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    float ao = texture(aoTexture, TexCoords).r;

    if (debugView == 1) {
        FragColor = vec4(abs(fragPosV) * 0.05, 1.0);
        return;
    }
    if (debugView == 2) {
        FragColor = vec4(abs(normalV), 1.0);
        return;
    }
    if (debugView == 3) {
        FragColor = vec4(albedo, 1.0);
        return;
    }
    if (debugView == 4) {
        FragColor = vec4(vec3(ao), 1.0);
        return;
    }
    if (debugView == 5) {
        FragColor = vec4(vec3(roughness), 1.0);
        return;
    }

    if (depth >= 0.999999) {
        FragColor = vec4(getSkyColor(vec3(0.0, 0.25, 0.0)), 1.0);
        return;
    }

    if (roughness < 0.0) {
        FragColor = vec4(albedo * ao, 1.0);
        return;
    }

    vec3 L = normalize(-light.direction);
    vec3 V = normalize(-fragPosV);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(normalV, L), 0.0);
    float NdotH = max(dot(normalV, H), 0.0);
    float NdotV = max(dot(normalV, V), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 worldPos = (invView * vec4(fragPosV, 1.0)).xyz; // Terrain pixel
    vec3 Lw = normalize(-sunDirectionWorld);

    float torShadow = 1.0;
    if (NdotL > 0.0)
        torShadow = tornadoShadow(worldPos, Lw); // Every pixel facing the sun gets shadow test

    vec3 F0 = vec3(0.04);

    // ── Blinn-Phong shading ──────────────────────────────────────────────────
    float shininess = mix(128.0, 4.0, clamp(roughness, 0.0, 1.0));

    float specStrength = mix(0.6, 0.05, clamp(roughness, 0.0, 1.0));

    float specFactor = pow(NdotH, shininess);
    vec3  specular   = vec3(specStrength) * specFactor;

    vec3 diffuse = albedo / 3.14159265;

    vec3 ambient = albedo * 0.18 * ao;
    vec3 color   = ambient + (diffuse + specular) * light.color * NdotL * torShadow;
    FragColor = vec4(color, 1.0);
}
