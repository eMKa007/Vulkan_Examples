
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=1 ) uniform sampler2D texSampler;

/* Input Variables */
layout( location=0 ) in vec3 fragColor;
layout( location=1 ) in vec2 fragTexCoord;
layout( location=2 ) in vec3 fragCameraPos;

layout( location=3 ) in vec3 fragAmbient;
layout( location=4 ) in vec3 fragDiffuse;
layout( location=5 ) in vec3 fragSpecular;

layout( location=6 ) in vec3 vertexPosition;
layout( location=7 ) in vec3 vertexNormal;

layout( location=8 ) in vec3 lightPos;

/* Output Variables */
layout( location=0 ) out vec4 outColor;

/* Calculate diffuse component based on light position and vertex position */
vec3 calculateDiffuse( vec3 vs_normal, vec3 lightPos0, vec3 vs_position )
{
    vec3 posToLightDirVec = normalize(lightPos0 - vs_position);
    float diffuse = clamp(dot(posToLightDirVec, vs_normal), 0, 1);
    vec3 diffuseFinal = fragDiffuse * diffuse;

    return diffuseFinal;
}

/* Calculate specular component based on light, camera and vertex position. */
vec3 calculateSpecular( vec3 cameraPosition, vec3 vs_position, vec3 vs_normal, vec3 lightPos0 )
{
    vec3 lightToPosDirVec = normalize(vs_position - lightPos0);
    vec3 reflectDirVec  = normalize( reflect(lightToPosDirVec, normalize(vs_normal)));
    vec3 posToViewDirVec = normalize(cameraPosition - vs_position);
    float  SpecularConstant = pow( max( dot( posToViewDirVec, reflectDirVec), 0), 32);

    return vec3(1.f) * SpecularConstant;
}

void main()
{
    /* Ambient light component */
    vec3 ambient = vec3(0.2f);

    /* Specular light component */
    vec3 specular = calculateSpecular( fragCameraPos, vertexPosition, vertexNormal, lightPos );

    /* Diffuse light component */
    vec3 diffuse = calculateDiffuse( vertexNormal, lightPos, vertexPosition );

    /* Out color combined with light components */
    outColor = vec4((ambient + diffuse + specular ) * fragColor, 1.0);
}