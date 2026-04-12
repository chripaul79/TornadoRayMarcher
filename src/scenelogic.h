#pragma once

#include <GLFW/glfw3.h>

#include <utilities/window.hpp>
#include "sceneGraph.hpp"

void updateNodeTransformations(SceneNode* node, glm::mat4 parentTransformation, glm::mat4 viewProjection, glm::mat4 viewMatrix);
void processInput(GLFWwindow* window);
void initTornado(GLFWwindow* window);
void shutdownTornado();
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);
void renderQuad();
void regenerateTrees();