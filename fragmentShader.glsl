#version 430

out vec4 color;

uniform bool isLine;
uniform bool swapColors;

void main() {
    if (isLine) {
        color = vec4(0.0, 0.0, 1.0, 1.0); // Kék szakasz
    } else {
        // gl_PointCoord (0,0)-tól (1,1)-ig megy a ponton belül
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist = length(coord);

        if (dist > 0.5) discard; // Körön kívüli pixelek elvetése

        // Színek definiálása
        vec3 red = vec3(1.0, 0.0, 0.0);
        vec3 green = vec3(0.0, 1.0, 0.0);

        vec3 centerCol = swapColors ? green : red;
        vec3 edgeCol = swapColors ? red : green;

        // Lineáris interpoláció a középpont (0.0) és a szél (0.5) között
        float t = dist / 0.5;
        vec3 finalCol = mix(centerCol, edgeCol, t);

        color = vec4(finalCol, 1.0);
    }
}