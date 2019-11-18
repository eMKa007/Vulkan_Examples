
#include "TutorialApp.h"



TutorialApp::TutorialApp( unsigned int windowWidth, unsigned int windowHeight, std::string windowName)
    : windowWidth(windowWidth), windowHeight(windowHeight), windowName(windowName)
{
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    this->initGLFW();
    this->initWindow();

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
    this->createInstance();
    this->createSurface();
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapChain();
    this->createImageViews();
}

void TutorialApp::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
            throw std::runtime_error("validation layers requested, but not available!");

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "First Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "eMKEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;

    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    if( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
        createInfo.ppEnabledLayerNames = this->validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &this->instance);

    if (result != VK_SUCCESS) 
        throw std::runtime_error("failed to create instance!");
}

void TutorialApp::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

    if( deviceCount == 0 )
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

    for (const auto& element : devices)
    {
        if( isDeviceSuitable(element) )
        {    this->physicalDevice = element;
            break;
        }
    }

    if( this->physicalDevice == VK_NULL_HANDLE )
        throw std::runtime_error("Failed to find a suitable GPU!");
}

void TutorialApp::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;

    for( uint32_t queueFamily : uniqueQueueFamilies )
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(this->deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = this->deviceExtensions.data();

    if( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(this->validationLayers.size());
        createInfo.ppEnabledLayerNames = this->validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if( vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS )
    {
        throw new std::runtime_error("Failed to create logical device! ");
    }

    vkGetDeviceQueue(this->device, indices.graphicsFamily.value(), 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, indices.presentFamily.value(), 0, &this->presentQueue);
}

bool TutorialApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data());

    /* Set of strings that represents unconfirmed required extensions */
    std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());

    for( const auto& extension : availableExtensions)
    {
        requiredExtension.erase(extension.extensionName);
    }

    return requiredExtension.empty();
}

void TutorialApp::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(this->physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
    VkExtent2D extent = chooseSwapExtent( swapChainSupport.capabilities );

    /* 
     * Decide how many images do we need in the swap chain 
     * - in that case  minimum number plus 1.
     */
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;    /* Specified number of layers that image is consist of. */
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /*
     * We'll be drawing on the images in the swap chain 
     * from the graphics queue and then submitting them on the presentation queue. 
     */
    QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice);
    uint32_t queueFamilyIndices[] { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if( indices.graphicsFamily != indices.presentFamily)
    {
        /* 
         * Images can be used across multiple queue families without explicit ownership transfers.
         */
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; 
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        /* 
         * An image is owned by one queue family at a time and ownership must be 
         * explicitly transfered before using it in another queue family. 
         */
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
        createInfo.queueFamilyIndexCount = 0;       /* Optional */
        createInfo.pQueueFamilyIndices = nullptr;   /* Optional */
    }

    /*
     * To specify that you do not want any transformation, simply specify the current transformation.
     */
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    /*
     * Specifies if the alpha channel should be used for blending with other windows in the window system. Now ignore.
     */
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if( vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapChain) != VK_SUCCESS )
    {
        throw new std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, this->swapChainImages.data());

    this->swapChainImageFormat = surfaceFormat.format;
    this->swapChainExtent = extent;
}

void TutorialApp::createImageViews()
{
    this->swapChainImageViews.resize(swapChainImages.size());

    /*
     * Loop through all images to create image view for every of them.
     */
    for ( unsigned int i = 0; i < swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = this->swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if( vkCreateImageView( this->device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw new std::runtime_error("Failed to create Image View!");
    }
}

void TutorialApp::createSurface()
{
    if( glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
    {
        throw new std::runtime_error("Failed to create window surface!");
    }

}

QueueFamilyIndices TutorialApp::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    

    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->surface, &presentSupport);

        if (presentSupport)
            indices.presentFamily = i;

        i++;
    }

    return indices;
}

SwapChainSupportDetails TutorialApp::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, this->surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);

    if( formatCount != 0 )
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, nullptr);

    if( presentModeCount != 0 )
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->surface, &presentModeCount, details.presentModes.data()); 
    }

    return details;
}

VkSurfaceFormatKHR TutorialApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for( const auto& availableFomrat : availableFormats )
    {
        if( availableFomrat.format == VK_FORMAT_B8G8R8A8_UNORM && 
            availableFomrat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return availableFomrat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR TutorialApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for( const auto& availablePresentMode : availablePresentModes )
    {
        if( availablePresentMode == VK_PRESENT_MODE_FIFO_KHR )
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TutorialApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if( capabilities.currentExtent.width != UINT32_MAX )
        return capabilities.currentExtent;
    else
    {
        VkExtent2D actualExtent = {this->windowWidth, this->windowHeight};

        /* 
         * Clamp values of 'this->windowWidth' and 'this->windowHeight' between 
         * allowed min and max extents supported by the implementation. 
         */
        actualExtent.width = std::max( capabilities.minImageExtent.width, 
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max( capabilities.minImageExtent.height, 
            std::min( capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
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
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
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

bool TutorialApp::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for( auto layerName : validationLayers) 
    {
        bool layerFound = false;

        for( const auto& layerProperties : availableLayers) 
        {
            if( strcmp(layerName, layerProperties.layerName) == 0) 
            {
                layerFound = true;
                break;
            }
        }

        if( !layerFound )
            return false;
    }

    return true;
}

bool TutorialApp::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if( extensionSupported )
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionSupported && swapChainAdequate;
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
    for (auto imageView : this->swapChainImageViews)
        vkDestroyImageView(this->device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyInstance(this->instance, nullptr);

    glfwDestroyWindow(this->window);
    glfwTerminate();
}
