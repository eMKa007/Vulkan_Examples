
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=0 ) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    mat4 depthBiasMVP;
    vec3 lightPos;
} ubo;

/* Input Data - vertex attributes specified per-vertex */
layout( location=0 ) in vec3 inPosition;
layout( location=1 ) in vec3 inColor;
layout( location=2 ) in vec2 inTexCoord;
layout( location=3 ) in vec3 inNormal;

/* Output Data */
layout( location=0 ) out vec3 fragColor;
layout( location=1 ) out vec2 fragTexCoord;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor       = inColor * inNormal;
    fragTexCoord    = inTexCoord;
}