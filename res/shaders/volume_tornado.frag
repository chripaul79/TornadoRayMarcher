#version 450 core

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D litScene;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler3D noiseTex;
layout(binding = 3) uniform sampler2D blueNoiseTex;

uniform mat4  invVP;
uniform vec3  cameraPos;
uniform int   uMaxSteps;
uniform int   uEnableVolume;
uniform vec3  sunDirectionWorld;
uniform float uTornadoMaxRadius;
uniform float uExtinction;
uniform float uAlbedo;
uniform float uAnisotropy;
uniform float uSunIntensity;
uniform float uAmbientIntensity;
uniform float uDensityScale;
uniform float uStepSize;
uniform float uLightStepSize;
uniform int   uLightSteps;
uniform float uDebrisDensityMul;
uniform float uDebrisOpacity;
uniform float uDebrisAmbientMul;
uniform float uDebrisSunMul;

uniform float uTime;
uniform vec3  uTornadoOrigin;
uniform vec3  uTornadoTint;
uniform vec3  uDebrisTint;

#include "tornado_density.glsl"
#include "tornado_debris.glsl"

#define NOISE_TEX_SIZE   256.0
#define BLUE_NOISE_SIZE 1024.0
#define MAX_STEPS        196
#define PI                3.14159265

// ── Physical volume constants ─────────────────────────────────────────────────
const vec3  SUN_TINT   = vec3(1.00, 0.95, 0.82);
const vec3  AMBIENT_TINT = vec3(0.70, 0.74, 0.85);

// ── Cumulonimbus uniforms ────────────────────────────────────────────────────
uniform int   uEnableCb;
uniform vec3  uCbCenter;
uniform vec3  uCbExtent;
uniform float uCbDensityMul;
uniform float uCbNoiseScale;
uniform float uCbNoiseSpeed;
uniform float uCbExtinction;
uniform float uCbCoverage;
uniform float uCbDetailScale;
uniform float uCbNoiseLod;
uniform float uCbEdgeSoftness;
uniform vec3  uCbTint;
uniform vec3  uCbLitCol;
uniform vec3  uCbDarkCol;
uniform vec3  uCbSunEdgeCol;

// ── Cumulonimbus SDF — solid core, noise-displaced surface ───────────────────
float cbMap(vec3 p)
{
    if (uEnableCb == 0) return 1e6;

    vec3 local = (p - uCbCenter) / max(uCbExtent, vec3(0.01));
    float dist = length(local) - 1.0;
    float minE = min(uCbExtent.x, min(uCbExtent.y, uCbExtent.z));

    vec3 wind = vec3(uTime * uCbNoiseSpeed * 0.15, 0.0, uTime * uCbNoiseSpeed * 0.12);
    float lodMain = clamp(uCbNoiseLod, 0.0, 6.0);

    float n1 = textureLod(noiseTex, p * uCbNoiseScale + wind, lodMain).r;
    float n2 = textureLod(noiseTex, p * uCbDetailScale + wind * 0.5,
                          min(lodMain + 1.5, 6.0)).r;
    float n = n1 * 0.8 + n2 * 0.2;

    float displacement = n * uCbEdgeSoftness * minE;
    return dist * minE - displacement;
}

vec3 cloudAlbedoColour(float density, float h);
float lightMarch(vec3 p);

// ── Helpers ───────────────────────────────────────────────────────────────────
vec2 aabbIntersect(vec3 ro, vec3 rd, vec3 bMin, vec3 bMax)
{
    vec3 invD  = 1.0 / rd;
    vec3 tNear = min((bMin - ro) * invD, (bMax - ro) * invD);
    vec3 tFar  = max((bMin - ro) * invD, (bMax - ro) * invD);
    return vec2(max(max(tNear.x, tNear.y), tNear.z),
                min(min(tFar.x,  tFar.y),  tFar.z));
}

// ── Bicubic filtering ─────────────────────────────────────────────────────────
vec4 cubicWeights(float t)
{
    float t2 = t * t, t3 = t2 * t;
    return vec4(-0.5*t3 + 1.0*t2 - 0.5*t,
                 1.5*t3 - 2.5*t2 + 1.0,
                -1.5*t3 + 2.0*t2 + 0.5*t,
                 0.5*t3 - 0.5*t2);
}

