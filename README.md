# Vulkan Examples
Vulkan API learning repository. 

Within this repository aditional libaries are used:
   * [GLFW](https://github.com/glfw/glfw) - window and input handling.
   * [GLM](https://github.com/g-truc/glm) - C++ mathematics library for graphics software.
   * [tinyobjectloader](https://github.com/tinyobjloader/tinyobjloader) - Loading models from *.obj files.
   * [stb image](https://github.com/nothings/stb) - Loading images to be used as textures.

## Vulkan_Tutorial
Project which follows "Vulkan Tutorial" by Alexander Overvoorde available on [vulkan-tutorial website](https://vulkan-tutorial.com/) and [github repository](https://github.com/Overv/VulkanTutorial).

Main topics covered in tutorial:
  * Instance and physical device selection
  * Validation Layers
  * Logical device and queue families
  * Swap chains
  * Shader Modules 
  * Fixed functions and render passes
  * Descriptor pool and sets
  * Command buffers
  * Image view and sampler
  * Depth buffering 

-----
## ShadowMapping
Calculating shadows based on 'Vulkan_Tutorial' project and Mr. Sascha Willems "Vulkan-Example" repository available [here](https://github.com/SaschaWillems/Vulkan/tree/master/examples/shadowmapping). 

Shadow creation is divided into two render passes:
   * First one- 'offscreen' render pass is used to calculate depth map from light's point of view (ortographic projection to simulate sunlight). Calculated depth map is stored into texture and used in second render pass.
   * Second one- 'scene' render pass uses depth texture to determine if specified fragment is placed within or not in shadow.
   
Logic of shadow mapping technique is well described in [learningopengl.com website](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping) and [opengl-tutorial.org](https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/)

Camera movement:
   * W/A/S/D keys to move camera forward/left/backward/right.
   * Mouse to rotate camera around.

<img src="https://github.com/eMKa007/Vulkan_Examples/blob/Shadow_Mapping/screens/shadow_map_vulkan.gif?raw=true" width="500" height="350" />
