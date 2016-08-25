/*
//VERT
#version 450

layout(std140, binding = 1) uniform viewUniform
{
	mat4 viewTransform;
	mat4 viewProjectionTransform;
	mat4 invViewTransform;
	vec3 cameraPos;
} blkviewUniform;

layout(std140, binding = 0) uniform modelTransform
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} blkmodelTransform;

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertUV0;
layout(location = 2) in uint tangentFrame;

out vs
{
	vec3 pos;
	vec3 vNormal;
	vec3 vTangent;
	vec2 vertUV0;
} blkvs;

void main()
{
	blkvs.pos = vertPos;
	blkvs.vertUV0 = vertUV0;
	gl_Position = blkviewUniform.viewProjectionTransform * vec4(vertPos, 1.0);
}

//FRAG
#version 450

in vs
{
	vec3 pos;
	vec3 vNormal;
	vec3 vTangent;
	vec2 vertUV0;
} blkvs;

layout(location = 0) out vec4 outputColor;

layout(binding = 4) uniform sampler2D debugTex;

void main()
{
	outputColor = texture(debugTex, blkvs.vertUV0);
}
*/