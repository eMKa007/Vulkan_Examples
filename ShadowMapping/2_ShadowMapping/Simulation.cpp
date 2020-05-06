
#include "Simulation.h"

/* Model Loader */
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

/*
* Callbacks Functionality.
*/
static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Simulation*>(glfwGetWindowUserPointer(window));
    app->_framebufferResized = true;
}

// ------------------------------------

Simulation::Simulation( unsigned int windowWidth, unsigned int windowHeight, std::string windowName)
    : _windowWidth(windowWidth), _windowHeight(windowHeight), _windowName(windowName),
    _camera(glm::vec3(5.f, 8.f, 5.f), glm::vec3(-45.f, -135.f, 0.f), glm::vec3(0.f, 1.f, 0.f))
{
    validation_layers.push_back("VK_LAYER_KHRONOS_validation");
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    init_GLFW();
    init_GLFW_window();

    init_vulkan();
}

Simulation::~Simulation() { }

void Simulation::run()
{
    main_loop();
    cleanup();
}

void Simulation::init_vulkan()
{
    create_instance();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();
    create_scene_render_pass();
    create_offscreen_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_depth_resources();
    create_scene_framebuffer();
    create_offscreen_framebuffer();
    create_command_pool();
    create_depth_texture_sampler();
    load_model();
    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
    create_sync_objects();
}

void Simulation::create_instance()
{
    if (enableValidationLayers && !check_validatio_layer_support())
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
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);

    if (result != VK_SUCCESS) 
        throw std::runtime_error("failed to create instance!");
}

void Simulation::pick_physical_device()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if( deviceCount == 0 )
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    for (const auto& element : devices)
    {
        if( is_device_suitable(element) )
        {    _physical_device = element;
            break;
        }
    }

    if( _physical_device == VK_NULL_HANDLE )
        throw std::runtime_error("Failed to find a suitable GPU!");
}

void Simulation::create_logical_device()
{
    QueueFamilyIndices indices = find_queue_families(_physical_device);

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

    createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    createInfo.ppEnabledExtensionNames = device_extensions.data();

    if( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if( vkCreateDevice(_physical_device, &createInfo, nullptr, &_device) != VK_SUCCESS )
    {
        throw new std::runtime_error("Failed to create logical device! ");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_queues.graphics_queue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_queues.present_queue);
}

bool Simulation::check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data());

    /* Set of strings that represents unconfirmed required extensions */
    std::set<std::string> requiredExtension(device_extensions.begin(), device_extensions.end());

    for( const auto& extension : availableExtensions)
    {
        requiredExtension.erase(extension.extensionName);
    }

    return requiredExtension.empty();
}

void Simulation::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(_physical_device);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format( swapChainSupport.formats );
    VkPresentModeKHR presentMode = choose_swap_present_mode( swapChainSupport.presentModes );
    VkExtent2D extent = choose_swap_extent( swapChainSupport.capabilities );

    /* 
     * Decide how many images do we need in the swap chain 
     * - in that case  minimum number plus 1.
     */
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;

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
    QueueFamilyIndices indices = find_queue_families(_physical_device);
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

    if( vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swap_chain.swap_chain) != VK_SUCCESS )
    {
        throw new std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(_device, _swap_chain.swap_chain, &imageCount, nullptr);
    _swap_chain.swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swap_chain.swap_chain, &imageCount, _swap_chain.swap_chain_images.data());

    _swap_chain.swap_chain_image_format = surfaceFormat.format;
    _swap_chain.swap_chain_extent = extent;
}

void Simulation::create_image_views()
{
    _swap_chain.swap_chain_image_views.resize(_swap_chain.swap_chain_images.size());

    /*
     * Loop through all images to create image view for every of them.
     */
    for ( unsigned int i = 0; i < _swap_chain.swap_chain_images.size(); i++)
        _swap_chain.swap_chain_image_views[i] = create_image_view(_swap_chain.swap_chain_images[i], _swap_chain.swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Simulation::create_scene_render_pass()
{
    /* COLOR ATTACHMENT */
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format  = _swap_chain.swap_chain_image_format;
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
    depthAttachment.format  = DEPTH_FORMAT;//findDepthFormat();
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

    if( vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_scene_pass.render_pass) != VK_SUCCESS )
    {
        throw std::runtime_error("Failed to create render pass. :( \n");
    }
}

void Simulation::create_descriptor_set_layout()
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

    if( vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptor_set_layout) != VK_SUCCESS )
        throw std::runtime_error("Failed to create Descriptor Set Layout. :( \n");
}

