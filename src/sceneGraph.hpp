#pragma once

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stack>
#include <vector>
#include <cstdio>
#include <stdbool.h>
#include <cstdlib> 
#include <ctime> 
#include <chrono>
#include <fstream>

enum SceneNodeType {
	GEOMETRY, POINT_LIGHT, SPOT_LIGHT, GEOMETRY_2D, NORMAL_MAP_GEOMETRY
};

struct SceneNode
{
	SceneNode() {
		position = glm::vec3(0.0f);
		scale = glm::vec3(1.0f);
		rotation = glm::vec3(0.0f);

		referencePoint = glm::vec3(0.0f);
		vertexArrayObjectID = -1;
		VAOIndexCount = 0;

		textureID = -1;
		normalMapTextureID = 0;
		roughnessMapTextureID = 0;
		textureArrayID = 0;
		diffuseUvScale = 1.0f;
		materialKind = 0;
		instanceCount = 0;
		useOrthoMvp = false;

		nodeType = GEOMETRY;
	}

	std::vector<SceneNode*> children;

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;

	glm::mat4 modelMatrix;
	glm::mat4 viewModelMatrix;
	glm::mat3 normalMatrix;

	glm::mat4 currentTransformationMatrix;

	glm::vec3 referencePoint;

	int vertexArrayObjectID;
	unsigned int VAOIndexCount;

	SceneNodeType nodeType;

	unsigned int lightID;
	unsigned int textureID;
	float diffuseUvScale;
	// 0 = default, 1 = terrain
	int materialKind;
	unsigned int normalMapTextureID;
	unsigned int roughnessMapTextureID;
	bool invertRoughness = false;

	unsigned int textureArrayID;
	int instanceCount;

	// GEOMETRY_2D: true = ortho * model (pixel-space HUD). false = perspective MVP (billboards).
	bool useOrthoMvp;
};

SceneNode* createSceneNode();
void addChild(SceneNode* parent, SceneNode* child);
void printNode(SceneNode* node);
int totalChildren(SceneNode* parent);
