#pragma once

#include "libs.h"

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    /* Ability to present on surface */
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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
    

    void run();

private:
    /* Instance */
    VkInstance instance;

    /* Surface */
    VkSurfaceKHR surface;

    /* Device */
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    /* Queues */
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    /* Swap chain */
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    /* Swap chain image handles */
    std::vector<VkImage> swapChainImages;

    /* Image Views */
    std::vector<VkImageView> swapChainImageViews;

    std::vector<const char*> validationLayers;
    std::vector<const char*> deviceExtensions;
#ifdef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    /* Initialize Vulkan API */
    void initVulkan();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();

    /* Initialize GLFW */
    void initGLFW();
    void initWindow();

    /* Auxiliary Functions */
    bool checkValidationLayerSupport();
    bool isDeviceSuitable( VkPhysicalDevice device );
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
    VkShaderModule createShaderModule( const std::vector<char>& code );

    void mainLoop();
    void cleanup();
};

static std::vector<char> readFile( const std::string& filename )
{
    /* 
     * ate - Start reading from the end of file
     * binary - read the file as binary file (avoid text transformations)
     */
    std::ifstream file(filename, std::ios::ate || std::ios::binary );

    if( !file.is_open() )
        throw new std::runtime_error("Failed to open file!");

    size_t fileSize = (size_t) file.tellg();    /* Current position in the stream */
    std::vector<char> buffer(fileSize);

    file.seekg(0);  /* Move back to the first position */
    file.read( buffer.data(), fileSize);

    file.close();

    return buffer;
}