void Simulation::create_graphics_pipeline()
{
    VkShaderModule vertShaderModule = creates_shader_module(read_file(VERT_SHADER));
    VkShaderModule fragShaderModule = creates_shader_module(read_file(FRAG_SHADER));

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
    viewport.width    = static_cast<float>(_swap_chain.swap_chain_extent.width);
    viewport.height   = static_cast<float>(_swap_chain.swap_chain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor  = {};
    scissor.offset    = {0, 0};
    scissor.extent    = _swap_chain.swap_chain_extent;

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
    std::array<VkDynamicState, 4> dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };
   
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount   = static_cast<uint32_t>(dynamicStates->size());
    dynamicState.pDynamicStates      = dynamicStates->data();

    /* Pipeline Layout */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;                          // One layout - Uniform Buffer Object bound to 0 position.
    pipelineLayoutInfo.pSetLayouts            = &_descriptor_set_layout; // Pointer to descriptor set layout which bound all of the descriptors.
    pipelineLayoutInfo.pushConstantRangeCount = 0;        // Optional
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;  // Optional
   
    if( vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipeline_layouts.scene) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout! :(\n");

    /* Create pipeline layout for offscreen render pass. */
    if( vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipeline_layouts.offscreen) != VK_SUCCESS)
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
    
    pipelineInfo.layout                 = _pipeline_layouts.scene;
    pipelineInfo.renderPass             = _scene_pass.render_pass;
    
    pipelineInfo.subpass                = 0;    // Index of the subpass where this pipeline will be used.
    
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;   // Optional
    pipelineInfo.basePipelineIndex      = -1;               // Optional

    if( vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipelines.scene) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Graphics Pipeline! :( \n");

    /* Offscreen Pipeline - vertex shader only */
    vertShaderStageInfo.module = creates_shader_module(read_file(OFFSCREEN_VERT_SHADER));
    shaderStages[0] = vertShaderStageInfo;

    pipelineInfo.stageCount = 1;
    // No blend attachment states (no color attachments used)
    colorBlending.attachmentCount = 0;
    // Cull front faces
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    // Enable depth bias
    rasterizer.depthBiasEnable = VK_TRUE;

    // Add depth bias to dynamic state, so we can change it at runtime
    pipelineInfo.pDynamicState = &dynamicState;
    
    pipelineInfo.layout     = _pipeline_layouts.offscreen;
    pipelineInfo.renderPass = _offscreen_pass.render_pass;

    if( vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipelines.offscreen) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Graphics Pipeline- offscreen render pass! :( \n");

    /* Tidy up unused objects */
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, vertShaderStageInfo.module, nullptr);
}

void Simulation::create_scene_framebuffer()
{
    _scene_pass.framebuffers.resize(_swap_chain.swap_chain_images.size());

    for(size_t i=0; i< _swap_chain.swap_chain_image_views.size(); i++)
    {
        std::array<VkImageView, 2> attachments = { _swap_chain.swap_chain_image_views[i], _scene_pass.depth.image_view };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass  = _scene_pass.render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments    = attachments.data();
        framebufferInfo.width           = _swap_chain.swap_chain_extent.width;
        framebufferInfo.height          = _swap_chain.swap_chain_extent.height;
        framebufferInfo.layers          = 1;
 
        if(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_scene_pass.framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer :( \n");
    }
}

void Simulation::create_offscreen_framebuffer()
{
    // Create frame buffer and connect it with created render pass. 
    VkFramebufferCreateInfo framebufferCreateInfo {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass        = _offscreen_pass.render_pass;
    framebufferCreateInfo.attachmentCount   = 1;
    framebufferCreateInfo.pAttachments      = &_offscreen_pass.depth.image_view;
    framebufferCreateInfo.width             = _windowWidth;
    framebufferCreateInfo.height            = _windowHeight;
    framebufferCreateInfo.layers            = 1;

    if(vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &_offscreen_pass.frameBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create offscreen framebuffer :( \n");
}

/* Crate render pass for offscreen frame buffer */
void Simulation::create_offscreen_render_pass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format  = DEPTH_FORMAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;  /* Clear values at the beginning of the render pass. */
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; /* Store values of depth attachment for further use. */
    depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED; /* Initial layout is not important. */
    depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 0;
    depthReference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;                   /* No color attachment needed */
    subpass.pDepthStencilAttachment = &depthReference;

    /* Use subpass dependencies for layout transitions. */
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount    = 1;
    renderPassCreateInfo.pAttachments       = &depthAttachment;
    renderPassCreateInfo.dependencyCount    = static_cast<uint32_t>(dependencies.size());
    renderPassCreateInfo.pDependencies      = dependencies.data();
    renderPassCreateInfo.subpassCount       = 1;
    renderPassCreateInfo.pSubpasses         = &subpass;

    if( vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &_offscreen_pass.render_pass) != VK_SUCCESS )
        throw std::runtime_error("Failed to create render pass. :( \n");
}

void Simulation::create_command_pool()
{
    QueueFamilyIndices queueFamilyIndices = find_queue_families(_physical_device);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex   = queueFamilyIndices.graphicsFamily.value();    // Commands for drawing- graphics queue
    poolInfo.flags  = 0;

    if( vkCreateCommandPool(_device, &poolInfo, nullptr, &_command_pool) != VK_SUCCESS )
        throw std::runtime_error("Failed to create command pool :( \n");
}

void Simulation::create_depth_texture_sampler()
{
    /* Offscreen depth map sampler */
    /* Samplers are user to read texels. They can apply filtering and other transformations to compute final color that is retrieved from sampler. */
    VkSamplerCreateInfo depthSamplerInfo = {};
    depthSamplerInfo.sType   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    depthSamplerInfo.magFilter   = VK_FILTER_LINEAR;    /* Describes how to interpolate texels that are magnified. */
    depthSamplerInfo.minFilter   = VK_FILTER_LINEAR;    /* Describes how to interpolate texels that are minified. */

    /* U/V/W are axes instead of X/Y/Z. Describes how to access texels while they outside the texture dimensions. */
    depthSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depthSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depthSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    /* Describing Anisotropic Filtering. */
    depthSamplerInfo.anisotropyEnable    = VK_TRUE;
    depthSamplerInfo.maxAnisotropy = 16.f;

    /* Describing which color to use if clamp to border addressing mode is used. */
    depthSamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    /* Describing texture coordinate system. Normalized are [0;1] */
    depthSamplerInfo.unnormalizedCoordinates =   VK_FALSE;

    /* Describe way to compare texel value to other values. Mainly used for percentage-closer filtering. */
    depthSamplerInfo.compareEnable   = VK_FALSE;
    depthSamplerInfo.compareOp       = VK_COMPARE_OP_ALWAYS;

    /* Specify mipmapping characteristics. */
    depthSamplerInfo.mipmapMode  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    depthSamplerInfo.mipLodBias  = 0.f;
    depthSamplerInfo.minLod      = 0.f;
    depthSamplerInfo.maxLod      = 1.f;
    
    /* Image Sampler do not refer VkImage object anywhere. It is distinct object that provide interface to extract color from texture. */
    if(vkCreateSampler(_device, &depthSamplerInfo, nullptr, &_offscreen_pass.depth_sampler) != VK_SUCCESS )
        throw std::runtime_error("Failed to create offscreen texture sampler! :( \n");
}

void Simulation::load_model()
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
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH))
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
    
            vertex.color =  {
                /*attrib.colors[3 * index.vertex_index+0],
                attrib.colors[3 * index.vertex_index+1],
                attrib.colors[3 * index.vertex_index+2] */
                0.4f, 0.4f, 0.4f
            };

            /* Check if same vertex has been already read. */
            if( uniqueVertices.count(vertex) == 0 )
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
                _vertices.push_back(vertex);
            }

            _indices.push_back(uniqueVertices[vertex]);
        }
    }

    /* Find minimum Y coordinate value of unordered_map object containing Vertex values. */
    auto minElement = *std::min_element(uniqueVertices.begin(), uniqueVertices.end(),
        [](const auto& l, const auto& r) { return l.first.pos.y < r.first.pos.y; }
    );
    float minY = (minElement.first.pos.y);

    add_quad_under_model(minY, _vertices.size(), 7.f);
}

