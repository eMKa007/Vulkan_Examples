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

struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() 
    {
        /* Structure describing data rate */
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding      = 0;                            /* Specifies the index of binding in the array of bindings. */
        bindingDescription.stride       = sizeof(Vertex);               /* Number of bytes from one entry to next one. */
        bindingDescription.inputRate    = VK_VERTEX_INPUT_RATE_VERTEX;  /* RATE_VERTEX means: move to next data entry after each vertex. */

        return bindingDescription;
    }
    
    /* An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data. 
    *  Thats why we have here 2 structures- one description for position, one description for color.
    *  Corresponding formats of data:
    *       float: VK_FORMAT_R32_SFLOAT
    *       vec2: VK_FORMAT_R32G32_SFLOAT
    *       vec3: VK_FORMAT_R32G32B32_SFLOAT
    *       vec4: VK_FORMAT_R32G32B32A32_SFLOAT
    *       ivec2: VK_FORMAT_R32G32_SINT
    *       uvec4: VK_FORMAT_R32G32B32A32_UINT
    *       double: VK_FORMAT_R64_SFLOAT
    */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

        /* Description of pos attribute */
        attributeDescriptions[0].binding    = 0;
        attributeDescriptions[0].location   = 0;  /* References to location inside Vertex Shader - 0 for position. */
        attributeDescriptions[0].format     = VK_FORMAT_R32G32_SFLOAT;  /* Format of vertex position data */
        attributeDescriptions[0].offset     = offsetof(Vertex, pos);
        
        /* Description of color attribute */
        attributeDescriptions[1].binding    = 0;
        attributeDescriptions[1].location   = 1;
        attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset     = offsetof(Vertex, color);

        return attributeDescriptions;
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

    /* Handling Window Resize Explicity */
    bool framebufferResized = false;

    void run();

private:
    const size_t MAX_FRAMES_IN_FLIGHT = 2;
    
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

    /* Pipeline Layouts */
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    /* Main Graphics Pipeline */
    VkPipeline graphicsPipeline;

    /* Framebuffers */
    std::vector<VkFramebuffer> swapChainFramebuffers;

    /* Command Pool- to store commands */
    VkCommandPool commandPool;

    /* Command Buffers- to record commands */
    std::vector<VkCommandBuffer> commandBuffers;

    /* Semaphore- signals that an image has been acquired and is ready for rendering. */
    std::vector<VkSemaphore> imageAvailableSemaphores;
    
    /* Semaphore- signals that rendering has finished and presentation can happen. */
    std::vector<VkSemaphore> renderFinishedSemaphores;

    /* Current used frame */
    size_t currentFrame = 0;

    /* CPU-GPU synchronization fences */
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    /* Vertex Buffer - vertices memory area */
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;               /* Index data for corresponding vertex buffer. */
    VkDeviceMemory indexBufferMemory;

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
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
    void createCommandBuffers();
    void createSyncObjects();

    /* Initialize GLFW */
    void initGLFW();
    void initWindow();

    /* Auxiliary Functions */
    bool checkValidationLayerSupport();
    bool isDeviceSuitable( VkPhysicalDevice device );
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities );
    VkShaderModule createShaderModule( const std::vector<char>& code );
    void createBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void recreateSwapChain();
    void cleanupSwapChain();

    /* Drawing */
    void drawFrame();

    void mainLoop();
    void cleanup();
};

static std::vector<char> readFile( const std::string& filename )
{
    /* 
     * ate - Start reading from the end of file
     * binary - read the file as binary file (avoid text transformations)
     */
    std::ifstream file(filename, std::ios::ate | std::ios::binary );

    if( !file.is_open() )
        throw new std::runtime_error("Failed to open file!");

    size_t fileSize = (size_t) file.tellg();    /* Current position in the stream */
    std::vector<char> buffer(fileSize);

    file.seekg(0);  /* Move back to the first position */
    file.read( buffer.data(), fileSize);

    file.close();

    return buffer;
}

/* Vertex Data - {{position}, {color}} */
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f},    {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f},     {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f},      {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f},     {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0 
};
