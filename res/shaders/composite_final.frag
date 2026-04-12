#version 450 core

// Final composite: lit scene, cubemap / procedural sky, tornado volume, lightning

in  vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

uniform sampler2D sceneColor;
uniform sampler2D gDepth;
uniform sampler2D tornadoBuffer;
uniform samplerCube skybox;

uniform mat4  invVP;
uniform mat4  VP;
uniform vec3  cameraPos;
uniform vec3  sunDirectionWorld;
uniform vec2  fullResolution;

uniform float fogDensity;
uniform float fogHeightFalloff;
uniform float atmStormMix;
uniform vec3  stormTint;
uniform float uSkyboxBlend;

uniform float terrainFadeStart;
uniform float terrainFadeEnd;
uniform float lightningFlash;
uniform vec3  lightningBoltStart;
uniform vec3  lightningBoltEnd;
uniform float lightningBoltSeed;

float computeFogAmount(float dist, vec3 ro, vec3 rd)
{
    float b  = fogHeightFalloff;
    float c  = fogDensity;
    float dy = rd.y;
    float rh = ro.y;

    float fogAmt;
    if (abs(dy) > 1e-4)
        fogAmt = c * exp(-b * rh) * (1.0 - exp(-b * dy * dist)) / (b * dy);
    else
        fogAmt = c * exp(-b * rh) * dist;
    return clamp(fogAmt, 0.0, 1.0);
}

vec3 computeFogColor(vec3 rd, vec3 sunDir)
{
    float cosT  = clamp(dot(rd, sunDir), -1.0, 1.0);
    float sunFog = pow(max(cosT, 0.0), 4.0);
    vec3  fogSun = mix(vec3(0.55, 0.65, 0.75), vec3(1.00, 0.80, 0.55), sunFog);
    return mix(fogSun, stormTint * 0.6 + fogSun * 0.4, atmStormMix);
}

vec3 worldRayDir(vec2 uv)
{
    vec2 ndc  = uv * 2.0 - 1.0;
    vec4 pFar = invVP * vec4(ndc, 1.0, 1.0);
    vec3 dir  = pFar.xyz / max(abs(pFar.w), 1e-6) - cameraPos;
    float len = length(dir);
    return len > 1e-6 ? dir / len : vec3(0.0, 1.0, 0.0);
}

// Fallback gradient when cubemap blend is low (no Rayleigh/Mie; storm applied after mix).
vec3 proceduralSkyClear(vec3 rd, vec3 sunDir)
{
    float t = clamp(rd.y * 0.5 + 0.5, 0.0, 1.0);
    vec3  zenith = vec3(0.32, 0.52, 0.88);
    vec3  horizonCol = vec3(0.62, 0.70, 0.80);
    vec3  base = mix(horizonCol, zenith, pow(max(t, 0.0), 0.75));

    float cosSun = max(dot(rd, sunDir), 0.0);
    float sunDisk = pow(cosSun, 48.0) * 0.22;
    base += vec3(1.0, 0.95, 0.82) * sunDisk * (1.0 - atmStormMix);
    return base;
}

vec3 applyStormToSky(vec3 base, vec3 rd)
{
    float stormDarken = mix(1.0, 0.30, atmStormMix);
    vec3  stormed = mix(base, stormTint, atmStormMix * 0.65) * stormDarken;
    float horizRing = pow(clamp(1.0 - abs(rd.y), 0.0, 1.0), 3.0);
    return stormed * (1.0 - horizRing * 0.4 * atmStormMix);
}

vec3 compositeSky(vec3 rd, vec3 sunDir)
{
    vec3 env = texture(skybox, rd).rgb;
    vec3 clear = proceduralSkyClear(rd, sunDir);
    float b = clamp(uSkyboxBlend, 0.0, 1.0);
    vec3 mixed = mix(clear, env, b);
    return applyStormToSky(mixed, rd);
}

// ── Procedural lightning bolt ────────────────────────────────────────────────
float hash1(float n) { return fract(sin(n) * 43758.5453); }

vec2 worldToScreen(vec3 wp)
{
    vec4 clip = VP * vec4(wp, 1.0);
    if (clip.w < 0.001) return vec2(-10.0);
    vec2 ndc = clip.xy / clip.w;
    return ndc * 0.5 + 0.5;
}

