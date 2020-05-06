
#version 450
//#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=0 ) uniform UniformBufferObject {
    mat4 modelMat;
    mat4 viewProjMat;

    vec4 cameraPos;

    /* Model-View-Projection matrix from lights POV */
    mat4 DepthMVP;

    /* Light Position */
    vec4 lightPos;
} ubo;

/* Input Data - vertex attributes specified per-vertex */
layout( location=0 ) in vec3 inPosition;
layout( location=1 ) in vec3 inColor;
layout( location=2 ) in vec2 inTexCoord;
layout( location=3 ) in vec3 inNormal;

/* Output Data */
layout (location = 0) out vec4 vertexPosition;
layout (location = 1) out vec4 vertexNormal;
layout (location = 2) out vec4 fragColor;
layout (location = 3) out vec4 fragCameraPos;
layout (location = 4) out vec4 PosLightSpace;
layout (location = 5) out vec4 lightPos;

const mat4 biasMat = mat4( 
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0 
);

void main() 
{
    gl_Position = ubo.viewProjMat * ubo.modelMat * vec4(inPosition, 1.0);

    /* Vertex position and normal in world coordinates */
    vertexPosition  = vec4(ubo.modelMat * vec4(inPosition, 1.0));
    vertexNormal    = vec4(ubo.modelMat * vec4(inNormal, 1.0));
    fragColor       = vec4(inColor, 1.0);

    fragCameraPos   = ubo.cameraPos;

    /* Light space Matrix */
    PosLightSpace = biasMat * ubo.DepthMVP * ubo.modelMat * vec4(inPosition, 1.0);

    /* Light Position */
    lightPos = ubo.lightPos;
}