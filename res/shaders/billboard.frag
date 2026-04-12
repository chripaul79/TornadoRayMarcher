#version 450 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out float gAO;

in vec2  vUV;
in vec3  vFragPosView;
flat in float vTexLayer;

layout(binding = 4) uniform sampler2DArray treeTex;
uniform float alphaCutoff;
uniform float treeAO;

void main()
{
    vec4 texel = texture(treeTex, vec3(vUV, vTexLayer));
    if (texel.a < alphaCutoff) discard;

    vec3 color = texel.rgb / max(texel.a, 0.001);

    gPosition = vec4(vFragPosView, 1.0);
    gNormal   = vec4(0.0, 1.0, 0.0, -1.0);  // w = -1 flags "unlit"
    gAlbedo   = vec4(color, 0.0);
    gAO       = treeAO;
}