float lightningBolt(vec2 uv, vec2 p0, vec2 p1, float seed)
{
    if (p0.x < -5.0 || p1.x < -5.0) return 0.0;

    const int SEGMENTS = 12;
    const int BRANCHES = 3;
    float brightness = 0.0;
    float aspect = fullResolution.x / max(fullResolution.y, 1.0);

    vec2 pts[13];
    pts[0] = p0;
    pts[SEGMENTS] = p1;

    vec2 dir = p1 - p0;
    vec2 perp = normalize(vec2(-dir.y, dir.x));

    for (int i = 1; i < SEGMENTS; ++i) {
        float t = float(i) / float(SEGMENTS);
        float jitter = (hash1(seed + float(i) * 17.31) - 0.5) * 0.06;
        jitter *= sin(t * 3.14159);
        pts[i] = mix(p0, p1, t) + perp * jitter;
    }

    for (int i = 0; i < SEGMENTS; ++i) {
        vec2 a = pts[i], b = pts[i + 1];
        vec2 seg = b - a;
        float segLen = length(seg * vec2(aspect, 1.0));
        if (segLen < 1e-6) continue;

        vec2 pa = (uv - a) * vec2(aspect, 1.0);
        vec2 sd = seg * vec2(aspect, 1.0);
        float proj = clamp(dot(pa, sd) / dot(sd, sd), 0.0, 1.0);
        float d = length(pa - sd * proj);

        float coreWidth = 0.0015;
        float glowWidth = 0.012;
        brightness += exp(-d * d / (coreWidth * coreWidth)) * 1.0;
        brightness += exp(-d * d / (glowWidth * glowWidth)) * 0.3;
    }

    for (int br = 0; br < BRANCHES; ++br) {
        float bt = hash1(seed + float(br) * 71.7) * 0.6 + 0.15;
        int si = int(bt * float(SEGMENTS));
        si = clamp(si, 0, SEGMENTS - 1);
        vec2 branchStart = pts[si];
        float angle = (hash1(seed + float(br) * 31.1) - 0.5) * 1.2;
        float brLen = length(p1 - p0) * (0.15 + hash1(seed + float(br) * 53.3) * 0.2);
        vec2 brDir = normalize(dir) + perp * angle;
        vec2 branchEnd = branchStart + normalize(brDir) * brLen;

        const int BR_SEGS = 5;
        for (int j = 0; j < BR_SEGS; ++j) {
            float t0 = float(j) / float(BR_SEGS);
            float t1 = float(j + 1) / float(BR_SEGS);
            vec2 a2 = mix(branchStart, branchEnd, t0);
            vec2 b2 = mix(branchStart, branchEnd, t1);
            float jit = (hash1(seed + float(br) * 100.0 + float(j) * 13.7) - 0.5) * 0.03;
            b2 += perp * jit * sin(t1 * 3.14159);

            vec2 seg2 = b2 - a2;
            float segLen2 = length(seg2 * vec2(aspect, 1.0));
            if (segLen2 < 1e-6) continue;

            vec2 pa2 = (uv - a2) * vec2(aspect, 1.0);
            vec2 sd2 = seg2 * vec2(aspect, 1.0);
            float proj2 = clamp(dot(pa2, sd2) / dot(sd2, sd2), 0.0, 1.0);
            float d2 = length(pa2 - sd2 * proj2);

            float fade = 1.0 - t0;
            brightness += exp(-d2 * d2 / (0.001 * 0.001)) * 0.5 * fade;
            brightness += exp(-d2 * d2 / (0.008 * 0.008)) * 0.15 * fade;
        }
    }

    return brightness;
}

void main()
{
    vec3  rd     = worldRayDir(TexCoords);
    vec3  sunDir = normalize(sunDirectionWorld);

    float depthNdc = texture(gDepth, TexCoords).r;
    float dist     = 1e6;
    if (depthNdc < 0.999999) {
        vec4 hitW  = invVP * vec4(TexCoords * 2.0 - 1.0, depthNdc * 2.0 - 1.0, 1.0);
        vec3 pos   = hitW.xyz / max(hitW.w, 1e-6);
        dist       = length(pos - cameraPos);
    }

    vec3 sky      = compositeSky(rd, sunDir);
    vec4 tornado  = texture(tornadoBuffer, TexCoords);
    vec3 scene    = texture(sceneColor,    TexCoords).rgb;

    float hasGeometry = depthNdc < 0.999999 ? 1.0 : 0.0;

    float fogAmt       = computeFogAmount(dist, cameraPos, rd);
    vec3  fogCol        = computeFogColor(rd, sunDir);
    vec3  sceneTinted   = mix(scene, fogCol, fogAmt * 0.7);

    float edgeFade      = 1.0 - smoothstep(terrainFadeStart, terrainFadeEnd, dist);
    float terrainAlpha  = hasGeometry * (1.0 - fogAmt) * edgeFade;

    vec3 base = mix(sky, sceneTinted, terrainAlpha);
    vec3 withTornado = base * (1.0 - clamp(tornado.a, 0.0, 1.0)) + tornado.rgb;

    float flash = clamp(lightningFlash, 0.0, 8.0);
    vec3 flashCol = vec3(0.85, 0.92, 1.0) * flash;
    withTornado += flashCol;

    if (flash > 0.05) {
        vec2 ss0 = worldToScreen(lightningBoltStart);
        vec2 ss1 = worldToScreen(lightningBoltEnd);
        float bolt = lightningBolt(TexCoords, ss0, ss1, lightningBoltSeed);
        withTornado += vec3(0.75, 0.85, 1.0) * bolt * flash;
    }

    FragColor = vec4(withTornado, 1.0);
}
