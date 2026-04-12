#ifndef TORNADO_DENSITY_GLSL
#define TORNADO_DENSITY_GLSL

// Shared tornado SDF. Include after declaring noiseTex, uTime, uTornadoOrigin.

uniform float uTornadoHeight;
uniform float uRadiusTop;
uniform float uRadiusExp;
uniform float uPinchStrength;
uniform float uShapeNoiseScaleXZ;
uniform float uShapeNoiseScaleY;
uniform float uShapeNoiseSpeed;
uniform float uShapeNoiseAmount;
uniform float uWobbleAmp;
uniform float uWobbleSpeed;
uniform float uSwayAmp;
uniform float uTwistBaseTop;
uniform float uTwistBaseBottom;
uniform float uTwistJitterAmp1;
uniform float uTwistJitterAmp2;
uniform float uTwistJitterAmp3;
uniform float uTwistSpeedBase;
uniform float uTwistSpeedVar1;
uniform float uTwistSpeedVar2;
uniform float uFilamentTwist1;
uniform float uFilamentTwist2;

mat2 rot2(float a) { float s = sin(a), c = cos(a); return mat2(c, -s, s, c); }

vec2 tornadoSway(float y, float t)
{
    vec2 s  = vec2(sin(y * 0.07 + t * 1.30), cos(y * 0.05 + t * 0.97)) * 2.2;
         s += vec2(sin(y * 0.13 - t * 2.10), cos(y * 0.11 + t * 1.73)) * 1.1;
         s += vec2(sin(y * 0.21 + t * 0.67), cos(y * 0.19 - t * 1.41)) * 0.5;
    return s;
}

float tornadoSampleNoise(vec3 p)
{
    const mat3 R = mat3(
         0.9717,  0.2165, -0.0967,
        -0.1895,  0.9563,  0.2240,
         0.1420, -0.2046,  0.9684
    );
    vec3 uvw = R * (p * vec3(uShapeNoiseScaleXZ, uShapeNoiseScaleY, uShapeNoiseScaleXZ))
               + vec3(0.0, -uTime * uShapeNoiseSpeed, 0.0);
    return texture(noiseTex, uvw).r;
}

float tornadoRadius(float h)
{
    float r = mix(1.0, uRadiusTop, pow(h, uRadiusExp));
    float pinch = 1.0 - uPinchStrength * exp(-pow((h - 0.4) / 0.12, 2.0));
    return r * pinch;
}

float sampleFilament(vec3 p, float twistAngle)
{
    const mat3 R2 = mat3(
         0.9717, -0.1895,  0.1420,
         0.2165,  0.9563, -0.2046,
        -0.0967,  0.2240,  0.9684
    );

    vec3 p1  = vec3(rot2(twistAngle * uFilamentTwist1) * p.xz, p.y);
    vec3 uvw1 = R2 * (p1 * vec3(0.07, 0.014, 0.07))
                + vec3(0.0, -uTime * 0.10, 0.0);

    vec3 p2  = vec3(rot2(-twistAngle * uFilamentTwist2) * p.xz, p.y);
    vec3 uvw2 = R2 * (p2 * vec3(0.05, 0.010, 0.05))
                + vec3(0.0,  uTime * 0.07, 0.0);

    float n1 = texture(noiseTex, uvw1).r;
    float n2 = texture(noiseTex, uvw2).r;
    return n1 * 0.6 + n2 * 0.4;
}

float map(vec3 p)
{
    vec3 q = p - uTornadoOrigin;
    float h = clamp(q.y / max(uTornadoHeight, 0.001), 0.0, 1.0);

    float wobbleSpeed = uTime * uWobbleSpeed;
    float thinAnchor  = 1.0 - h * h;
    q.xz -= vec2(sin(q.y * 0.15 + wobbleSpeed),
                 cos(q.y * 0.10 + wobbleSpeed * 0.7)) * uWobbleAmp * thinAnchor;

    float swayAnchor = h * h;
    q.xz -= tornadoSway(q.y, uTime) * swayAnchor * uSwayAmp;

    float outerR = tornadoRadius(h);

    float spatialJitter = sin(q.y * 0.09) * uTwistJitterAmp1
                        + sin(q.y * 0.23) * uTwistJitterAmp2
                        + sin(q.y * 0.41) * uTwistJitterAmp3;
    float spatialTwist  = q.y * mix(uTwistBaseTop, uTwistBaseBottom, h) + spatialJitter;
    float angSpeedBase = uTwistSpeedBase + sin(uTime * 0.4) * uTwistSpeedVar1
                                       + sin(uTime * 1.1) * uTwistSpeedVar2;
    float angSpeed      = angSpeedBase / max(outerR, 0.8);
    float twistAngle = spatialTwist - uTime * angSpeed;
    vec2  twisted    = rot2(twistAngle) * q.xz;
    float r             = length(twisted);

    float dBase = r - outerR;
    dBase = max(dBase, -q.y);

    vec3 twistedP = vec3(twisted.x, q.y, twisted.y);

    float shape      = tornadoSampleNoise(twistedP);
    float shapeDelta = shape * uShapeNoiseAmount;
    shapeDelta *= smoothstep(0.0, 0.15, h);
    shapeDelta *= mix(1.0, 0.5, smoothstep(0.75, 1.0, h));

    float surfaceMask = smoothstep(-max(outerR * 0.35, 4.0), 1.5, dBase);
    float filament    = sampleFilament(vec3(twisted, q.y), twistAngle);
    float detailDelta = filament * 1.6 * surfaceMask;
    detailDelta *= smoothstep(0.0, 0.12, h);

    return dBase + shapeDelta + detailDelta;
}

// Cheaper version without filament detail
float mapShadow(vec3 p)
{
    vec3 q = p - uTornadoOrigin;
    float h = clamp(q.y / max(uTornadoHeight, 0.001), 0.0, 1.0);

    float wobbleSpeed = uTime * uWobbleSpeed;
    float thinAnchor  = 1.0 - h * h;
    q.xz -= vec2(sin(q.y * 0.15 + wobbleSpeed),
                 cos(q.y * 0.10 + wobbleSpeed * 0.7)) * uWobbleAmp * thinAnchor;

    float swayAnchor = h * h;
    q.xz -= tornadoSway(q.y, uTime) * swayAnchor * uSwayAmp;

    float outerR = tornadoRadius(h);

    float spatialJitter = sin(q.y * 0.09) * uTwistJitterAmp1
                        + sin(q.y * 0.23) * uTwistJitterAmp2
                        + sin(q.y * 0.41) * uTwistJitterAmp3;
    float spatialTwist  = q.y * mix(uTwistBaseTop, uTwistBaseBottom, h) + spatialJitter;
    float angSpeedBase = uTwistSpeedBase + sin(uTime * 0.4) * uTwistSpeedVar1
                                       + sin(uTime * 1.1) * uTwistSpeedVar2;
    float angSpeed      = angSpeedBase / max(outerR, 0.8);
    float twistAngle = spatialTwist - uTime * angSpeed;
    vec2  twisted    = rot2(twistAngle) * q.xz;
    float r             = length(twisted);

    float dBase = r - outerR;
    dBase = max(dBase, -q.y);

    vec3 twistedP = vec3(twisted.x, q.y, twisted.y);
    float shape      = tornadoSampleNoise(twistedP);
    float shapeDelta = shape * uShapeNoiseAmount;
    shapeDelta *= smoothstep(0.0, 0.15, h);
    shapeDelta *= mix(1.0, 0.5, smoothstep(0.75, 1.0, h));

    return dBase + shapeDelta;
}

#endif
