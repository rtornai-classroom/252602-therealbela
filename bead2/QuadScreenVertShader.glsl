#version 400 core

layout (location=0) in vec4 vPosition;

uniform mat4 matModelView;
uniform mat4 matProjection;

void main() {
	gl_Position = matProjection * matModelView * vPosition;
}