#include "sceneGraph.hpp"
#include <iostream>

SceneNode* createSceneNode()
{
	SceneNode* node = new SceneNode();
	return node;
}
void addChild(SceneNode* parent, SceneNode* child)
{
	parent->children.push_back(child);
}

int totalChildren(SceneNode* node)
{
	int count = node->children.size();
	for (SceneNode* child : node->children)
	{
		count += totalChildren(child);
	}
	return count;
}

void printNode(SceneNode* node)
{
	printf(
		"SceneNode {\n"
		"    Child count: %i\n"
		"    Rotation: (%f, %f, %f)\n"
		"    Location: (%f, %f, %f)\n"
		"    Reference point: (%f, %f, %f)\n"
		"    VAO ID: %i\n"
		"}\n",
		int(node->children.size()),
		node->rotation.x, node->rotation.y, node->rotation.z,
		node->position.x, node->position.y, node->position.z,
		node->referencePoint.x, node->referencePoint.y, node->referencePoint.z,
		node->vertexArrayObjectID);
}
