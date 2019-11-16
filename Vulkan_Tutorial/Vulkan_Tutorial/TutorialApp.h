#pragma once

class TutorialApp
{
public:
    TutorialApp( unsigned int windowHeight, unsigned int windowWidth, std::string windowName);
    ~TutorialApp();

    unsigned int windowWidth;
    unsigned int windowHeight;
    std::string windowName;
    GLFWwindow* window;

    void run();

private:
    void initVulkan();
    void initGLFW();
    void initWindow();

    void mainLoop();

    void cleanup();
};

