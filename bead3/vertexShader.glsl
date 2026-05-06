#version 330

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 varyingFragmentPosition;
out vec3 varyingNormal;
out vec2 varyingTexCoord;

uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;
uniform mat3 inverseTransposeMatrix;

void main(void) {
    varyingFragmentPosition = vec3(matModel * vec4(aPosition, 1.0));
    varyingNormal = inverseTransposeMatrix * aNormal;
    varyingTexCoord = aTexCoord; // Textúra koordináták továbbítása
    
    gl_Position = matProjection * matView * vec4(varyingFragmentPosition, 1.0);
}