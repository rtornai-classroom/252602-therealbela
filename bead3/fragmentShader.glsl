#version 330

in vec3 varyingFragmentPosition;
in vec3 varyingNormal;
in vec2 varyingTexCoord;

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

uniform vec3 lightColor;         // A fényforrásunk (és a Nap) színe
uniform bool u_lightingEnabled;  // Világítás ki/bekapcsoló gomb
uniform bool u_isSun;            // Megadja, hogy épp a fényforrás gömbjét rajzoljuk-e

uniform sampler2D sunTexture;    // Textúra a Naphoz

out vec4 outColor;

void main(void) {
    if (u_isSun) {
        // A NAP RAJZOLÁSA: Megvilágítás nélkül, a textúra és a saját diffúz szín kombinációja
        vec4 texColor = texture(sunTexture, varyingTexCoord);
        outColor = vec4(lightColor, 1.0) * texColor;
    } 
    else {
        // A KOCKÁK RAJZOLÁSA: Színük mindig fehér
        vec3 objectColor = vec3(1.0, 1.0, 1.0);

        if (u_lightingEnabled) {
            // AMBIENT
            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * objectColor;

            // DIFFUSE
            vec3 normalizedNormal = normalize(varyingNormal);
            vec3 lightDirection = normalize(lightPosition - varyingFragmentPosition);
            float diffuseCoEfficient = max(dot(normalizedNormal, lightDirection), 0.0);
            vec3 diffuse = diffuseCoEfficient * lightColor * objectColor;

            // SPECULAR
            float specularStrength = 0.5;
            vec3 viewDirection = normalize(cameraPosition - varyingFragmentPosition);
            vec3 reflectDirection = reflect(-lightDirection, normalizedNormal);
            float specularCoEfficient = pow(max(dot(viewDirection, reflectDirection), 0.0), 32);
            vec3 specular = specularStrength * specularCoEfficient * lightColor; 

            outColor = vec4(ambient + diffuse + specular, 1.0);
        } else {
            // Ha lekapcsoljuk a villanyt, a kockák a saját fehér színükben pompáznak fény nélkül
            outColor = vec4(objectColor, 1.0);
        }
    }
}