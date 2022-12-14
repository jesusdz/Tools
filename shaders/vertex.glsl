#version 450

//#define EMBEDDED_VERTICES 1
#if USE_EMBEDDED_VERTICES
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);
#else
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
#endif

layout(location = 0) out vec3 fragColor;

void main()
{
#if USE_EMBEDDED_VERTICES
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
#else
	gl_Position = vec4(inPosition, 0.0, 1.0);
	fragColor = inColor;
#endif
}

