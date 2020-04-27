rem Compiling selected vertex shaders
C:\VulkanSDK\1.2.131.1\Bin32\glslc.exe shader.vert -o vert.spv
C:\VulkanSDK\1.2.131.1\Bin32\glslc.exe offscreen.vert -o offscreen_vert.spv

rem Compiling selected fragment shaders
C:\VulkanSDK\1.2.131.1\Bin32\glslc.exe shader.frag -o frag.spv
C:\VulkanSDK\1.2.131.1\Bin32\glslc.exe offscreen.frag -o offscreen_frag.spv

pause