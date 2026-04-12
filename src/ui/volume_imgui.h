#pragma once

#include <GLFW/glfw3.h>

void initImGui(GLFWwindow* window);
void drawImGuiOverlay();
void shutdownImGui();
void resetVolumeGuiDefaults();
void saveVolumeGuiSettings();
void loadVolumeGuiSettings();

void volumeUiMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void volumeUiKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void volumeUiCharCallback(GLFWwindow* window, unsigned int c);
void volumeUiCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void volumeUiScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

bool volumeUiWantsMouseCapture();
bool volumeUiWantsKeyboardCapture();
