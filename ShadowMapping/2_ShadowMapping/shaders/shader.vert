
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
layout( location = 0 ) out struct VS_OUT {
    vec3 fragColor;
    vec2 fragTexCoord;
    vec3 fragCameraPos;

    vec3 fragAmbient;
    vec3 fragDiffuse;
    vec3 fragSpecular;

    vec3 vertexPosition;
    vec3 vertexNormal;

    vec3 lightPos;
} vs_out;



void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    vs_out.fragColor       = inColor;
    vs_out.fragTexCoord    = inTexCoord;
    vs_out.fragCameraPos   = ubo.cameraPos;

    /* Light components */
    vs_out.fragAmbient = ubo.ambient;
    vs_out.fragDiffuse = ubo.diffuse;
    vs_out.fragSpecular    = ubo.specular;

    /* Vertex position and normal in world coordinates */
    vs_out.vertexPosition  = vec4(ubo.model * vec4(inPosition, 1.f)).xyz;
    vs_out.vertexNormal    = vec4(ubo.model * vec4(inNormal, 1.f)).xyz;

    vs_out.lightPos =  ubo.lightPos;
}