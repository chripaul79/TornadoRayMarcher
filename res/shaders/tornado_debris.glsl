#ifndef TORNADO_DEBRIS_GLSL
#define TORNADO_DEBRIS_GLSL

// Requires tornado_density.glsl

uniform float uDebrisOrbitSpeed;
uniform float uDebrisInnerOrbitSpeed;
uniform float uDebrisRadiusBottomAdd;
uniform float uDebrisRadiusTopAdd;
uniform float uDebrisRadiusMinor;
uniform float uDebrisVerticalOffset;
uniform float uDebrisNoise1ScaleXZ;
uniform float uDebrisNoise1ScaleY;
uniform float uDebrisNoise1Amp;
uniform float uDebrisNoise1SpeedX;
uniform float uDebrisNoise1SpeedZ;
uniform float uDebrisNoise2ScaleXZ;
uniform float uDebrisNoise2ScaleY;
uniform float uDebrisNoise2Amp;
uniform float uDebrisNoise2SpeedX;
uniform float uDebrisNoise2SpeedZ;
uniform float uDebrisHeightFadeMax;
uniform float uDebrisGroundClip;
uniform vec3  uDebrisPosOffset;
uniform float uDebrisHeightMin;
uniform float uDebrisHeightMax;

bool debrisInBounds(vec3 p)
{
    const float debrisAnchorY = 10.0;
    vec3 base = p - (uTornadoOrigin + uDebrisPosOffset);
    float yLocal = (base.y - debrisAnchorY) + uDebrisVerticalOffset;

    float hMin = min(uDebrisHeightMin, uDebrisHeightMax);
    float hMax = max(uDebrisHeightMin, uDebrisHeightMax);
    float fade = max(uDebrisHeightFadeMax, 0.001);
    if (yLocal < hMin - fade || yLocal > hMax + fade) return false;

    float worldY = debrisAnchorY + yLocal;
    float hR = clamp(worldY / max(uTornadoHeight, 0.001), 0.0, 1.0);
    float profileR = tornadoRadius(hR);
    float maxR = profileR + max(uDebrisRadiusBottomAdd, uDebrisRadiusTopAdd)
               + uDebrisNoise1Amp + uDebrisNoise2Amp + 2.0;

    float r2 = base.x * base.x + base.z * base.z;
    return r2 < maxR * maxR;
}

float debrisShellSDF(vec3 p)
{
    const float debrisAnchorY = 10.0;
    vec3 base = p - (uTornadoOrigin + uDebrisPosOffset);
    float yProfile = base.y;
    float hProfile = clamp(yProfile / max(uTornadoHeight, 0.001), 0.0, 1.0);
    float wobbleSpeed = uTime * uWobbleSpeed;
    float thin = 1.0 - hProfile * hProfile;
    vec2 wobble = vec2(sin(yProfile * 0.15 + wobbleSpeed),
                       cos(yProfile * 0.10 + wobbleSpeed * 0.7)) * uWobbleAmp * thin;
    vec2 sway = tornadoSway(yProfile, uTime) * (hProfile * hProfile) * uSwayAmp;
    vec2 centerXZ = wobble + sway;

    vec3 q = vec3(base.x - centerXZ.x, base.y - debrisAnchorY, base.z - centerXZ.y);
    q.xz = rot2(uTime * uDebrisOrbitSpeed) * q.xz;

    float yLocal = q.y + uDebrisVerticalOffset;
    float hMin = min(uDebrisHeightMin, uDebrisHeightMax);
    float hMax = max(uDebrisHeightMin, uDebrisHeightMax);
    float hSpan = max(hMax - hMin, 0.001);
    float hN = clamp((yLocal - hMin) / hSpan, 0.0, 1.0);

    float hR = clamp((debrisAnchorY + yLocal) / max(uTornadoHeight, 0.001), 0.0, 1.0);
    float profileR = tornadoRadius(hR);
    float outerR = mix(max(profileR + uDebrisRadiusBottomAdd, 0.2),
                       max(profileR + uDebrisRadiusTopAdd, 0.05), hN);
    float innerR = max(outerR - max(uDebrisRadiusMinor, 0.02), 0.01);

    float radial = length(q.xz);
    float d = max(radial - outerR, innerR - radial);

    // Fade shell outside height band
    float below = smoothstep(0.0, max(uDebrisHeightFadeMax, 0.001), hMin - yLocal);
    float above = smoothstep(0.0, max(uDebrisHeightFadeMax, 0.001), yLocal - hMax);
    float outBand = clamp(below + above, 0.0, 1.0);
    d = mix(d, abs(d) + 10.0, outBand);

    d = max(d, -yLocal - uDebrisGroundClip);
    return d;
}

