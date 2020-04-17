
#include "Simulation.h"

/* Image Loader */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/* Model Loader */
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

/*
* Callbacks Functionality.
*/
static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Simulation*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

// ------------------------------------

Simulation::Simulation( unsigned int windowWidth, unsigned int windowHeight, std::string windowName)
    : windowWidth(windowWidth), windowHeight(windowHeight), windowName(windowName)
{
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    this->initGLFW();
    this->initWindow();

    this->initVulkan();
}


Simulation::~Simulation() { }

void Simulation::run()
{
    this->mainLoop();
    this->cleanup();
}

void Simulation::initVulkan()
{
    this->createInstance();
    this->createSurface();
    this->pickPhysicalDevice();
    this->createLogicalDevice();
    this->createSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createDescriptorSetLayout();
    this->createGraphicsPipeline();
    this->createDepthResources();
    this->createFramebuffers();
    this->createCommandPool();
    this->createTextureImage();
    this->createTextureImageView();
    this->createTextureSamper();
    this->loadModel();
    this->createVertexBuffer();
    this->createIndexBuffer();
    this->createUniformBuffers();
    this->createDescriptorPool();
    this->createDescriptorSets();
    this->createCommandBuffers();
    this->createSyncObjects();
}

void Simulation::createInstance()
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

void Simulation::pickPhysicalDevice()
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

void Simulation::createLogicalDevice()
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
    deviceFeatures.samplerAnisotropy    = VK_TRUE;

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

bool Simulation::checkDeviceExtensionSupport(VkPhysicalDevice device)
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

void Simulation::createSwapChain()
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

