#version 430

uniform vec2 circleCenter;
uniform float lineY;
uniform bool isLine;

void main() {
    if (isLine) {
        // A vonal két végpontja (fix X, eltolt Y)
        float xCoords[2] = float[2](-0.333, 0.333); // 1/3 szélesség
        gl_Position = vec4(xCoords[gl_VertexID], lineY, 0.0, 1.0);
    } else {
        gl_Position = vec4(circleCenter, 0.0, 1.0);
    }
}