float debrisSDF(vec3 p)
{
    const float debrisAnchorY = 10.0;
    vec3 base = p - (uTornadoOrigin + uDebrisPosOffset);

    float yProfile = base.y;
    float hProfile = clamp(yProfile / max(uTornadoHeight, 0.001), 0.0, 1.0);
    float wobbleSpeed = uTime * uWobbleSpeed;
    float thin = 1.0 - hProfile * hProfile;
    vec2 wobble = vec2(
        sin(yProfile * 0.15 + wobbleSpeed),
        cos(yProfile * 0.10 + wobbleSpeed * 0.7)
    ) * uWobbleAmp * thin;

    float swayW = hProfile * hProfile;
    vec2 sway = tornadoSway(yProfile, uTime) * swayW * uSwayAmp;

    vec2 centerXZ = wobble + sway;

    vec3 q = vec3(base.x - centerXZ.x, base.y - debrisAnchorY, base.z - centerXZ.y);

    float orbitAngle = uTime * uDebrisOrbitSpeed;
    q.xz = rot2(orbitAngle) * q.xz;

    vec2 innerXZ = rot2(-uTime * uDebrisInnerOrbitSpeed) * q.xz;

    float yLocal = q.y + uDebrisVerticalOffset;
    float hMin = min(uDebrisHeightMin, uDebrisHeightMax);
    float hMax = max(uDebrisHeightMin, uDebrisHeightMax);
    float hSpan = max(hMax - hMin, 0.001);
    float hN = clamp((yLocal - hMin) / hSpan, 0.0, 1.0);

    float worldY = debrisAnchorY + yLocal;
    float hR = clamp(worldY / max(uTornadoHeight, 0.001), 0.0, 1.0);
    float profileR = tornadoRadius(hR);

    float outerBottom = max(profileR + uDebrisRadiusBottomAdd, 0.2);
    float outerTop    = max(profileR + uDebrisRadiusTopAdd, 0.05);
    float outerR = mix(outerBottom, outerTop, hN);

    float shellThickness = max(uDebrisRadiusMinor, 0.02);
    float innerR = max(outerR - shellThickness, 0.01);

    float radial = length(q.xz);
    float dOuter = radial - outerR;
    float dInner = innerR - radial;
    float d = max(dOuter, dInner);

    vec3 noiseP = vec3(q.xz, q.y) * vec3(uDebrisNoise1ScaleXZ, uDebrisNoise1ScaleY, uDebrisNoise1ScaleXZ)
                + vec3(uTime * uDebrisNoise1SpeedX, 0.0, uTime * uDebrisNoise1SpeedZ);
    float n = texture(noiseTex, noiseP).r;
    d += n * uDebrisNoise1Amp;

    vec3 noiseP2 = vec3(innerXZ, q.y) * vec3(uDebrisNoise2ScaleXZ, uDebrisNoise2ScaleY, uDebrisNoise2ScaleXZ)
                 - vec3(uTime * uDebrisNoise2SpeedX, 0.0, uTime * uDebrisNoise2SpeedZ);
    float n2 = texture(noiseTex, noiseP2).r;
    d += n2 * uDebrisNoise2Amp;

    float below = smoothstep(0.0, max(uDebrisHeightFadeMax, 0.001), hMin - yLocal);
    float above = smoothstep(0.0, max(uDebrisHeightFadeMax, 0.001), yLocal - hMax);
    float outBand = clamp(below + above, 0.0, 1.0);
    d = mix(d, abs(d) + 10.0, outBand);

    d = max(d, -yLocal - uDebrisGroundClip);

    return d;
}

#endif
