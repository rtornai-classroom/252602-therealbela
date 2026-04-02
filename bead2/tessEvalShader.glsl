#version 430
layout (isolines, equal_spacing, ccw) in;

void main() {
    float t = gl_TessCoord.x;
    int n = gl_PatchVerticesIn;
    
    vec3 p[32]; 
    for (int i = 0; i < n; i++) p[i] = gl_in[i].gl_Position.xyz;

    // De Casteljau iteration
    for (int j = 1; j < n; j++) {
        for (int i = 0; i < n - j; i++) {
            p[i] = mix(p[i], p[i+1], t);
        }
    }
    gl_Position = vec4(p[0], 1.0);
}