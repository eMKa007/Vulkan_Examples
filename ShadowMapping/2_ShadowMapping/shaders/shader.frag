
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* Input Data - descriptors, global for all vertex */
layout( binding=1 ) uniform sampler2D texSampler;

/* Input Variables */
layout( location=0 ) in vec3 fragColor;
layout( location=1 ) in vec2 fragTexCoord;

/* Output Variables */
layout( location=0 ) out vec4 outColor;

void main()
{
    /* Out color combined with texture color */
    outColor = vec4( fragColor, 1.0);
}