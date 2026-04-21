#version 400 core

layout (vertices = 1) out; // Elég 1, mert csak a generálást indítjuk el

void main() {
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 1;
        gl_TessLevelOuter[1] = 1024; // A felbontás maradhat magas
    }
    // Nem másolunk gl_out-ba semmit, a pontokat uniformból olvassuk
}