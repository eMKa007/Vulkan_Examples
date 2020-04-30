
#version 450
//#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=1 ) uniform sampler2D shadowMapTex;

/* Input Variables */
layout (location = 0) in vec4 vertexPosition;
layout (location = 1) in vec4 vertexNormal;
layout (location = 2) in vec4 fragColor;
layout (location = 3) in vec4 fragCameraPos;
layout (location = 4) in vec4 PosLightSpace;
layout (location = 5) in vec4 lightPos;

/* Output Variables */
layout( location=0 ) out vec4 outColor;

#define AMBIENT 0.2

/* Calculate diffuse component based on light position and vertex position */
vec3 calculateDiffuse( vec3 vs_normal, vec3 lightPos0, vec3 vs_position )
{
    vec3 posToLightDirVec = normalize(lightPos0 - vs_position);
    float diffuse = clamp(dot(posToLightDirVec, vs_normal), 0, 1);
    
    return vec3(diffuse);
}

/* Calculate specular component based on light, camera and vertex position. */
vec3 calculateSpecular( vec3 cameraPosition, vec3 vs_position, vec3 vs_normal, vec3 lightPos0 )
{
    vec3 lightToPosDirVec = normalize(vs_position - lightPos0);
    vec3 reflectDirVec  = normalize( reflect(lightToPosDirVec, normalize(vs_normal)));
    vec3 posToViewDirVec = normalize(cameraPosition - vs_position);
    float  SpecularConstant = pow( max( dot( posToViewDirVec, reflectDirVec), 0), 32);

    return vec3(1.0) * SpecularConstant;
}

/* Check if fragment is placed in shadow */
float shadowCalc(vec4 shadowCoord)
{
    float shadow = 0.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
    {
        float dist = texture( shadowMapTex, shadowCoord.st).r;
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
        {
            shadow = 1.0;
        }
    }
    return shadow;
}

void main()
{
    /* Ambient light component */
    vec3 ambient = vec3(AMBIENT);

    /* Specular light component */
    vec3 specular = calculateSpecular( fragCameraPos.xyz, vertexPosition.xyz, vertexNormal.xyz, lightPos.xyz );

    /* Diffuse light component */
    vec3 diffuse = calculateDiffuse( vertexNormal.xyz, lightPos.xyz, vertexPosition.xyz );

    /* Calculate shadow */
    float shadow = shadowCalc(PosLightSpace/PosLightSpace.w);

    /* Out color combined with light components */
    outColor = vec4((ambient + (1.0 - shadow) * (diffuse + specular) ) * fragColor.xyz, 1.0);
}