#version 450

layout(location = 0) in vec3 normalWs;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 albedo = vec3(0.7, 0.8, 1.0);
	vec3 L = normalize(vec3(1.0, 3.0, 2.0));
	vec3 N = normalize(normalWs);
	float diffuse = clamp(dot(N, L), 0.0, 1.0);
	vec3 ambient = vec3(0.1, 0.1, 0.1);
	outColor = vec4(diffuse * albedo + ambient, 1.0);
}