void Simulation::add_quad_under_model(float minY, int count, float quad_coord)
{
    /* Add floor vertices. */
    glm::vec3 floor_color = { 0.5f, 0.5f, 0.5f };
    glm::vec3 floor_normal = glm::vec3(0.f, 1.f, 0.f);

    Vertex vert = {};
    vert.pos    = { -quad_coord, minY, -quad_coord };
    vert.color  = floor_color;
    vert.normal = floor_normal;
    vert.texCoord = { 0.f, 0.f };
    _vertices.push_back(vert);

    vert.pos        = { -quad_coord, minY, quad_coord };
    vert.texCoord   = { 1.f, 1.f };
    _vertices.push_back(vert);

    vert.pos        = { quad_coord, minY, quad_coord };
    vert.texCoord   = { 0.f, 1.f };
    _vertices.push_back(vert);

    vert.pos        = { quad_coord, minY, -quad_coord };
    vert.texCoord   = { 0.f, 1.f };
    _vertices.push_back(vert);

    _indices.push_back(count + 0); /* v1 */
    _indices.push_back(count + 1); /* v2 */
    _indices.push_back(count + 2); /* v3 */

    _indices.push_back(count + 0); /* v1 */
    _indices.push_back(count + 2); /* v3 */
    _indices.push_back(count + 3); /* v4 */
}

