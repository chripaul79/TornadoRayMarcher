// TornadoSceneProject.cpp : Defines the entry point for the application.
//

#include "program.hpp"
#include <utilities/window.hpp>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>


#include <cstdlib>
#include <arrrgh.hpp>

using namespace std;

static void glfErrorCallback(int error, const char* description)
{
	cerr << "GLFW Error (" << error << "): " << description << endl;
}

GLFWwindow* initialize()
{
	if (!glfwInit())
	{
		cerr << "Failed to initialize GLFW" << endl;
		return nullptr;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwSetErrorCallback(glfErrorCallback);

	glfwWindowHint(GLFW_RESIZABLE, windowResizable);
	glfwWindowHint(GLFW_SAMPLES, windowSamples); // Enable multisampling for anti-aliasing

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);

	if (!window)
	{
		cerr << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return nullptr;
	}

	glfwMakeContextCurrent(window);
	gladLoadGL();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cerr << "Failed to initialize GLAD" << endl;
		return nullptr;
	}
	glViewport(0, 0, 800, 600);

	// Print various OpenGL information to stdout
	printf("%s: %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	printf("GLFW\t %s\n", glfwGetVersionString());
	printf("OpenGL\t %s\n", glGetString(GL_VERSION));
	printf("GLSL\t %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	return window;
}

int main()
{

	GLFWwindow* window = initialize();

	runProgram(window);

	glfwTerminate();

	return EXIT_SUCCESS;
}