float bicubic2D(sampler2D tex, vec2 uv, vec2 texSize)
{
    vec2 px = uv * texSize - 0.5, fr = fract(px);
    vec2 base = floor(px) / texSize, st = 1.0 / texSize;
    vec4 wx = cubicWeights(fr.x), wy = cubicWeights(fr.y);
    vec2 ox = vec2(wx.x+wx.y, wx.z+wx.w), oy = vec2(wy.x+wy.y, wy.z+wy.w);
    vec2 fx = vec2((wx.y/ox.x)-1.0, (wx.w/ox.y)+1.0)*st.x;
    vec2 fy = vec2((wy.y/oy.x)-1.0, (wy.w/oy.y)+1.0)*st.y;
    return ox.x*(oy.x*texture(tex,base+vec2(fx.x,fy.x)).r + oy.y*texture(tex,base+vec2(fx.x,fy.y)).r)
          +ox.y*(oy.x*texture(tex,base+vec2(fx.y,fy.x)).r + oy.y*texture(tex,base+vec2(fx.y,fy.y)).r);
}

// ── Debris shading ───────────────────────────────────────────────────────────
vec4 shadeDebris(vec3 p, float density, float sdf)
{
    const vec2 e = vec2(0.3, 0.0);
    vec3 n = normalize(vec3(
        debrisShellSDF(p + e.xyy) - debrisShellSDF(p - e.xyy),
        debrisShellSDF(p + e.yxy) - debrisShellSDF(p - e.yxy),
        debrisShellSDF(p + e.yyx) - debrisShellSDF(p - e.yyx)
    ));

    float diff  = clamp(dot(n, sunDirectionWorld), 0.0, 1.0);

    float h = clamp((p.y - uTornadoOrigin.y) / max(uTornadoHeight, 0.001), 0.0, 1.0);
    vec3 debrisCol = uDebrisTint * mix(0.85, 1.0, h);

    float shadowT = lightMarch(p);
    vec3 lin = AMBIENT_TINT * uAmbientIntensity * uDebrisAmbientMul
             + SUN_TINT * uSunIntensity * diff * uDebrisSunMul * shadowT;

    float alpha = clamp(density * uDebrisOpacity, 0.0, 1.0);
    vec4 color = vec4(debrisCol * lin * alpha, alpha);
    return color;
}

// ── Henyey-Greenstein phase function ─────────────────────────────────────────
float henyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(abs(1.0 + g2 - 2.0 * g * cosTheta), 1.5));
}

float phase(float cosTheta)
{
    float g = clamp(uAnisotropy, -0.95, 0.95);
    float forward  = henyeyGreenstein(cosTheta,  g);
    float backward = henyeyGreenstein(cosTheta, -g * 0.6);
    return mix(forward, backward, 0.3);
}

// ── Cloud colour palette (tornado) ───────────────────────────────────────────
vec3 cloudAlbedoColour(float density, float h)
{
    vec3 litCol  = vec3(0.97, 0.97, 0.98);
    vec3 midCol  = vec3(0.78, 0.80, 0.84);
    vec3 darkCol = vec3(0.42, 0.44, 0.50);
    vec3 baseCol = mix(darkCol, midCol, smoothstep(0.0, 0.6, h));
    return mix(litCol, baseCol, density) * uTornadoTint;
}

// ── Cumulonimbus albedo — sun-influenced ─────────────────────────────────────
vec3 cbAlbedo(vec3 p)
{
    float vert = clamp((p.y - (uCbCenter.y - uCbExtent.y)) / max(uCbExtent.y * 2.0, 0.01), 0.0, 1.0);
    vec3 fromCenter = normalize(p - uCbCenter + vec3(0.0, 0.01, 0.0));
    float sunFacing = clamp(dot(fromCenter, sunDirectionWorld), 0.0, 1.0);

    vec3 base = mix(uCbDarkCol, uCbLitCol, smoothstep(0.0, 0.55, vert));
    base = mix(base, uCbSunEdgeCol, sunFacing * 0.4);
    base *= uCbTint;
    return base;
}

