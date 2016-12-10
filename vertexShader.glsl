#version 420 core

layout(location=0) in vec4 terrainCoords;
layout(location=1) in vec3 Normals;
layout(location=2) in vec2 texCoords;
uniform mat4 projMat;
uniform mat4 modelViewMat;
uniform mat3 normalMat;

out vec2 texCoordsExport;
out vec3 normalExport;

void main(void)
{
	normalExport = normalize(normalMat * Normals);
	texCoordsExport = texCoords;
	gl_Position = projMat * modelViewMat * terrainCoords;
}