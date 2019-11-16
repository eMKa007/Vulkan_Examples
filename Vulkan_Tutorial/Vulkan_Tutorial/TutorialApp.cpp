
#include "libs.h"
#include "TutorialApp.h"



TutorialApp::TutorialApp( unsigned int windowWidth, unsigned int windowHeight, std::string windowName)
    : windowWidth(windowWidth), windowHeight(windowHeight), windowName(windowName)
{
    this->initVulkan();
}


TutorialApp::~TutorialApp()
{
}

void TutorialApp::run()
{
    this->mainLoop();
    this->cleanup();
}

void TutorialApp::initVulkan()
{
    this->initGLFW();
    this->initWindow();
}

void TutorialApp::initGLFW()
{
    /* Init GLFW */
	if( glfwInit() == GLFW_FALSE )
	{
		std::cout << "ERROR::GLFW_INIT_FAILED  \n";
		glfwTerminate();
	}
}

void TutorialApp::initWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->window = glfwCreateWindow(static_cast<int>(this->windowWidth), 
        static_cast<int>(this->windowHeight), 
        this->windowName.c_str(), 
        nullptr, 
        nullptr);

	if( this->window == nullptr)
	{
		std::cout << "ERROR::GLFW_WINDOW_INIT_FAILED  \n";
		glfwTerminate();
	}

}

void TutorialApp::mainLoop()
{
    while(!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
    }
}

void TutorialApp::cleanup()
{
    glfwDestroyWindow(this->window);
    glfwTerminate();
}