// ── Light march — tornado + debris + cumulonimbus ────────────────────────────
float lightMarch(vec3 p)
{
    float opticalDepth = 0.0;
    int lightSteps = clamp(uLightSteps, 1, 64);
    float lightStep = clamp(uLightStepSize, 0.05, 8.0);

    for (int i = 0; i < 64; i++)
    {
        if (i >= lightSteps) break;
        vec3  lp  = p + sunDirectionWorld * (float(i) + 0.5) * lightStep;

        float sdf = map(lp);
        if (sdf < 0.0)
        {
            opticalDepth += clamp(-sdf * uExtinction, 0.0, 1.0) * lightStep;
            if (opticalDepth > 8.0) break;
        }

        if (debrisInBounds(lp))
        {
            float sdfDeb = debrisSDF(lp);
            if (sdfDeb < 0.0)
            {
                float debDensity = clamp(-sdfDeb * max(uDensityScale, 0.01), 0.0, 1.0);
                const float kDebrisLightOcc = 0.22;
                opticalDepth += clamp(debDensity * uExtinction * uDebrisDensityMul * kDebrisLightOcc, 0.0, 1.0) * lightStep;
                if (opticalDepth > 8.0) break;
            }
        }

        float cbSdf = cbMap(lp);
        if (cbSdf < 0.0)
        {
            float cbDens = clamp(-cbSdf * uCbDensityMul * 0.08, 0.0, 1.0);
            opticalDepth += cbDens * uCbExtinction * lightStep;
            if (opticalDepth > 8.0) break;
        }
    }

    float beer   = exp(-opticalDepth);
    float powder = 1.0 - exp(-opticalDepth * 1.5);
    return beer * powder * 2.2;
}

// ── Unified ray march — tornado + debris + cumulonimbus ──────────────────────
vec4 rayMarch(vec3 ro, vec3 rd, float tMin, float tMax, float offset)
{
    vec3  scatteredLight    = vec3(0.0);
    float viewTransmittance = 1.0;

    float ph = phase(dot(-rd, sunDirectionWorld));

    float stepSize = clamp(uStepSize, 0.04, 1.0);
    int stepCount = clamp(uMaxSteps, 8, MAX_STEPS);
    float depth = tMin + offset * stepSize;

    for (int i = 0; i < MAX_STEPS; i++)
    {
        if (i >= stepCount || depth > tMax || viewTransmittance < 0.02) break;

        vec3  p      = ro + rd * depth;
        float sdfVol = map(p);
        float sdfDeb = debrisInBounds(p) ? debrisSDF(p) : 1e6;
        float sdfCb  = cbMap(p);
        float sdf    = min(min(sdfVol, sdfDeb), sdfCb);

        if (sdf < 0.2)
        {
            bool isDebris = sdfDeb <= sdfVol && sdfDeb <= sdfCb;

            if (isDebris)
            {
                float localSDF = sdfDeb;
                float density  = clamp(-localSDF * max(uDensityScale, 0.01), 0.0, 1.0);
                if (density > 0.0005) {
                    vec4 debCol = shadeDebris(p, density, localSDF);
                    scatteredLight += viewTransmittance * debCol.rgb;
                    viewTransmittance *= (1.0 - debCol.a);
                }
                depth += stepSize * (viewTransmittance < 0.1 ? 2.5 : 1.0);
            }
            else
            {
                float torDens = clamp(-sdfVol * max(uDensityScale, 0.01), 0.0, 1.0);
                float cbDens  = clamp(-sdfCb  * uCbDensityMul * 0.08,    0.0, 1.0);

                float blendZone = 8.0;
                float cbW = 0.0;
                if (cbDens > 0.001 && torDens > 0.001) {
                    cbW = smoothstep(0.0, blendZone, sdfVol - sdfCb);
                } else if (cbDens > 0.001) {
                    cbW = 1.0;
                }
                float torW = 1.0 - cbW;

                float density  = torDens * torW + cbDens * cbW;
                if (density > 0.0005)
                {
                    float h = clamp((p.y - uTornadoOrigin.y) / max(uTornadoHeight, 0.001), 0.0, 1.0);

                    float sigmaT_tor = uExtinction * torDens;
                    float sigmaT_cb  = uCbExtinction * cbDens;
                    float sigmaT     = sigmaT_tor * torW + sigmaT_cb * cbW;
                    float sigmaS_tor = sigmaT_tor * clamp(uAlbedo, 0.0, 1.0);
                    float sigmaS_cb  = sigmaT_cb * 0.92;
                    float sigmaS     = sigmaS_tor * torW + sigmaS_cb * cbW;

                    float curStep = stepSize * mix(1.0, 5.0, cbW);
                    float stepT   = exp(-sigmaT * curStep);

                    vec3 torAlbedo = cloudAlbedoColour(torDens, h);
                    vec3 cbAlb     = cbAlbedo(p);
                    vec3 albedo    = torAlbedo * torW + cbAlb * cbW;

                    float shadowT = (viewTransmittance > 0.1) ? lightMarch(p) : 0.3;

                    vec3 sunLight = SUN_TINT * uSunIntensity * shadowT * ph * sigmaS;
                    float ambScale = mix(1.0, 0.5, cbW);
                    vec3 ambLight = AMBIENT_TINT * uAmbientIntensity * sigmaS * ambScale;
                    vec3 Sint = (sunLight + ambLight) * albedo
                              * (1.0 - stepT) / max(sigmaT, 1e-6);

                    scatteredLight    += viewTransmittance * Sint;
                    viewTransmittance *= stepT;
                }
                depth += stepSize * mix(1.0, 5.0, cbW)
                       * (viewTransmittance < 0.1 ? 2.5 : 1.0);
            }
        }
        else
        {
            depth += sdf * 0.65;
        }
    }

    float alpha = 1.0 - viewTransmittance;
    return vec4(scatteredLight, alpha);
}

