#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

layout(set = 0, binding = 0) uniform Camera {
	mat4 proj;
	mat4 view;
} cam;

layout( push_constant ) uniform constants
{
	mat4 transform;
} PushConstants;


void main() {
	// vec3 bitangent = cross(norm, tangent.xyz) * tangent.w;
	// outTBN = mat3(tangent.xyz, bitangent, norm);
	//
	// outPos = pos.xyz * 0.05;
	// outNormal = norm.xyz;
	// outUV = uv;
    gl_Position = cam.proj * cam.view * PushConstants.transform *  vec4(pos, 1.0);
}