void Simulation::createImageViews()
{
    this->swapChainImageViews.resize(swapChainImages.size());

    /*
     * Loop through all images to create image view for every of them.
     */
    for ( unsigned int i = 0; i < swapChainImages.size(); i++)
    {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Simulation::createRenderPass()
{
    /* COLOR ATTACHMENT */
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format  = this->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear data in attachment before rendering.
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store rendered contents in memory, so it can be read later.
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment   = 0;
    colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /* DEPTH ATTACHMENT */
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format  = this->findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;      /* Clear data in attachment before rendering. */
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; /* Do not store depth data for now. */
    depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment   = 1;
    depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    /* Define subpass attachments */
    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    /* Subpass dependencies */
    VkSubpassDependency dependency = {};
    dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass   = 0;                    // Refers to our subpass, which is one and only one subpass here.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    /* Render pass consist of two attachment- color and depth. */
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType        = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments     = attachments.data();
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &dependency;

    if( vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS )
    {
        throw std::runtime_error("Failed to create render pass. :( \n");
    }
}

void Simulation::createDescriptorSetLayout()
{
    /* Binding to UniformBufferObject structure. */
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding    = 0;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   /* Descriptor will be referenced in vertex shader stage. */
    uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount    = 1;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    /* Binding to texture image sampler. */
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding    = 1;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;  /* Descriptor will be referenced in fragment shader stage. */

    /* Layout info describing all of the bindings. */
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType    = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if( vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->descriptorSetLayout) != VK_SUCCESS )
        throw std::runtime_error("Failed to create Descriptor Set Layout. :( \n");
}

void Simulation::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    /* Vertex Input */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    /* Input Assembly - what kind of geometry will be drawn; enable primitive restart? */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType              = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology           = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;   // Is special 0xFFFF index is present inside vertex data?

    /* Viewports and scissors */
    VkViewport viewport = {};
    viewport.x     = 0.0f;
    viewport.y     = 0.0f;
    viewport.width    = static_cast<float>(swapChainExtent.width);
    viewport.height   = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor  = {};
    scissor.offset    = {0, 0};
    scissor.extent    = swapChainExtent;

    //Combine viewport and scissor into viewport state
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType           = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount   = 1;
    viewportState.pViewports      = &viewport;
    viewportState.scissorCount    = 1;
    viewportState.pScissors       = &scissor;

    /* Rasterizer */
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable         = VK_FALSE;   // Clamp fragments that are beyond near and far planes. - Useful to shadow maps.
    rasterizer.rasterizerDiscardEnable  = VK_FALSE;   // If true- geometry is never passes through the rasterizer, this disables any output to the framebuffer.
    rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth                = 1.f;        // Width - number of fragments
    rasterizer.cullMode                 = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    /* Add factor to depth values - useful in shadow mapping */
    rasterizer.depthBiasEnable          = VK_FALSE;
    rasterizer.depthBiasConstantFactor  = 0.f;        // Optional
    rasterizer.depthBiasClamp           = 0.f;        // Optional
    rasterizer.depthBiasSlopeFactor     = 0.f;        // Optional

    /* Multisampling */
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.f;      // Optional
    multisampling.pSampleMask           = nullptr;  //Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable      = VK_FALSE;

    /* Depth and stencil testing */
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable    = VK_TRUE;
    depthStencil.depthWriteEnable   = VK_TRUE;
    depthStencil.depthCompareOp     = VK_COMPARE_OP_LESS;   /* lower depth = closer to screen */
    depthStencil.depthBoundsTestEnable  = VK_FALSE; /* Depth bounds allow to keep only fragments within specified bounds */
    depthStencil.minDepthBounds         = 0.f;  /* Optional */
    depthStencil.maxDepthBounds         = 1.f;  /* Optional */
    depthStencil.stencilTestEnable      = VK_FALSE;
    depthStencil.front  = {};                   /* Optional due to stencil test disable. */
    depthStencil.back   = {};                   /* Optional due to stencil test disable. */


    /* Color blending */
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp         = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp         = VK_BLEND_OP_ADD;
   
    /* Alpha blending - Most common way for color blending.
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    */

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;
    colorBlending.blendConstants[0]  = 0.f;   // Optional
    colorBlending.blendConstants[1]  = 0.f;   // Optional
    colorBlending.blendConstants[2]  = 0.f;   // Optional
    colorBlending.blendConstants[3]  = 0.f;   // Optional

    /* Dynamic State */
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
   
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount   = 2;
    dynamicState.pDynamicStates      = dynamicStates;

    /* Pipeline Layout */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;                          // One layout - Uniform Buffer Object bound to 0 position.
    pipelineLayoutInfo.pSetLayouts            = &this->descriptorSetLayout; // Pointer to descriptor set layout which bound all of the descriptors.
    pipelineLayoutInfo.pushConstantRangeCount = 0;        // Optional
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;  // Optional
   
    if( vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout! :(\n");
    
    /* Combine structures to create pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pDepthStencilState     = &depthStencil;
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = nullptr;
    
    pipelineInfo.layout                 = this->pipelineLayout;
    pipelineInfo.renderPass             = this->renderPass;
    
    pipelineInfo.subpass                = 0;    // Index of the subpass where this pipeline will be used.
    
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;   // Optional
    pipelineInfo.basePipelineIndex      = -1;               // Optional

    if( vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Graphics Pipeline! :( \n");

    /* Tidy up unused objects */
    vkDestroyShaderModule(this->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(this->device, vertShaderModule, nullptr);
}

void Simulation::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImages.size());

    for(size_t i=0; i< swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageView };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass  = this->renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = swapChainExtent.width;
        framebufferInfo.height          = swapChainExtent.height;
        framebufferInfo.layers          = 1;
 
        if(vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer :( \n");
    }
}

void Simulation::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(this->physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex   = queueFamilyIndices.graphicsFamily.value();    // Commands for drawing- graphics queue
    poolInfo.flags  = 0;

    if( vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool) != VK_SUCCESS )
        throw std::runtime_error("Failed to create command pool :( \n");
}

void Simulation::createTextureImage()
{
    /* Local Variables */
    int texWidth    = 0;
    int texHeight   = 0;
    int texChannels = 0;

    /* Load an image from file. */
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;  /* 4 bytes per pixel- RGBA values */

    if(!pixels)
        throw std::runtime_error("Failed to load texture file: textures/texture.jpg :( \n");

    /* Load image via staging buffer. */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    /* Crate staging buffer */
    this->createBuffer(imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, 
        stagingBufferMemory
    );

    /* Copy data directly to staging buffer. */
    void* data;
    if(vkMapMemory(this->device, stagingBufferMemory, 0, imageSize, 0, &data) != VK_SUCCESS )
        throw std::runtime_error("Failed to map staging buffer memory. :( \n");
    memcpy(data, pixels, static_cast<uint32_t>(imageSize));
    vkUnmapMemory(this->device, stagingBufferMemory);

    /* Free stbi image data */
    stbi_image_free(pixels);

    /* Create object to hold image data. */
    this->createImage(texWidth, 
        texHeight, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        this->textureImage, 
        this->textureImageMemory
    );

    /* Copy staging buffer to created texture image.
    *   1. Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (it was created with undefined layout)
    *   2. Execute the buffer to image copy operation.
    *   3. Transition image to SHADER_READ_ONLY_OPTIMAL layout to prepare it for shader access.
    *   TODO: Combine these operations in single command buffer and execute them asynchronously.
    *       Create setupCommandBuffer to record commands into.
    *       Execute commands with flushSetupCommands() that have been recorded so far. 
    */
    this->transitionImageLayout(this->textureImage, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    
    this->copyBufferToImage(stagingBuffer, 
        this->textureImage,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    this->transitionImageLayout(this->textureImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    /* Free staging buffer resources. */
    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
}

void Simulation::createTextureImageView()
{
    /* Images are accessed through image views rather than directly. Thus we have to create one for texture image. */
    this->textureImageView = this->createImageView(this->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Simulation::createTextureSamper()
{
    /* Samplers are user to read texels. They can apply filtering and other transformations to compute final color that is retrieved from sampler. */
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    
    samplerInfo.magFilter   = VK_FILTER_LINEAR;     /* Describes how to interpolate texels that are magnified. */
    samplerInfo.minFilter   = VK_FILTER_LINEAR;     /* Describes how to interpolate texels that are minified. */
    
    /* U/V/W are axes instead of X/Y/Z. Describes how to access texels while they outside the texture dimensions. */
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  /* Repeat the texture when going beyond the image dimensions. */
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    /* Describing Anisotropic Filtering. */
    samplerInfo.anisotropyEnable    = VK_TRUE;
    samplerInfo.maxAnisotropy       = 16;       /* Lower value = lower quality but better performance. */

    /* Describing which color to use if clamp to border addressing mode is used. */
    samplerInfo.borderColor =   VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    /* Describing texture coordinate system. Normalized are [0;1] */
    samplerInfo.unnormalizedCoordinates =   VK_FALSE;

    /* Describe way to compare texel value to other values. Mainly used for percentage-closer filtering. */
    samplerInfo.compareEnable   = VK_FALSE;
    samplerInfo.compareOp       = VK_COMPARE_OP_ALWAYS;
    
    /* Specify mipmapping characteristics. */
    samplerInfo.mipmapMode  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias  = 0.f;
    samplerInfo.minLod      = 0.f;
    samplerInfo.maxLod      = 0.f;

    /* Image Sampler do not refer VkImage object anywhere. It is distinct object that provide interface to extract color from texture. */
    if(vkCreateSampler(this->device, &samplerInfo, nullptr, &this->textureSampler) != VK_SUCCESS )
        throw std::runtime_error("Failed to create texture sampler! :( \n");
}

void Simulation::loadModel()
{
    /* Vertex attributes */
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    /* Local Warning/Error variable */
    std::string warn;
    std::string err;

    /* Unique vertices container. */
    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    /* Load object from *.obj file. */
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    /* Iterate over all of the shapes to combine all of the faces into a single model. */
    for(const auto& shape : shapes)
    {
        for(const auto& index : shape.mesh.indices)
        {
            Vertex vertex = {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index+0],
                attrib.vertices[3 * index.vertex_index+1],
                attrib.vertices[3 * index.vertex_index+2]
            };

            vertex.normal = {
                attrib.normals[3 * index.vertex_index+0],
                attrib.normals[3 * index.vertex_index+1],
                attrib.normals[3 * index.vertex_index+2]
            };

            vertex.texCoord = {
                /* attrib.texcoords[2 * index.texcoord_index+0],
                1.0f - attrib.texcoords[2 * index.texcoord_index+1] */
                0.5f, 0.5f
            };
    
            vertex.color = {0.7f, 0.7f, 0.7f};

            /* Check if same vertex has been already read. */
            if( uniqueVertices.count(vertex) == 0 )
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void Simulation::createDepthResources()
{
    VkFormat depthFormat = this->findDepthFormat();
    
    /* Create vkImage object with given properties */
    this->createImage(this->swapChainExtent.width,
            this->swapChainExtent.height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory
        );
    
    /* Crate Image view bound to previously created depth image */
    this->depthImageView = this->createImageView(this->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Simulation::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    /* Temporary host buffer to copy data from CPU to GPU */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Mapping staging buffer memory and filling it with vertex data. */
    void* data;
    if( vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS )
        throw std::runtime_error("Failed to create staging buffer. :( \n");
    
    /* Copy vertices data to staging buffer */
    memcpy(data, vertices.data(), (size_t)bufferSize);

    /* Unmap previously mapped memory object as data was copied */
    vkUnmapMemory(this->device, stagingBufferMemory);

    /* Create Vertex Buffer on GPU */
    createBuffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        this->vertexBuffer,
        this->vertexBufferMemory);

    /* Copy data from staging buffer to high performance memory on GPU */
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    /* Clean up resources */
    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
}

void Simulation::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    /* With additional staging buffer copy data to GPU memory. */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    /* Create buffer for storing indices data. */
    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Map memory to hold indices data. */
    void* data;
    if(vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS )
        throw std::runtime_error("Failed to create indices staging buffer! :( \n");
    
    /* Copy data to mapped memory. */
    memcpy(data, indices.data(), (size_t)bufferSize);

    /* Unmap memory to free resources. */
    vkUnmapMemory(this->device, stagingBufferMemory);

    /* Create buffer to hold indices data on GPU. */
    createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory);

    /* Copy data to GPU indices buffer */
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    /* Free resources */
    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
}

void Simulation::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    /* Resize array of buffers to hold buffer for every image in swapchain */
    this->uniformBuffers.resize(this->swapChainImages.size());
    this->uniformBuffersMemory.resize(this->swapChainImages.size());

    /* Create Buffer for every of uniformBuffer array member */
    for( size_t i = 0; i<this->swapChainImages.size(); i++)
    {
        this->createBuffer(bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            this->uniformBuffers[i],
            this->uniformBuffersMemory[i]
        );
    }

    /* Separate function will update buffers in every frame so it is now required to map memory here. */
}

void Simulation::createDescriptorPool()
{
    /* Provide information about descriptors type of our descriptor sets and how many of them. 
    *  This structure is referenced in by the main VkDescriptorPoolCreateInfo structure. 
    */
    std::array<VkDescriptorPoolSize, 2> poolSize = {};
    poolSize[0].type    = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    /* Which descriptors types this pool is going to contain. */
    poolSize[0].descriptorCount     = static_cast<uint32_t>(this->swapChainImages.size());
    poolSize[1].type    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount     = static_cast<uint32_t>(this->swapChainImages.size());


    /* Allocate one pool which can contain up to swap images count descriptors sets. */
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType  = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount  = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes     = poolSize.data();
    poolInfo.maxSets        = static_cast<uint32_t>(this->swapChainImages.size());
    poolInfo.flags          = 0; /* Default Value */

    if(vkCreateDescriptorPool(this->device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS )
        throw std::runtime_error("Failed to create descriptor pool. :( \n");
}

void Simulation::createDescriptorSets()
{
    /* We will create one descriptor set for each swap chain image- all with the same layout. */
    std::vector<VkDescriptorSetLayout> layouts(this->swapChainImages.size(), this->descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool        = this->descriptorPool;
    allocInfo.descriptorSetCount    = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts           = layouts.data();

    /* Allocate every descriptor set */
    this->descriptorSets.resize(this->swapChainImages.size());
    if(vkAllocateDescriptorSets(this->device, &allocInfo, this->descriptorSets.data()) != VK_SUCCESS )
        throw std::runtime_error("Failed to allocate descriptor sets. :( \n");

    /* Configure each descriptor. */
    for( size_t i = 0; i<this->swapChainImages.size(); i++)
    {
        /* Specify UBO information */
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer   = this->uniformBuffers[i];
        bufferInfo.offset   = 0;
        bufferInfo.range    = sizeof(UniformBufferObject);  /* If Updating whole buffer - we can use VK_WHOLE_SIZE */
    
        /* Specify Sampler information */
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView     = this->textureImageView;
        imageInfo.sampler       = this->textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrite = {};
        /* Descriptor set for buffer object. */
        descriptorWrite[0].sType   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[0].dstSet  = descriptorSets[i];
        descriptorWrite[0].dstBinding      = 0;    /* Destination binding in shader */
        descriptorWrite[0].dstArrayElement = 0;    /* Descriptors set can be an arrays, so we have to provide element to update. */
        
        descriptorWrite[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite[0].descriptorCount = 1;

        descriptorWrite[0].pBufferInfo     = &bufferInfo;      /* Array with the descriptors count structs. */
        descriptorWrite[0].pImageInfo      = nullptr;          /* Optional */
        descriptorWrite[0].pTexelBufferView = nullptr;         /* Optional */

        /* Descriptor set for texture sampler image info. */
        descriptorWrite[1].sType   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[1].dstSet  = descriptorSets[i];
        descriptorWrite[1].dstBinding      = 1;    /* Destination binding in shader */
        descriptorWrite[1].dstArrayElement = 0;    /* Descriptors set can be an arrays, so we have to provide element to update. */
        
        descriptorWrite[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[1].descriptorCount = 1;

        descriptorWrite[1].pBufferInfo     = nullptr;          /* Optional */
        descriptorWrite[1].pImageInfo      = &imageInfo;       /* Array with the descriptors count structs - image samplers */
        descriptorWrite[1].pTexelBufferView = nullptr;         /* Optional */

        vkUpdateDescriptorSets(this->device, 
            static_cast<uint32_t>(descriptorWrite.size()),
            descriptorWrite.data(), 
            0, 
            nullptr
        );
    }
}

void Simulation::createCommandBuffers()
{
    // Allocate and record commands for each swap chain image.
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool           = this->commandPool;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  //Can be submitted to a queue for execution, but cannot be called from other command buffers.
    allocInfo.commandBufferCount    = static_cast<uint32_t>(this->commandBuffers.size());
    
    if( vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers. :( \n");

    // Starting command buffer recording
    for( size_t i = 0; i<this->commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                    // Optional
        beginInfo.pInheritanceInfo  = nullptr;  // Optional

        /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it. */
        if( vkBeginCommandBuffer( this->commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer. :( \n");

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass   = this->renderPass;
        renderPassInfo.framebuffer  = this->swapChainFramebuffers[i];
        
        /* Define size of render area */
        renderPassInfo.renderArea.offset    = {0,0};
        renderPassInfo.renderArea.extent    = this->swapChainExtent;
        
        /* Clear values - specify clear operation.
         * Order of clear values should be same as attachments.
         */
        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.f, 0.f, 0.f, 1.f};
        clearValues[1].depthStencil = {1.f, 0}; 
        renderPassInfo.clearValueCount  = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues     = clearValues.data();
        
        /* RECORDING */
        vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        vkCmdBindPipeline(this->commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

        /* Binding vertex buffer */
        VkBuffer vertexBuffers[] = {this->vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(this->commandBuffers[i], 0, 1, vertexBuffers, offsets);

        /* Binding index buffer */
        vkCmdBindIndexBuffer(this->commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        /* Bind descriptor sets- to update uniform data. */
        vkCmdBindDescriptorSets(this->commandBuffers[i], 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            this->pipelineLayout, 
            0, 
            1, 
            &descriptorSets[i], 
            0, 
            nullptr);

        /* Draw command by using indexes of vertices. */
        vkCmdDrawIndexed(this->commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        /* END RECORDING */
        vkCmdEndRenderPass(this->commandBuffers[i]);

        if( vkEndCommandBuffer( this->commandBuffers[i]) != VK_SUCCESS )
            throw std::runtime_error("Failed to record command buffer! :( \n");
    }
}

void Simulation::createSyncObjects()
{
    this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    this->imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create fence as signaled. Initial frame should not now wait for previous frame- means for ever.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(size_t i = 0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        if( vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS || 
            vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS )
        {
            throw std::runtime_error("Failed to create semaphores :( \n");
        }
    }
}

void Simulation::createSurface()
{
    if( glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
    {
        throw new std::runtime_error("Failed to create window surface!");
    }

}

QueueFamilyIndices Simulation::findQueueFamilies(VkPhysicalDevice device)
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

uint32_t Simulation::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);
    
    /* 000100010b 
    *          ^  - check bit field in every step.
    */
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if((typeFilter & (1 << i)) &&
          ((memProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type. :( \n");
}

VkFormat Simulation::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for( VkFormat format : candidates )
    {
        /* VkFormatProperties containing fields..
        *       linearTilingFeatures:   Use cases that are supported with linear tiling
        *       optimalTilingFeatures:  Use cases that are supported with optimal tiling
        *       bufferFeatures:         Use cases that are supported for buffers
        */
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &properties);

        if( tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features )
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features )
            return format;
    }

    throw std::runtime_error("Failed to find supported format! :( \n");
}

VkFormat Simulation::findDepthFormat()
{
    return this->findSupportedFormat( 
                    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                );
}

SwapChainSupportDetails Simulation::querySwapChainSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR Simulation::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR Simulation::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for( const auto& availablePresentMode : availablePresentModes )
    {
        if( availablePresentMode == VK_PRESENT_MODE_FIFO_KHR )
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Simulation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if( capabilities.currentExtent.width != UINT32_MAX )
        return capabilities.currentExtent;
    else
    {
        int width;
        int height;
        glfwGetFramebufferSize(this->window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width), 
            static_cast<uint32_t>(height)
        };

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

VkShaderModule Simulation::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if( vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS )
        throw new std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

void Simulation::createBuffer(VkDeviceSize deviceSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
    /* Specify memory desired type and size */
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size     = deviceSize;                           /* Size of buffer in bytes */
    bufferInfo.usage    = usage;
    bufferInfo.flags    = 0;                                    /* No additional parameters */
    bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(this->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS )
        throw std::runtime_error("Failed to create vertex buffer. :( \n");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(this->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = this->findMemoryType(memRequirements.memoryTypeBits, properties ); // Mapped memory always matches the contents of the allocated memory.
 
    if(vkAllocateMemory(this->device, &allocInfo, nullptr, &bufferMemory))
        throw std::runtime_error("Failed to allocate vertex buffer memory! :( \n");

    /* Bind created memory to vertex buffer object */
    vkBindBufferMemory(this->device, buffer, bufferMemory, 0);
}

void Simulation::createImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imgTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags imgMemoryProperties, VkImage & image, VkDeviceMemory & imgMemory)
{
    /* Create object to hold image data. */
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = imageFormat;
    imageInfo.tiling        = imgTiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usageFlags;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(this->device, &imageInfo, nullptr, &image) != VK_SUCCESS )
        throw std::runtime_error("Failed to create image. :( \n");

    /* Allocate memory for image memory. As like for the buffer. 
    *  Query for memory requirements for previously created VkImage object.
    *  Set up allocation info structure.
    *  Allocate memory for VkDeviceMemory object.
    *  Bind Allocated memory with previously created VkImage object.
    */
    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(this->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = this->findMemoryType(memRequirements.memoryTypeBits, imgMemoryProperties);

    if(vkAllocateMemory(this->device, &allocInfo, nullptr, &imgMemory) != VK_SUCCESS )
        throw std::runtime_error("Failed to allocate image memory. :( \n");
    
    /* Bind image object with image memory object. */
    vkBindImageMemory(this->device, image, imgMemory, 0);
}

VkImageView Simulation::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image  = image;
    viewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format     = format;
    viewInfo.subresourceRange.aspectMask        = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel      = 0;
    viewInfo.subresourceRange.levelCount        = 1;
    viewInfo.subresourceRange.baseArrayLayer    = 0;
    viewInfo.subresourceRange.layerCount        = 1;

    VkImageView imageView;
    if(vkCreateImageView(this->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS )
        throw std::runtime_error("Failed to create image view. :( \n");

    return imageView;
}

void Simulation::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    /* Begin recording command buffer */
    VkCommandBuffer commandBuffer = this->beganSingleTimeCommands();

    /* Provide information about regions to copy from and to. Call CopyBuffer command. */
    VkBufferCopy copyRegion = {};
    copyRegion.size         = size;
    copyRegion.dstOffset    = 0;
    copyRegion.srcOffset    = 0;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /* End commands recording. */
    this->endSingleTimeCommands(commandBuffer);
}

void Simulation::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = this->beganSingleTimeCommands();
    
    /* Specify which part of the buffer is going to be copied to which part of the image */
    VkBufferImageCopy region = {};
    region.bufferOffset         = 0;
    region.bufferRowLength      = 0;    /* Pixels are tightly packed */
    region.bufferImageHeight    = 0;

    region.imageSubresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel        = 0;
    region.imageSubresource.baseArrayLayer  = 0;
    region.imageSubresource.layerCount      = 1;

    region.imageOffset  = {0, 0, 0};            /* x, y, z - values */
    region.imageExtent  = {width, height, 1};   /* width, height, depth - values*/

    vkCmdCopyBufferToImage(commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    this->endSingleTimeCommands(commandBuffer);
}

void Simulation::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    /* Update variables inside uniform buffer */
    UniformBufferObject ubo = {};
    ubo.model   = glm::rotate(glm::mat4(1.f), time*glm::radians(45.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.model   *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    ubo.view    = glm::lookAt(glm::vec3(5.f, 5.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
    ubo.proj    = glm::perspective(glm::radians(45.f),
                        this->swapChainExtent.width / (float) this->swapChainExtent.height,
                        0.1f,
                        20.f);
    /* GLM was originally designed for OpenGL, it is important to revert scaling factor of Y axis. */
    ubo.proj[1][1] *= -1;

    /* With providing this information, we can now map memory of the uniform buffer. */
    void* data;
    vkMapMemory(this->device, 
        this->uniformBuffersMemory[currentImage], 
        0,
        sizeof(ubo),
        0,
        &data );
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(this->device, this->uniformBuffersMemory[currentImage]);
}

void Simulation::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = this->beganSingleTimeCommands();

    /* Use VkImageMemoryBarrier to ensure that write to the buffer completes before reading from it. 
    *  It is equivalent to VkBufferMemoryBarrier for buffers.
    */
    VkImageMemoryBarrier barrier = {};
    barrier.sType   = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout   = oldLayout;    /* It is possible to use VK_IMAGE_LAYOUT_UNDEFINED */
    barrier.newLayout   = newLayout;

    /* If we are using Image Barrier to transfer queue family ownership then these two fields have to be specified.
    *  If we are not using those- we have to set it to FAMILU_IGNORED (it is not default value).
    */
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    /* Specify an image to be affected and specific part of this image (if mipmapping included). */
    barrier.image   = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    /* Specify type of operations that involve the resource must happen before and after the barrier. 
    *  We need to handle two transitions:
    *   Undefined -> transfer destination           Transfer writes do not have to wait for anything.
    *   Transfer destination -> shader reading      Shader reads should wait on transfer writes, especially in the fragment shader.
    */
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask   = 0;
        barrier.dstAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage         = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;     /* Transfer write must occur in the pipeline transfer stage. */
        barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;

        sourceStage         = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;    /* Image will be read inside fragment shader */
    }
    else
    {
        throw std::runtime_error("Unsupported layout transition! :( \n");
    }

    vkCmdPipelineBarrier( commandBuffer,
        sourceStage,            /* Specifies in which pipeline stage the operations should occur that should happen before the barrier.  */ 
        destinationStage,       /* Specifies the pipeline stage in which operations will wait on the barrier. */
        0,
        0, nullptr,     /* Memory Barriers          */
        0, nullptr,     /* Buffer Memory Barriers   */
        1, &barrier     /* Image Memory Barriers    */
        );

    this->endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Simulation::beganSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool           = this->commandPool;
    allocInfo.commandBufferCount    = 1;

    /* Allocate command buffer according to allocation structure */
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo  = nullptr;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Simulation::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType    = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;

    vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(this->graphicsQueue);
    
    vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);
}

void Simulation::recreateSwapChain()
{
    /* Handling Window minimization - size of framebuffer is 0 */
    int width;
    int height;
    
    glfwGetFramebufferSize(this->window, &width, &height);

    /* Wait till window is returned to foreground. */
    while( width == 0 || height == 0 )
    {
        glfwGetFramebufferSize(this->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(this->device);

    this->cleanupSwapChain();

    this->createSwapChain();
    this->createImageViews();
    this->createRenderPass();
    this->createGraphicsPipeline();
    this->createDepthResources();
    this->createFramebuffers();
    this->createUniformBuffers();
    this->createDescriptorPool();
    this->createDescriptorSets();
    this->createCommandBuffers();
}

void Simulation::cleanupSwapChain()
{
    /* Destroy depth resources */
    vkDestroyImageView(this->device, this->depthImageView, nullptr);
    vkDestroyImage(this->device, this->depthImage, nullptr);
    vkFreeMemory(this->device, this->depthImageMemory, nullptr);

    for( size_t i = 0; i < swapChainFramebuffers.size(); i++ )
        vkDestroyFramebuffer(this->device, this->swapChainFramebuffers[i], nullptr);

    vkFreeCommandBuffers(this->device, this->commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(this->device, this->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->pipelineLayout, nullptr);
    vkDestroyRenderPass(this->device, this->renderPass, nullptr);

    for( size_t i = 0; i < swapChainImageViews.size(); i++ )
        vkDestroyImageView(this->device, this->swapChainImageViews[i], nullptr);

    vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
    
    for (size_t i = 0; i < swapChainImages.size(); i++) 
    {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Simulation::initGLFW()
{
    /* Init GLFW */
    if( glfwInit() == GLFW_FALSE )
    {
        std::cout << "ERROR::GLFW_INIT_FAILED  \n";
        glfwTerminate();
    }
}

void Simulation::initWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
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
    
    /*
    * Set pointer to GLFWwindow inside callbacks functions.
    */
    glfwSetWindowUserPointer(this->window, this);
    glfwSetFramebufferSizeCallback(this->window, framebufferResizeCallback);
}

bool Simulation::checkValidationLayerSupport()
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

bool Simulation::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if( extensionSupported )
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void Simulation::drawFrame()
{
    // Wait for previous frame to be finished. 
    vkWaitForFences(this->device, 1, &this->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    /*
    *  Perform operations:
    *   Acquire an image from the swap chain.
    *   Execute the command buffer with that image as attachment in the framebuffer.
    *   Return the image to the swap chain for presentation.
    */
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        this->device,                   // Logical Device
        this->swapChain,                // Swap chain from which acquire image.
        UINT64_MAX,                     // Timeout in nanoseconds
        this->imageAvailableSemaphores[this->currentFrame],  // Semaphore to be signalized after using the image
        VK_NULL_HANDLE,                 // Fence - null
        &imageIndex                     // Image index - refers to the VkImage in swapChainImages array.
    );

    if( result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        this->recreateSwapChain();
        return;
    }
    else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
    {
        throw std::runtime_error("Failed to acquire swap chain image. :(\n");
    }

    // Check if previous frame is using this image (there is its fence to wait on).
    if( this->imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(this->device, 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);

    // Mark the image as now being in use by current frame
    this->imagesInFlight[imageIndex] = this->inFlightFences[currentFrame];
    
    /* With information about current image we can update its uniform buffer. */
    this->updateUniformBuffer(imageIndex);

    /* Submit the command buffer */
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;

    /* Which command buffer to actually submit for execution */
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &this->commandBuffers[imageIndex];

    /* Which semaphore to signal once the command buffer finished execution */
    VkSemaphore signalSemaphores[]  = { this->renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    //Reset fence to be 'unsignaled'.
    vkResetFences(this->device, 1, &this->inFlightFences[currentFrame]);

    if( vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VK_SUCCESS )
        throw std::runtime_error("Failed to submit draw command buffer! :(\n");

    /* Presentation - submit the result back to the swap chain */
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = signalSemaphores; // Which semaphores to wait on before presentation.

    VkSwapchainKHR  swapChains[]    = {swapChain};
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = swapChains;   // Which swapchains to present image. 
    presentInfo.pImageIndices       = &imageIndex;

    // Can specify array of VkResult values to check for every individual swap chain if presentation was successful.
    presentInfo.pResults = nullptr; //Optional.

    VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);

    if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized )
    {
        framebufferResized = false;
        this->recreateSwapChain();
    }
    else if( result != VK_SUCCESS )
    {
        throw std::runtime_error("Failed to present swap chain image! :(\n");
    }

    vkQueueWaitIdle(presentQueue);

    // Proceed to next frame counter
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Simulation::mainLoop()
{
    while(!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void Simulation::cleanup()
{
    this->cleanupSwapChain();

    /* Destroy and free memory for texture image. */
    vkDestroySampler(this->device, this->textureSampler, nullptr);
    vkDestroyImageView(this->device, this->textureImageView, nullptr);
    vkDestroyImage(this->device, this->textureImage, nullptr);
    vkFreeMemory(this->device, this->textureImageMemory, nullptr);

    /* Destroy descriptor set layout which is bounding all of the descriptors. */
    vkDestroyDescriptorSetLayout(this->device, this->descriptorSetLayout, nullptr);

    /* Destroy Index Buffer and allocated to it memory */
    vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
    vkFreeMemory(this->device, this->indexBufferMemory, nullptr);

    /* Destroy Vertex Buffer and allocated to it memory */
    vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
    vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);

    for(size_t i = 0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(this->device, this->commandPool, nullptr);

    vkDestroyDevice(this->device, nullptr);

    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkDestroyInstance(this->instance, nullptr);

    glfwDestroyWindow(this->window);
    glfwTerminate();
}