void Simulation::create_depth_resources()
{
    /* Create vkImage object with given properties */
    create_image(_swap_chain.swap_chain_extent.width,
            _swap_chain.swap_chain_extent.height,
            DEPTH_FORMAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _scene_pass.depth.image,
            _scene_pass.depth.memory
        );
    
    /* Crate Image view bound to previously created depth image */
    _scene_pass.depth.image_view = create_image_view(_scene_pass.depth.image, 
        DEPTH_FORMAT, 
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    /* Create Image for depth map. Offscreen */
    create_image(_windowWidth, 
        _windowHeight, 
        DEPTH_FORMAT, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _offscreen_pass.depth.image,
        _offscreen_pass.depth.memory
    );

    /* Crate Image View for crated depth image. Offscreen */
    _offscreen_pass.depth.image_view = create_image_view(_offscreen_pass.depth.image,
        DEPTH_FORMAT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void Simulation::create_vertex_buffer()
{
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(_vertices[0])) * _vertices.size();

    /* Temporary host buffer to copy data from CPU to GPU */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    create_buffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Mapping staging buffer memory and filling it with vertex data. */
    void* data;
    if( vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS )
        throw std::runtime_error("Failed to create staging buffer. :( \n");
    
    /* Copy vertices data to staging buffer */
    memcpy(data, _vertices.data(), (size_t)bufferSize);

    /* Unmap previously mapped memory object as data was copied */
    vkUnmapMemory(_device, stagingBufferMemory);

    /* Create Vertex Buffer on GPU */
    create_buffer(bufferSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _vertex_buffer,
        _vertex_buffer_memory);

    /* Copy data from staging buffer to high performance memory on GPU */
    copy_buffer(stagingBuffer, _vertex_buffer, bufferSize);

    /* Clean up resources */
    vkDestroyBuffer(_device, stagingBuffer, nullptr);
    vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void Simulation::create_index_buffer()
{
    VkDeviceSize bufferSize = static_cast<uint64_t>(sizeof(_indices[0])) * _indices.size();
    
    /* With additional staging buffer copy data to GPU memory. */
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    /* Create buffer for storing indices data. */
    create_buffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    /* Map memory to hold indices data. */
    void* data;
    if(vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS )
        throw std::runtime_error("Failed to create indices staging buffer! :( \n");
    
    /* Copy data to mapped memory. */
    memcpy(data, _indices.data(), (size_t)bufferSize);

    /* Unmap memory to free resources. */
    vkUnmapMemory(_device, stagingBufferMemory);

    /* Create buffer to hold indices data on GPU. */
    create_buffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _index_buffer,
        _index_buffer_memory);

    /* Copy data to GPU indices buffer */
    copy_buffer(stagingBuffer, _index_buffer, bufferSize);

    /* Free resources */
    vkDestroyBuffer(_device, stagingBuffer, nullptr);
    vkFreeMemory(_device, stagingBufferMemory, nullptr);
}

void Simulation::create_uniform_buffers()
{
    VkDeviceSize bufferSize = sizeof(_scene_uniform_buf_obj);

    /* Resize array of buffers to hold buffer for every image in _swap_chain.swap_chain */
    _scene_uniform_buffers.resize(_swap_chain.swap_chain_images.size());
    _scene_uniform_buf_memory.resize(_swap_chain.swap_chain_images.size());

    /* Create Buffer for every of uniformBuffer array member */
    for( size_t i = 0; i<_swap_chain.swap_chain_images.size(); i++)
    {
        create_buffer(bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _scene_uniform_buffers[i],
            _scene_uniform_buf_memory[i]
        );
    }

    /* Initialize uniform buffer object for offscreen render pass */
    _offscreen_buffer.size          = sizeof(UBOOffscreenVS);
    _offscreen_buffer.usageFlags    = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    _offscreen_buffer.memory_property_flags   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    /* Create Buffer for offscreen rendering */
    create_buffer(_offscreen_buffer.size,
        _offscreen_buffer.usageFlags,
        _offscreen_buffer.memory_property_flags,
        _offscreen_buffer.buffer,
        _offscreen_buffer.memory
    );

    // Initialize a default descriptor that covers the whole buffer size
    _offscreen_buffer.descriptor.offset     = 0;
    _offscreen_buffer.descriptor.buffer     = _offscreen_buffer.buffer;
    _offscreen_buffer.descriptor.range      = VK_WHOLE_SIZE; 
}

void Simulation::create_descriptor_pool()
{
    /* Provide information about descriptors type of our descriptor sets and how many of them. 
    *  This structure is referenced in by the main VkDescriptorPoolCreateInfo structure. 
    */
    std::array<VkDescriptorPoolSize, 2> poolSize = {};
    poolSize[0].type    = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    /* Which descriptors types this pool is going to contain. */
    poolSize[0].descriptorCount     = static_cast<uint32_t>(_swap_chain.swap_chain_images.size() + 1);  
    poolSize[1].type    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[1].descriptorCount     = static_cast<uint32_t>(_swap_chain.swap_chain_images.size() + 1);


    /* Allocate one pool which can contain up to swap images count descriptors sets. */
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType  = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount  = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes     = poolSize.data();
    poolInfo.maxSets        = static_cast<uint32_t>(_swap_chain.swap_chain_images.size() + 1);
    poolInfo.flags          = 0; /* Default Value */

    if(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptor_pool) != VK_SUCCESS )
        throw std::runtime_error("Failed to create descriptor pool. :( \n");
}

void Simulation::create_descriptor_sets()
{
    /* We will create one descriptor set for each swap chain image- all with the same layout. */
    std::vector<VkDescriptorSetLayout> layouts(_swap_chain.swap_chain_images.size(), _descriptor_set_layout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool        = _descriptor_pool;
    allocInfo.descriptorSetCount    = static_cast<uint32_t>(_swap_chain.swap_chain_images.size());
    allocInfo.pSetLayouts           = layouts.data();

    /* Allocate every descriptor set */
    _descriptor_sets.scene.resize(_swap_chain.swap_chain_images.size());
    if(vkAllocateDescriptorSets(_device, &allocInfo, _descriptor_sets.scene.data()) != VK_SUCCESS )
        throw std::runtime_error("Failed to allocate descriptor sets. :( \n");

    /* Configure each descriptor. */
    for( size_t i = 0; i<_swap_chain.swap_chain_images.size(); i++)
    {
        /* Specify UBO information */
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer   = _scene_uniform_buffers[i];
        bufferInfo.offset   = 0;
        bufferInfo.range    = VK_WHOLE_SIZE;  /* If Updating whole buffer - we can use VK_WHOLE_SIZE */
    
        /* Specify Sampler information */
        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        imageInfo.imageView     = _offscreen_pass.depth.image_view;
        imageInfo.sampler       = _offscreen_pass.depth_sampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrite = {};
        /* Descriptor set for buffer object. */
        descriptorWrite[0].sType   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[0].dstSet  = _descriptor_sets.scene[i];
        descriptorWrite[0].dstBinding      = 0;    /* Destination binding in shader */
        descriptorWrite[0].dstArrayElement = 0;    /* Descriptors set can be an arrays, so we have to provide element to update. */
        
        descriptorWrite[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite[0].descriptorCount = 1;

        descriptorWrite[0].pBufferInfo     = &bufferInfo;      /* Array with the descriptors count structs. */
        descriptorWrite[0].pImageInfo      = nullptr;          /* Optional */
        descriptorWrite[0].pTexelBufferView = nullptr;         /* Optional */

        /* Descriptor set for texture sampler image info. */
        descriptorWrite[1].sType   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[1].dstSet  = _descriptor_sets.scene[i];
        descriptorWrite[1].dstBinding      = 1;    /* Destination binding in shader */
        descriptorWrite[1].dstArrayElement = 0;    /* Descriptors set can be an arrays, so we have to provide element to update. */
        
        descriptorWrite[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[1].descriptorCount = 1;

        descriptorWrite[1].pBufferInfo     = nullptr;          /* Optional */
        descriptorWrite[1].pImageInfo      = &imageInfo;       /* Array with the descriptors count structs - image samplers */
        descriptorWrite[1].pTexelBufferView = nullptr;         /* Optional */

        vkUpdateDescriptorSets(_device, 
            static_cast<uint32_t>(descriptorWrite.size()),
            descriptorWrite.data(), 
            0, 
            nullptr
        );
    }



    /* Configure descriptors for offscreen rendering. */
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_descriptor_set_layout;
    if(vkAllocateDescriptorSets(_device, &allocInfo, &_descriptor_sets.offscreen) != VK_SUCCESS )
        throw std::runtime_error("Failed to allocate descriptor sets. :( \n");

    std::array<VkWriteDescriptorSet, 1> writeDescriptorSets = {};
    
    /* Specify UBO information */
    VkDescriptorBufferInfo uboOffscreen = {};
    uboOffscreen.buffer   = _offscreen_buffer.buffer;
    uboOffscreen.offset   = 0;
    uboOffscreen.range    = sizeof(UBOOffscreenVS);  /* If Updating whole buffer - we can use VK_WHOLE_SIZE */
    
    writeDescriptorSets = {};
    /* Descriptor set for buffer object. */
    writeDescriptorSets[0].sType   = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet  = _descriptor_sets.offscreen;
    writeDescriptorSets[0].dstBinding      = 0;    /* Destination binding in shader */
    writeDescriptorSets[0].dstArrayElement = 0;    /* Descriptors set can be an arrays, so we have to provide element to update. */
        
    writeDescriptorSets[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].descriptorCount = 1;

    writeDescriptorSets[0].pBufferInfo     = &_offscreen_buffer.descriptor;      /* Array with the descriptors count structs. */
    writeDescriptorSets[0].pImageInfo      = nullptr;          /* Optional */
    writeDescriptorSets[0].pTexelBufferView = nullptr;         /* Optional */

    vkUpdateDescriptorSets(_device, 
        static_cast<uint32_t>(writeDescriptorSets.size()),
        writeDescriptorSets.data(), 
        0, 
        nullptr
    );
}

void Simulation::create_command_buffers()
{
    // Allocate and record commands for each swap chain image.
    _command_buffers.resize(_scene_pass.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool           = _command_pool;
    allocInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  //Can be submitted to a queue for execution, but cannot be called from other command buffers.
    allocInfo.commandBufferCount    = static_cast<uint32_t>(_command_buffers.size());
    
    if( vkAllocateCommandBuffers(_device, &allocInfo, _command_buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers. :( \n");

     /* Clear values - specify clear operation.
     * Order of clear values should be same as attachments.
     */
     std::array<VkClearValue, 2> clearValues;

    // Starting command buffer recording
    for( size_t i = 0; i<_command_buffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                    // Optional
        beginInfo.pInheritanceInfo  = nullptr;  // Optional

        /* If the command buffer was already recorded once, then a call to vkBeginCommandBuffer will implicitly reset it. */
        if( vkBeginCommandBuffer( _command_buffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer. :( \n");

        /*
            First render pass: Generate shadow map by rendering the scene from light's POV
        */
        {
            clearValues[0].depthStencil = {1.f, 0};

            VkRenderPassBeginInfo renderPassInfo {};
            renderPassInfo.sType    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass   = _offscreen_pass.render_pass;
            renderPassInfo.framebuffer  = _offscreen_pass.frameBuffer;
            renderPassInfo.renderArea.extent.width      = _windowWidth;
            renderPassInfo.renderArea.extent.height     = _windowHeight;
            renderPassInfo.renderArea.offset            = {0, 0};
            renderPassInfo.clearValueCount              = 1;
            renderPassInfo.pClearValues                 = clearValues.data();

            vkCmdBeginRenderPass(_command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.offscreen);

            VkViewport viewport {};
            viewport.width  = static_cast<float>(_windowWidth);
            viewport.height = static_cast<float>(_windowHeight);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;
            vkCmdSetViewport(_command_buffers[i], 0, 1, &viewport);

            VkRect2D scissor {};
            scissor.extent.height   = _windowWidth;
            scissor.extent.width    = _windowHeight;
            scissor.offset.x    = 0;
            scissor.offset.y    = 0;
            vkCmdSetScissor(_command_buffers[i], 0, 1, &scissor);

            /* Set depth bias. Avoiding artifacts. */
            vkCmdSetDepthBias(_command_buffers[i], 1.25f, 0, 1.75f);

            vkCmdBindDescriptorSets(_command_buffers[i], 
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                _pipeline_layouts.offscreen,
                0,
                1,
                &_descriptor_sets.offscreen,
                0,
                nullptr
            );

            VkBuffer vertexBuffers[] = {_vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(_command_buffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(_command_buffers[i], _index_buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(_command_buffers[i], static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(_command_buffers[i]);
        }



        /*
            Second render pass: Generate scene with applied shadows by using generated previously shadow map.
        */
        {
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass   = _scene_pass.render_pass;
            renderPassInfo.framebuffer  = _scene_pass.framebuffers[i];
            
            /* Define size of render area */
            renderPassInfo.renderArea.offset    = {0,0};
            renderPassInfo.renderArea.extent    = _swap_chain.swap_chain_extent;
            
           
            clearValues[0].color = {0.f, 0.f, 0.f, 1.f};
            clearValues[1].depthStencil = {1.f, 0}; 
            renderPassInfo.clearValueCount  = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues     = clearValues.data();
            
            /* RECORDING */
            vkCmdBeginRenderPass(_command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            
            vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.scene);

            /* Binding vertex buffer */
            VkBuffer vertexBuffers[] = {_vertex_buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(_command_buffers[i], 0, 1, vertexBuffers, offsets);

            /* Binding index buffer */
            vkCmdBindIndexBuffer(_command_buffers[i], _index_buffer, 0, VK_INDEX_TYPE_UINT32);

            /* Bind descriptor sets- to update uniform data. */
            vkCmdBindDescriptorSets(_command_buffers[i], 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                _pipeline_layouts.scene, 
                0, 
                1, 
                &_descriptor_sets.scene[i], 
                0, 
                nullptr);

            /* Draw command by using indexes of vertices. */
            vkCmdDrawIndexed(_command_buffers[i], static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

            /* END RECORDING */
            vkCmdEndRenderPass(_command_buffers[i]);

            if( vkEndCommandBuffer( _command_buffers[i]) != VK_SUCCESS )
                throw std::runtime_error("Failed to record command buffer! :( \n");
        }
    }
}

void Simulation::create_sync_objects()
{
    _sync_obj._image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _sync_obj._render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _sync_obj.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    _sync_obj.images_in_flight.resize(_swap_chain.swap_chain_images.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create fence as signaled. Initial frame should not now wait for previous frame- means for ever.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for(size_t i = 0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        if( vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_sync_obj._image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_sync_obj._render_finished_semaphores[i]) != VK_SUCCESS || 
            vkCreateFence(_device, &fenceInfo, nullptr, &_sync_obj.in_flight_fences[i]) != VK_SUCCESS )
        {
            throw std::runtime_error("Failed to create semaphores :( \n");
        }
    }
}

void Simulation::create_surface()
{
    if( glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
    {
        throw new std::runtime_error("Failed to create window surface!");
    }

}

QueueFamilyIndices Simulation::find_queue_families(VkPhysicalDevice device)
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

        if (presentSupport)
            indices.presentFamily = i;

        i++;
    }

    return indices;
}

uint32_t Simulation::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memProperties);
    
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

VkFormat Simulation::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for( VkFormat format : candidates )
    {
        /* VkFormatProperties containing fields..
        *       linearTilingFeatures:   Use cases that are supported with linear tiling
        *       optimalTilingFeatures:  Use cases that are supported with optimal tiling
        *       bufferFeatures:         Use cases that are supported for buffers
        */
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(_physical_device, format, &properties);

        if( tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features )
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features )
            return format;
    }

    throw std::runtime_error("Failed to find supported format! :( \n");
}

VkFormat Simulation::find_depth_format()
{
    return find_supported_format( 
                    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                );
}

SwapChainSupportDetails Simulation::query_swap_chain_support(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if( formatCount != 0 )
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if( presentModeCount != 0 )
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data()); 
    }

    return details;
}

VkSurfaceFormatKHR Simulation::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR Simulation::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for( const auto& availablePresentMode : availablePresentModes )
    {
        if( availablePresentMode == VK_PRESENT_MODE_FIFO_KHR )
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Simulation::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if( capabilities.currentExtent.width != UINT32_MAX )
        return capabilities.currentExtent;
    else
    {
        int width;
        int height;
        glfwGetFramebufferSize(_window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width), 
            static_cast<uint32_t>(height)
        };

        /* 
         * Clamp values of '_windowWidth' and 'windowHeight' between 
         * allowed min and max extents supported by the implementation. 
         */
        actualExtent.width = std::max( capabilities.minImageExtent.width, 
            std::min(capabilities.maxImageExtent.width, actualExtent.width));

        actualExtent.height = std::max( capabilities.minImageExtent.height, 
           std::min( capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkShaderModule Simulation::creates_shader_module(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if( vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS )
        throw new std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

void Simulation::create_buffer(VkDeviceSize deviceSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
    /* Specify memory desired type and size */
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size     = deviceSize;                           /* Size of buffer in bytes */
    bufferInfo.usage    = usage;
    bufferInfo.flags    = 0;                                    /* No additional parameters */
    bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS )
        throw std::runtime_error("Failed to create vertex buffer. :( \n");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = find_memory_type(memRequirements.memoryTypeBits, properties ); // Mapped memory always matches the contents of the allocated memory.
 
    if(vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory))
        throw std::runtime_error("Failed to allocate vertex buffer memory! :( \n");

    /* Bind created memory to vertex buffer object */
    vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

void Simulation::create_image(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imgTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags imgMemoryProperties, VkImage & image, VkDeviceMemory & imgMemory)
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

    if(vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS )
        throw std::runtime_error("Failed to create image. :( \n");

    /* Allocate memory for image memory. As like for the buffer. 
    *  Query for memory requirements for previously created VkImage object.
    *  Set up allocation info structure.
    *  Allocate memory for VkDeviceMemory object.
    *  Bind Allocated memory with previously created VkImage object.
    */
    VkMemoryRequirements memRequirements = {};
    vkGetImageMemoryRequirements(_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = memRequirements.size;
    allocInfo.memoryTypeIndex   = find_memory_type(memRequirements.memoryTypeBits, imgMemoryProperties);

    if(vkAllocateMemory(_device, &allocInfo, nullptr, &imgMemory) != VK_SUCCESS )
        throw std::runtime_error("Failed to allocate image memory. :( \n");
    
    /* Bind image object with image memory object. */
    vkBindImageMemory(_device, image, imgMemory, 0);
}

VkImageView Simulation::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
    if(vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS )
        throw std::runtime_error("Failed to create image view. :( \n");

    return imageView;
}

void Simulation::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    /* Begin recording command buffer */
    VkCommandBuffer commandBuffer = began_single_time_commands();

    /* Provide information about regions to copy from and to. Call CopyBuffer command. */
    VkBufferCopy copyRegion = {};
    copyRegion.size         = size;
    copyRegion.dstOffset    = 0;
    copyRegion.srcOffset    = 0;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /* End commands recording. */
    end_single_time_commands(commandBuffer);
}

void Simulation::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = began_single_time_commands();
    
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

    end_single_time_commands(commandBuffer);
}

void Simulation::update_variables(uint32_t imageIndex)
{
    /* Update Time information */
    update_DT();

    /* Update mouse move variables */
    update_mouse_input();

    /* According to mouse offset values update camera pitch/yaw/roll */
    _camera.updateMouseInput(_time.dt, _mouse_input.mouse_offset_X, _mouse_input.mouse_offset_Y);

    /* Check keyboard input. */
    update_keyboard_input();

    /* Update light position */
    update_light();

    /* Update offscreen uniform buffer */
    update_offscreen_uniform_buf();

    /* With information about current image we can update its uniform buffer. */
    update_scene_uniform_buf(imageIndex);
}

void Simulation::update_DT()
{
    _time.currTime = static_cast<float>(glfwGetTime());
    _time.dt = _time.currTime - _time.lastTime;
    _time.lastTime = _time.currTime;
}

void Simulation::update_scene_uniform_buf(uint32_t currentImage)
{
    /* Update variables inside uniform buffer */
    glm::mat4 modelMat  = glm::mat4(1.f);
    glm::mat4 viewMat   = _camera.getViewMatrix();
    glm::mat4 projMat   = glm::perspective(glm::radians(_light.light_FOV),
                        _swap_chain.swap_chain_extent.width / static_cast<float>(_swap_chain.swap_chain_extent.height),
                        0.1f,
                        50.f);
    /* GLM was originally designed for OpenGL, it is important to revert scaling factor of Y axis. */
    projMat[1][1] *= -1;

    _scene_uniform_buf_obj.modelMat     = modelMat;
    _scene_uniform_buf_obj.viewProjMat  = projMat * viewMat;

    _scene_uniform_buf_obj.cameraPos    = glm::vec4(_camera.getPosition(), 1.f);
    _scene_uniform_buf_obj.DepthMVP     = _offscreen_uniform_buf_obj.proj * _offscreen_uniform_buf_obj.view * _offscreen_uniform_buf_obj.model;
    _scene_uniform_buf_obj.lightPos     = glm::vec4(_light.light_pos, 1.f);

    /* With providing this information, we can now map memory of the uniform buffer. */
    void* data;
    vkMapMemory(_device, 
        _scene_uniform_buf_memory[currentImage], 
        0,
        VK_WHOLE_SIZE,
        0,
        &data );
    memcpy(data, &_scene_uniform_buf_obj, sizeof(_scene_uniform_buf_obj));
    vkUnmapMemory(_device, _scene_uniform_buf_memory[currentImage]);
}

void Simulation::update_offscreen_uniform_buf()
{
    // Matrix from light's point of view
    _offscreen_uniform_buf_obj.proj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 50.f);

    /* GLM was originally designed for OpenGL, it is important to revert scaling factor of Y axis. */
    _offscreen_uniform_buf_obj.proj[1][1] *= -1;
    _offscreen_uniform_buf_obj.view = glm::lookAt(_light.light_pos, glm::vec3(0.0f, 0.f, 0.f), glm::vec3(0, 1, 0));
    _offscreen_uniform_buf_obj.model = glm::mat4(1.0f);

    void* data;
    vkMapMemory( _device,
        _offscreen_buffer.memory,
        0,
        VK_WHOLE_SIZE,
        0, 
        &data);
    memcpy(data, &_offscreen_uniform_buf_obj, sizeof(_offscreen_uniform_buf_obj));
    vkUnmapMemory(_device, _offscreen_buffer.memory);
}

void Simulation::update_keyboard_input()
{
    // Application
    if( glfwGetKey( _window, GLFW_KEY_ESCAPE ) == GLFW_PRESS )
    {
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    }

    // Camera
    if( glfwGetKey( _window, GLFW_KEY_W ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, FORWARD);
    }

    if( glfwGetKey( _window, GLFW_KEY_S ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, BACKWARD);
    }

    if( glfwGetKey( _window, GLFW_KEY_A ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, LEFT);
    }

    if( glfwGetKey( _window, GLFW_KEY_D ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, RIGTH);
    }

    if( glfwGetKey( _window, GLFW_KEY_SPACE ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, UPWARD);
    }

    if( glfwGetKey( _window, GLFW_KEY_C ) == GLFW_PRESS )
    {
        _camera.move(_time.dt, DOWNWARD);
    }

    if( glfwGetKey( _window, GLFW_KEY_L ) == GLFW_PRESS )
    {
        _light.move_light = !_light.move_light;
    }

}

void Simulation::update_mouse_input()
{
    glfwGetCursorPos(_window, &_mouse_input.mouse_X, &_mouse_input.mouse_Y);

    if(_mouse_input.firstMouse)
    {
        _mouse_input.last_mouse_X = _mouse_input.mouse_X;
        _mouse_input.last_mouse_Y = _mouse_input.mouse_Y;
        _mouse_input.firstMouse = false;
    }

    /* Calculate mouse move offset */
    _mouse_input.mouse_offset_X  = _mouse_input.mouse_X - _mouse_input.last_mouse_X;
    _mouse_input.mouse_offset_Y  = _mouse_input.mouse_Y - _mouse_input.last_mouse_Y;

    /* Set last X and Y values. */
    _mouse_input.last_mouse_X    = _mouse_input.mouse_X;
    _mouse_input.last_mouse_Y    = _mouse_input.mouse_Y;
}

void Simulation::update_light()
{
    if( _light.move_light )
    {
        _light.angle += _time.dt * 0.7f;

        if (_light.angle > glm::two_pi<float>()) 
            _light.angle -= glm::two_pi<float>();

        float new_x = std::cosf(_light.angle) * 7.f;
        float new_z = std::sinf(_light.angle) * 7.f;

        _light.light_pos.x = new_x;
        _light.light_pos.z = new_z;
    }
}

void Simulation::transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = began_single_time_commands();

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

    end_single_time_commands(commandBuffer);
}

VkCommandBuffer Simulation::began_single_time_commands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool           = _command_pool;
    allocInfo.commandBufferCount    = 1;

    /* Allocate command buffer according to allocation structure */
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo  = nullptr;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Simulation::end_single_time_commands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType    = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;

    vkQueueSubmit(_queues.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queues.graphics_queue);
    
    vkFreeCommandBuffers(_device, _command_pool, 1, &commandBuffer);
}

void Simulation::recreate_swap_chain()
{
    /* Handling Window minimization - size of framebuffer is 0 */
    int width;
    int height;
    
    glfwGetFramebufferSize(_window, &width, &height);

    /* Wait till window is returned to foreground. */
    while( width == 0 || height == 0 )
    {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_device);

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_scene_render_pass();
    create_graphics_pipeline();
    create_depth_resources();
    create_scene_framebuffer();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_command_buffers();
}

void Simulation::cleanup_swap_chain()
{
    /* Destroy depth resources */
    vkDestroyImageView(_device, _scene_pass.depth.image_view, nullptr);
    vkDestroyImage(_device, _scene_pass.depth.image, nullptr);
    vkFreeMemory(_device, _scene_pass.depth.memory, nullptr);

    for( size_t i = 0; i < _scene_pass.framebuffers.size(); i++ )
        vkDestroyFramebuffer(_device, _scene_pass.framebuffers[i], nullptr);

    vkFreeCommandBuffers(_device, _command_pool, static_cast<uint32_t>(_command_buffers.size()), _command_buffers.data());

    vkDestroyPipeline(_device, _pipelines.scene, nullptr);
    vkDestroyPipelineLayout(_device, _pipeline_layouts.scene, nullptr);
    vkDestroyRenderPass(_device, _scene_pass.render_pass, nullptr);

    for( size_t i = 0; i < _swap_chain.swap_chain_image_views.size(); i++ )
        vkDestroyImageView(_device, _swap_chain.swap_chain_image_views[i], nullptr);

    vkDestroySwapchainKHR(_device, _swap_chain.swap_chain, nullptr);
    
    for (size_t i = 0; i < _swap_chain.swap_chain_images.size(); i++) 
    {
        vkDestroyBuffer(_device, _scene_uniform_buffers[i], nullptr);
        vkFreeMemory(_device, _scene_uniform_buf_memory[i], nullptr);
    }

    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
}

void Simulation::init_GLFW()
{
    /* Init GLFW */
    if( glfwInit() == GLFW_FALSE )
    {
        std::cout << "ERROR::GLFW_INIT_FAILED  \n";
        glfwTerminate();
    }
}

void Simulation::init_GLFW_window()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    _window = glfwCreateWindow(static_cast<int>(_windowWidth), 
        static_cast<int>(_windowHeight), 
        _windowName.c_str(), 
        nullptr, 
        nullptr);

    if( _window == nullptr)
    {
        std::cout << "ERROR::GLFW_WINDOW_INIT_FAILED  \n";
        glfwTerminate();
    }
    
    /*
    * Set pointer to GLFWwindow inside callbacks functions.
    */
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

    /* Capture cursor as input mode. */
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool Simulation::check_validatio_layer_support()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for( auto layerName : validation_layers) 
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

bool Simulation::is_device_suitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if( extensionSupported )
    {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void Simulation::draw_frame()
{
    // Wait for previous frame to be finished. 
    vkWaitForFences(_device, 1, &_sync_obj.in_flight_fences[_currentFrame], VK_TRUE, UINT64_MAX);

    /*
    *  Perform operations:
    *   Acquire an image from the swap chain.
    *   Execute the command buffer with that image as attachment in the framebuffer.
    *   Return the image to the swap chain for presentation.
    */
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        _device,                   // Logical Device
        _swap_chain.swap_chain,                // Swap chain from which acquire image.
        UINT64_MAX,                     // Timeout in nanoseconds
        _sync_obj._image_available_semaphores[_currentFrame],  // Semaphore to be signalized after using the image
        VK_NULL_HANDLE,                 // Fence - null
        &imageIndex                     // Image index - refers to the VkImage in swapChainImages array.
    );

    if( result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        recreate_swap_chain();
        return;
    }
    else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
    {
        throw std::runtime_error("Failed to acquire swap chain image. :(\n");
    }

    // Check if previous frame is using this image (there is its fence to wait on).
    if( _sync_obj.images_in_flight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(_device, 1, &_sync_obj.images_in_flight[imageIndex], VK_TRUE, UINT64_MAX);

    // Mark the image as now being in use by current frame
    _sync_obj.images_in_flight[imageIndex] = _sync_obj.in_flight_fences[_currentFrame];
    
    /* Update Input and Variables */
    update_variables(imageIndex);

    /* Submit the command buffer */
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { _sync_obj._image_available_semaphores[_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;

    /* Which command buffer to actually submit for execution */
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &_command_buffers[imageIndex];

    /* Which semaphore to signal once the command buffer finished execution */
    VkSemaphore signalSemaphores[]  = { _sync_obj._render_finished_semaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    //Reset fence to be 'unsignaled'.
    vkResetFences(_device, 1, &_sync_obj.in_flight_fences[_currentFrame]);

    if( vkQueueSubmit(_queues.graphics_queue, 1, &submitInfo, _sync_obj.in_flight_fences[_currentFrame]) != VK_SUCCESS )
        throw std::runtime_error("Failed to submit draw command buffer! :(\n");

    /* Presentation - submit the result back to the swap chain */
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType   = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = signalSemaphores; // Which semaphores to wait on before presentation.

    VkSwapchainKHR  swapChains[]    = {_swap_chain.swap_chain};
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = swapChains;   // Which swapchains to present image. 
    presentInfo.pImageIndices       = &imageIndex;

    // Can specify array of VkResult values to check for every individual swap chain if presentation was successful.
    presentInfo.pResults = nullptr; //Optional.

    VkResult presentResult = vkQueuePresentKHR(_queues.present_queue, &presentInfo);

    if( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized )
    {
        _framebufferResized = false;
        recreate_swap_chain();
    }
    else if( result != VK_SUCCESS )
    {
        throw std::runtime_error("Failed to present swap chain image! :(\n");
    }

    vkQueueWaitIdle(_queues.present_queue);

    // Proceed to next frame counter
    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Simulation::main_loop()
{
    while(!glfwWindowShouldClose(_window)) 
    {
        glfwPollEvents();
        draw_frame();
    }

    vkDeviceWaitIdle(_device);
}

void Simulation::cleanup()
{
    cleanup_swap_chain();

    vkDestroyPipeline(_device, _pipelines.offscreen, nullptr);
    vkDestroyPipelineLayout(_device, _pipeline_layouts.offscreen, nullptr);

    vkFreeMemory(_device, _offscreen_buffer.memory, nullptr);
    vkDestroyBuffer(_device, _offscreen_buffer.buffer, nullptr);

    vkDestroyFramebuffer(_device, _offscreen_pass.frameBuffer, nullptr);
    vkDestroyRenderPass(_device, _offscreen_pass.render_pass, nullptr);

    vkDestroySampler(_device, _offscreen_pass.depth_sampler, nullptr);
    vkDestroyImageView(_device, _offscreen_pass.depth.image_view, nullptr);
    vkDestroyImage(_device, _offscreen_pass.depth.image, nullptr);
    vkFreeMemory(_device, _offscreen_pass.depth.memory, nullptr);

    /* Destroy descriptor set layout which is bounding all of the descriptors. */
    vkDestroyDescriptorSetLayout(_device, _descriptor_set_layout, nullptr);

    /* Destroy Index Buffer and allocated to it memory */
    vkDestroyBuffer(_device, _index_buffer, nullptr);
    vkFreeMemory(_device, _index_buffer_memory, nullptr);

    /* Destroy Vertex Buffer and allocated to it memory */
    vkDestroyBuffer(_device, _vertex_buffer, nullptr);
    vkFreeMemory(_device, _vertex_buffer_memory, nullptr);

    for(size_t i = 0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(_device, _sync_obj._image_available_semaphores[i], nullptr);
        vkDestroySemaphore(_device, _sync_obj._render_finished_semaphores[i], nullptr);
        vkDestroyFence(_device, _sync_obj.in_flight_fences[i], nullptr);
    }

    vkDestroyCommandPool(_device, _command_pool, nullptr);

    vkDestroyDevice(_device, nullptr);

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);

    glfwDestroyWindow(_window);
    glfwTerminate();
}
