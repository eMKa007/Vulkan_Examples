
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=1 ) uniform sampler2D texSampler;

/* Input Variables */
layout( location = 0 ) in struct VS_OUT {
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

/* Output Variables */
layout( location=0 ) out vec4 outColor;

/* Calculate diffuse component based on light position and vertex position */
vec3 calculateDiffuse( vec3 fragDiffuse, vec3 vs_normal, vec3 lightPos0, vec3 vs_position )
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
    vec3 ambient = vec3(0.1f);

    /* Specular light component */
    vec3 specular = calculateSpecular( vs_out.fragCameraPos, vs_out.vertexPosition, vs_out.vertexNormal, vs_out.lightPos );

    /* Diffuse light component */
    vec3 diffuse = calculateDiffuse( vs_out.fragDiffuse, vs_out.vertexNormal, vs_out.lightPos, vs_out.vertexPosition );

    /* Out color combined with light components */
    outColor = vec4((ambient + diffuse + specular ) * vs_out.fragColor, 1.0) * texture(texSampler, gl_FragCoord.xy);
}