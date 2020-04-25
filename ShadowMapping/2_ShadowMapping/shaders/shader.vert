
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=0 ) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;

    vec3 cameraPos;

    mat4 depthBiasMVP;
    vec3 lightPos;

    /* Ambient/Diffuse/Specular Lightning */
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} ubo;

/* Input Data - vertex attributes specified per-vertex */
layout( location=0 ) in vec3 inPosition;
layout( location=1 ) in vec3 inColor;
layout( location=2 ) in vec2 inTexCoord;
layout( location=3 ) in vec3 inNormal;

/* Output Data */
layout( location=0 ) out vec3 fragColor;
layout( location=1 ) out vec2 fragTexCoord;
layout( location=2 ) out vec3 fragCameraPos;

layout( location=3 ) out vec3 fragAmbient;
layout( location=4 ) out vec3 fragDiffuse;
layout( location=5 ) out vec3 fragSpecular;

layout( location=6 ) out vec3 vertexPosition;
layout( location=7 ) out vec3 vertexNormal;

layout( location=8 ) out vec3 lightPos;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    fragColor       = inColor;
    fragTexCoord    = inTexCoord;
    fragCameraPos   = ubo.cameraPos;

    /* Light components */
    fragAmbient = ubo.ambient;
    fragDiffuse = ubo.diffuse;
    fragSpecular    = ubo.specular;

    /* Vertex position and normal in world coordinates */
    vertexPosition  = vec4(ubo.model * vec4(inPosition, 1.f)).xyz;
    vertexNormal    = vec4(ubo.model * vec4(inNormal, 1.f)).xyz;

    lightPos =  ubo.lightPos;
}