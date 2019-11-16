#pragma once

#include "libs.h"

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value();
    }
};


class TutorialApp
{
public:
    TutorialApp( unsigned int windowHeight, unsigned int windowWidth, std::string windowName);
    ~TutorialApp();

    unsigned int windowWidth;
    unsigned int windowHeight;
    std::string windowName;
    GLFWwindow* window;
    std::vector<const char*> validationLayers;

    void run();

private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> devices;


#ifdef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    void initVulkan();
    void createInstance();
    void pickPhysicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void initGLFW();
    void initWindow();

    bool checkValidationLayerSupport();
    bool isDeviceSuitable( VkPhysicalDevice device );

    void mainLoop();

    void cleanup();
};

