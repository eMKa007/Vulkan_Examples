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
    VkSurfaceCapabilitiesKHR capabilities {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

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
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() 
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

        /* Description of pos attribute */
        attributeDescriptions[0].binding    = 0;
        attributeDescriptions[0].location   = 0;  /* References to location inside Vertex Shader - 0 for position. */
        attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;  /* Format of vertex position data - vec3 */
        attributeDescriptions[0].offset     = offsetof(Vertex, pos);
        
        /* Description of color attribute */
        attributeDescriptions[1].binding    = 0;
        attributeDescriptions[1].location   = 1;
        attributeDescriptions[1].format     = VK_FORMAT_R32G32B32_SFLOAT;   /* vec3 format */
        attributeDescriptions[1].offset     = offsetof(Vertex, color);

        /* Description of Texture coordinate attribute */
        attributeDescriptions[2].binding    = 0;
        attributeDescriptions[2].location   = 2;
        attributeDescriptions[2].format     = VK_FORMAT_R32G32_SFLOAT;      /* vec2 format */
        attributeDescriptions[2].offset     = offsetof(Vertex, texCoord);

        /* Description of normal vector attribute */
        attributeDescriptions[3].binding    = 0;
        attributeDescriptions[3].location   = 3;
        attributeDescriptions[3].format     = VK_FORMAT_R32G32B32_SFLOAT;      /* vec3 format */
        attributeDescriptions[3].offset     = offsetof(Vertex, normal);

        return attributeDescriptions;
    }

    /* Override == operator to specify equality comparison. */
    bool operator==(const Vertex& other) const 
    {
        return (pos == other.pos) && 
            (color      == other.color) && 
            (texCoord   == other.texCoord) && 
            (normal     == other.normal);
    }
};

/* Hash calculation function for unordered map. */
namespace std {
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const noexcept
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ 
                    (hash<glm::vec3>()(vertex.color) << 1 )) >> 1) ^
                    (hash<glm::vec2>()(vertex.texCoord) << 1) ^
                    (hash<glm::vec3>()(vertex.normal) << 1);
        }
    };
}


class Simulation
{
public:
    Simulation( unsigned int windowHeight, unsigned int windowWidth, std::string windowName);
    ~Simulation();

    unsigned int _windowWidth;
    unsigned int _windowHeight;
    std::string _windowName;
    GLFWwindow* _window = nullptr;

    /* Handling Window Resize Explicity */
    bool _framebufferResized = false;

    void run();

private:
    /* Delta Time variables */
    struct Time_Count {
        float currTime  = 0.f;
        float dt        = 0.f;
        float lastTime  = 0.f;
    } _time;
    
    /* Camera Object */
    Camera _camera;

    /* Mouse Input Variables */
    struct Mouse_Input {
        bool firstMouse     = true;
        double last_mouse_X = 0.f;
        double last_mouse_Y = 0.f;

        double mouse_X      = 0.f;
        double mouse_Y      = 0.f;

        double mouse_offset_X   = 0.f;
        double mouse_offset_Y   = 0.f;
    } _mouse_input;

    struct Light {
        bool move_light     = true;
        glm::vec3 light_pos = glm::vec3(5.f, 5.f, 5.f);
        float light_FOV     = 45.0f;

        /* Angle variable to move light */
        float angle         = 0.f;
    } _light;
    
    /* Available and enable API extensions */
    std::vector<const char*> validation_layers;
    std::vector<const char*> device_extensions;

    /* Instance */
    VkInstance instance = nullptr;

    /* Surface */
    VkSurfaceKHR surface = 0;

    /* Device */
    VkPhysicalDevice    physical_device = VK_NULL_HANDLE;
    VkDevice            device          = nullptr;

    /* Current used frame */
    size_t _currentFrame = 0;

    /* Queues */
    struct Queues {
        VkQueue graphics_queue;
        VkQueue present_queue;
    } _queues;
    
    /* Swap chain */
    struct Swapchain {
        VkSwapchainKHR  swap_chain {};
        VkFormat        swap_chain_image_format {};
        VkExtent2D      swap_chain_extent {};

        /* Swap chain image handles */
        std::vector<VkImage> swap_chain_images {};

        /* Image Views */
        std::vector<VkImageView> swap_chain_image_views {};
    } _swap_chain;

    /* Descriptor pool to hold descriptors set. */
    VkDescriptorPool        _descriptor_pool;
    /* Descriptors Layout - all of the descriptors are combined into single descriptor set layout. */
    VkDescriptorSetLayout   _descriptor_set_layout;

    struct FrameBufferAttachment {
        VkImage         image;
        VkDeviceMemory  memory;
        VkImageView     image_view;
    };

    struct OffscreenPass {
        VkFramebuffer           frameBuffer;
        FrameBufferAttachment   depth;
        VkRenderPass            render_pass;
        VkSampler               depth_sampler;
        VkDescriptorImageInfo   descriptor;
    } _offscreen_pass;

    struct ScenePass {
        /* Separate framebuffer for each swapchain image. */
        std::vector<VkFramebuffer>  framebuffers {};

