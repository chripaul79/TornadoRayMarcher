#version 450 core

layout(location = 0) in vec2 aQuadPos;

layout(location = 1) in vec3 iWorldPos;
layout(location = 2) in vec4 iSizeLayerPad; // x = height, y = width ratio, z = layer, w = unused

uniform mat4 VP;
uniform mat4 viewMatrix;
uniform vec3 cameraPos;

out vec2 vUV;
out vec3 vFragPosView;
flat out float vTexLayer;

void main()
{
    float treeHeight = iSizeLayerPad.x;
    float treeWidth  = treeHeight * iSizeLayerPad.y;
    vTexLayer = iSizeLayerPad.z;

    vec2 quad = aQuadPos;
    vUV = quad * 0.5 + 0.5;

    vec3 toCam = cameraPos - iWorldPos;
    toCam.y = 0.0;
    float l = length(toCam);
    vec3 fwd = (l > 1e-5) ? (toCam / l) : vec3(0.0, 0.0, 1.0);
    vec3 right = vec3(-fwd.z, 0.0, fwd.x);
    vec3 up    = vec3(0.0, 1.0, 0.0);

    vec3 offset = right * (quad.x * treeWidth * 0.5)
                + up    * ((quad.y * 0.5 + 0.5) * treeHeight);

    vec3 worldPos = iWorldPos + offset;

    vFragPosView = vec3(viewMatrix * vec4(worldPos, 1.0));

    gl_Position = VP * vec4(worldPos, 1.0);
}
