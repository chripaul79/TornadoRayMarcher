#include "program.hpp"
#include "scenelogic.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>


void runProgram(GLFWwindow* window) 
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glClearColor(0.3f, 0.5f, 0.8f, 1.0f);
	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	initTornado(window);

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		updateFrame(window);

		renderFrame(window);

		glfwPollEvents();
		handleKeyboardInput(window);

		glfwSwapBuffers(window);
	}

	shutdownTornado();
}

void handleKeyboardInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}