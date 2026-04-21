#version 400

out vec4 outColor;
uniform bool isPoint;

void main() {
    if (isPoint) {
        // Távolság a pont közepétől (0.5, 0.5)
        float dist = distance(gl_PointCoord, vec2(0.5));
        
        // Sima átmenet a kör szélein (élsimítás)
        // 0.5 a sugár, 0.48-tól kezdjük elmosni a szélét
        float alpha = 1.0 - smoothstep(0.45, 0.5, dist);
        
        // Ha teljesen kívül van, elvetjük a pixelt (teljesítmény miatt)
        if (alpha <= 0.0) {
            discard;
        }
        
        // Piros szín, az átlátszóságot (alpha) használva a kerekítéshez
        outColor = vec4(1.0, 0.0, 0.0, alpha);
    } else {
        // Kék szín a vonalaknak 
        outColor = vec4(0.0, 0.0, 1.0, 1.0);
    }
}