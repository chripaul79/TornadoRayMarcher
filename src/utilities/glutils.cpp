#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#include <cmath>

template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}

unsigned int generateBuffer(Mesh &mesh) {
    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    generateAttribute(0, 3, mesh.vertices, false);
    if (mesh.normals.size() > 0) {
        generateAttribute(1, 3, mesh.normals, true);
    }
    if (mesh.textureCoordinates.size() > 0) {
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }

    const bool canBuildTangents =
        !mesh.vertices.empty() &&
        mesh.textureCoordinates.size() == mesh.vertices.size() &&
        mesh.indices.size() >= 3;

    if (canBuildTangents) {
        std::vector<glm::vec3> tangents(mesh.vertices.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> bitangents(mesh.vertices.size(), glm::vec3(0.0f));

        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            unsigned int i0 = mesh.indices[i + 0];
            unsigned int i1 = mesh.indices[i + 1];
            unsigned int i2 = mesh.indices[i + 2];

            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
                continue;
            }

            glm::vec3& v0 = mesh.vertices[i0];
            glm::vec3& v1 = mesh.vertices[i1];
            glm::vec3& v2 = mesh.vertices[i2];

            glm::vec2& uv0 = mesh.textureCoordinates[i0];
            glm::vec2& uv1 = mesh.textureCoordinates[i1];
            glm::vec2& uv2 = mesh.textureCoordinates[i2];

            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
            if (std::abs(det) < 1e-8f) {
                continue;
            }

            float r = 1.0f / det;
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;

            bitangents[i0] += bitangent;
            bitangents[i1] += bitangent;
            bitangents[i2] += bitangent;
        }

        generateAttribute(3, 3, tangents, true);
        generateAttribute(4, 3, bitangents, true);
    }

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

    return vaoID;
}

unsigned int generateTextureBuffer(PNGImage image) {

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());

	glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

unsigned int generateTextureBuffer(JPGImage image)
{
	PNGImage asPng;
	asPng.width = image.width;
	asPng.height = image.height;
	asPng.pixels = std::move(image.pixels);
	return generateTextureBuffer(asPng);
}
