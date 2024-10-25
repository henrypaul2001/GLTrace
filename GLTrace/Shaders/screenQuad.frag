#version 430 core
out vec4 FragColour;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2DArray screenTexture;

void main() {
	FragColour = texture(screenTexture, vec3(TexCoords, 0));
}