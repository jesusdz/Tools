#version 450

layout(std430, push_constant) uniform PushConstants
{
	mat4 model;
} constants;

layout(binding = 0) uniform Globals
{
	mat4 view;
	mat4 proj;
} globals;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 normalWs;
layout(location = 1) out vec2 texCoord;

void main()
{
	gl_Position = globals.proj * globals.view * constants.model * vec4(inPosition, 1.0);
	normalWs = vec3( constants.model * vec4( inNormal, 0.0 ) );
	texCoord = inTexCoord;
}

