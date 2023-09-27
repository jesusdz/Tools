#version 450

layout(location = 0) in vec3 normalWs;
layout(location = 1) in vec2 texCoord;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 albedo = texture(texSampler, texCoord).rgb;// vec3(texCoord, 0.0);//vec3(0.7, 0.8, 1.0);
	vec3 L = normalize(vec3(1.0, 3.0, 2.0));
	vec3 N = normalize(normalWs);
	vec3 lightColor = vec3(1.0);
	vec3 diffuse = 0.8 * clamp(dot(N, L), 0.0, 1.0) * lightColor;
	vec3 ambient = 0.2 * lightColor;
	outColor = vec4((diffuse + ambient) * albedo, 1.0);
}
