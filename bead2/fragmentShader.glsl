#version 430
uniform vec3 uColor;
uniform bool isDrawingPoint;
out vec4 fColor;

void main() {
    if (isDrawingPoint) {
        // Map gl_PointCoord (0 to 1) to a centered coordinate (-0.5 to 0.5)
        vec2 centerDist = gl_PointCoord - vec2(0.5);
        float distSquared = dot(centerDist, centerDist);
        
        // The radius of a circle inside a square is 0.5. 
        // Radius squared is 0.25.
        // We use smoothstep to anti-alias the edge (from 0.23 to 0.25)
        float alpha = 1.0 - smoothstep(0.20, 0.25, distSquared);
        
        if (alpha <= 0.0) discard; // Don't draw the corners of the square
        
        fColor = vec4(uColor, alpha);
    } else {
        fColor = vec4(uColor, 1.0);
    }
}