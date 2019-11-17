
#include "TutorialApp.h"



TutorialApp::TutorialApp( unsigned int windowWidth, unsigned int windowHeight, std::string windowName)
    : windowWidth(windowWidth), windowHeight(windowHeight), windowName(windowName)
{
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
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
    this->pickPhysicalDevice();
    this->createLogicalDevice();


    this->initGLFW();
    this->initWindow();
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

    this->devices.resize(deviceCount);
    vkEnumeratePhysicalDevices(this->instance, &deviceCount, this->devices.data());

    for (const auto& element : this->devices)
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

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 0;

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

        i++;
    }

    return indices;
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

    return indices.isComplete();
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
    vkDestroyDevice(this->device, nullptr);
    vkDestroyInstance(this->instance, nullptr);

    glfwDestroyWindow(this->window);
    glfwTerminate();
}
