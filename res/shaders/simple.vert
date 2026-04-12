#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewModelMatrix;

out vec3 normal;
out vec3 fragPos;
out vec2 texCoords;
out mat3 TBN;

void main()
{
    fragPos = vec3(viewModelMatrix * vec4(aPos, 1.0f));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    normal = N;
    texCoords = aTexCoords;

    gl_Position = MVP * vec4(aPos, 1.0);
}
