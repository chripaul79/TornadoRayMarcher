#version 450 core

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out float gAO;

in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;
in mat3 TBN;

layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D roughnessMap;

uniform int useDiffuse;
uniform int useNormalMap;
uniform int useRoughnessMap;
uniform int invertRoughness;
uniform float diffuseUvScale;
uniform int materialKind;
uniform float defaultRoughness;

void main()
{
    gPosition = vec4(fragPos, 1.0);

    vec2 tiledUv = texCoords * diffuseUvScale;

    vec3 N = normalize(normal);
    if (useNormalMap != 0) {
        vec3 mapN = texture(normalMap, tiledUv).rgb * 2.0 - 1.0;
        N = normalize(TBN * mapN);
    }

    float roughness = defaultRoughness;
    if (useRoughnessMap != 0) {
        roughness = texture(roughnessMap, tiledUv).r;
        if (invertRoughness != 0) roughness = 1.0 - roughness;
    }

    gNormal = vec4(N, roughness);

    vec4 textureColor = texture(diffuseMap, tiledUv);
    float terrainTag = (materialKind == 1) ? 1.0 : 0.0;

    if (useDiffuse != 0) {
        gAlbedo = vec4(textureColor.rgb, terrainTag);
    } else {
        gAlbedo = vec4(vec3(0.95), terrainTag);
    }

    gAO = 1.0;
}