// ── Main ──────────────────────────────────────────────────────────────────────
void main()
{
    if (uEnableVolume == 0) { FragColor = vec4(0.0); return; }

    float depthNdc = texture(gDepth, TexCoords).r;

    vec4 nearW4 = invVP * vec4(TexCoords * 2.0 - 1.0, -1.0, 1.0);
    vec4 farW4  = invVP * vec4(TexCoords * 2.0 - 1.0,  1.0, 1.0);
    vec3 rd = normalize(farW4.xyz / farW4.w - nearW4.xyz / nearW4.w);
    vec3 ro = cameraPos;

    float tMax = 600.0;
    if (depthNdc < 0.999999) {
        vec4 w = invVP * vec4(TexCoords * 2.0 - 1.0, depthNdc * 2.0 - 1.0, 1.0);
        tMax = length(w.xyz / w.w - ro);
    }

    vec2  noiseUV = gl_FragCoord.xy / BLUE_NOISE_SIZE;
    float jitter  = bicubic2D(blueNoiseTex, noiseUV, vec2(BLUE_NOISE_SIZE));
    float offset  = fract(jitter + mod(uTime, 64.0) * 0.6180339887);

    vec3 bMin = uTornadoOrigin + vec3(-uTornadoMaxRadius, -0.5, -uTornadoMaxRadius);
    vec3 bMax = uTornadoOrigin + vec3( uTornadoMaxRadius, uTornadoHeight + 2.0, uTornadoMaxRadius);
    if (uEnableCb != 0) {
        bMin = min(bMin, uCbCenter - uCbExtent);
        bMax = max(bMax, uCbCenter + uCbExtent);
    }

    vec4 volumeCol = vec4(0.0);
    vec2 box = aabbIntersect(ro, rd, bMin, bMax);
    if (!(box.y < box.x || box.y < 0.0)) {
        float tEnter = max(box.x, 0.0);
        float tExit  = min(box.y, tMax);
        volumeCol = rayMarch(ro, rd, tEnter, tExit, offset);
        volumeCol.a = clamp(volumeCol.a, 0.0, 1.0);
    }

    FragColor = vec4(volumeCol.rgb, clamp(volumeCol.a, 0.0, 1.0));
}