        /* Depth testing requires three resources- image, memory and image view. */
        FrameBufferAttachment       depth {};
        VkRenderPass                render_pass {};
        VkSampler                   depth_sampler {};
        VkDescriptorImageInfo       descriptor {};
    } _scene_pass;

    struct {
        VkPipelineLayout offscreen;
        VkPipelineLayout scene;
    } _pipeline_layouts;

    struct {
        /* Offscreen rendering pipeline */
        VkPipeline offscreen;
        /* Main graphics pipeline */
        VkPipeline scene;
    } _pipelines;

    struct {
        VkDescriptorSet                 offscreen {};
        std::vector<VkDescriptorSet>    scene {};
    } _descriptor_sets;

    /* Command Pool- to store commands */
    VkCommandPool _command_pool;

    /* Command Buffers- to record commands */
    std::vector<VkCommandBuffer> _command_buffers;

    struct Sync_Objects {
        /* Semaphore- signals that an image has been acquired and is ready for rendering. */
        std::vector<VkSemaphore> _image_available_semaphores;
    
        /* Semaphore- signals that rendering has finished and presentation can happen. */
        std::vector<VkSemaphore> _render_finished_semaphores;

        /* CPU-GPU synchronization fences */
        std::vector<VkFence> in_flight_fences;
        std::vector<VkFence> images_in_flight;
    } _sync_obj;

    /* Vertex and Indices Variables */
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    VkBuffer        _vertex_buffer;
    VkDeviceMemory  _vertex_buffer_memory;
    VkBuffer        _index_buffer;               /* Index data for corresponding vertex buffer. */
    VkDeviceMemory  _index_buffer_memory;

    /* Uniform Buffers - they'll be update after every frame so every image in swapchain will have own uniform buffer. */
    std::vector<VkBuffer>       _scene_uniform_buffers;
    std::vector<VkDeviceMemory> _scene_uniform_buf_memory;

    struct {
        VkBuffer                buffer = VK_NULL_HANDLE;
        VkDeviceMemory          memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo  descriptor;
        VkDeviceSize            size = 0;
        VkDeviceSize            alignment = 0;
        VkBufferUsageFlags      usageFlags {};
        VkMemoryPropertyFlags   memory_property_flags {};
    } _offscreen_buffer;

    struct UBOOffscreenVS {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } _offscreen_uniform_buf_obj;

    struct {
        glm::mat4 modelMat;
        glm::mat4 viewProjMat;

        /* Camera position */
        glm::vec4 cameraPos;
        
        /* Model-View-Projection matrix from lights POV */
        glm::mat4 DepthMVP;

        glm::vec4 lightPos;
    } _scene_uniform_buf_obj;

#ifdef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

    /* Initialize Vulkan API */
    void init_vulkan();
    void create_instance();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    bool check_device_extension_support(VkPhysicalDevice device);
    void create_swap_chain();
    void create_image_views();
    void create_scene_render_pass();
    void create_offscreen_render_pass();
    void create_descriptor_set_layout();
    void create_graphics_pipeline();
    void create_depth_resources();
    void create_depth_texture_sampler();
    void create_scene_framebuffer();
    void create_offscreen_framebuffer();
    void create_command_pool();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();
    void create_command_buffers();
    void create_sync_objects();

    /* Initialize GLFW */
    void init_GLFW();
    void init_GLFW_window();

    /* Load model using tiny_obj_loader library */
    void load_model();

    /* Auxiliary Functions */
    bool                    check_validatio_layer_support();
    bool                    is_device_suitable( VkPhysicalDevice device );

    QueueFamilyIndices      find_queue_families(VkPhysicalDevice device);
    uint32_t                find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat                find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat                find_depth_format();
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    VkSurfaceFormatKHR      choose_swap_surface_format( const std::vector<VkSurfaceFormatKHR>& availableFormats );
    VkPresentModeKHR        choose_swap_present_mode( const std::vector<VkPresentModeKHR>& availablePresentModes );
    VkExtent2D              choose_swap_extent( const VkSurfaceCapabilitiesKHR& capabilities );

    VkShaderModule          creates_shader_module( const std::vector<char>& code );
    void                    create_buffer(VkDeviceSize deviceSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void                    create_image(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imgTiling, VkImageUsageFlags imgFlags, 
                                VkMemoryPropertyFlags imgMemoryProperties, VkImage& image, VkDeviceMemory& imgMemory);
    VkImageView             create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void                    copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void                    copy_buffer_to_image( VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void                    add_quad_under_model(float minY, int count, float quad_coord);

    void                    update_variables(uint32_t imageIndex);
    void                    update_DT();
    void                    update_scene_uniform_buf(uint32_t currentImage);
    void                    update_offscreen_uniform_buf();
    void                    update_keyboard_input();
    void                    update_mouse_input();
    void                    update_light();

    void                    transition_image_layout(VkImage image, VkFormat format,
                                VkImageLayout oldLayout, VkImageLayout newLayout);

    VkCommandBuffer         began_single_time_commands();
    void                    end_single_time_commands(VkCommandBuffer commandBuffer);

    void recreate_swap_chain();
    void cleanup_swap_chain();

    /* Drawing */
    void draw_frame();

    void main_loop();
    void cleanup();
};

static std::vector<char> read_file( const std::string& filename )
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

