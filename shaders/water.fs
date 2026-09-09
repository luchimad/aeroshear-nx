#version 330

// Input vertex attributes from vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform textures
uniform sampler2D texture0; // Water_1.png

// Uniforms de animacion, iluminacion y niebla
uniform float uTime;
uniform vec3 uCameraPos;
uniform vec3 uSunDirection;
uniform vec4 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Escala del mapa de agua en coordenadas de mundo
    vec2 worldUV = fragPosition.xz * 0.015;

    // Animación de doble onda de flujo y marea
    vec2 uv1 = worldUV + vec2(uTime * 0.025, uTime * 0.015);
    vec2 uv2 = worldUV * 1.35 + vec2(-uTime * 0.020, uTime * 0.030) + vec2(0.35, 0.72);

    vec4 waterTex1 = texture(texture0, uv1);
    vec4 waterTex2 = texture(texture0, uv2);
    vec4 waterDiffuse = mix(waterTex1, waterTex2, 0.50);

    // Iluminación solar especular sobre el plano de agua
    vec3 viewDir = normalize(uCameraPos - fragPosition);
    vec3 lightDir = normalize(uSunDirection);
    vec3 normal = vec3(0.0, 1.0, 0.0);

    // Perturbación sutil de la normal con la textura
    normal.x += (waterTex1.r - 0.5) * 0.15;
    normal.z += (waterTex2.r - 0.5) * 0.15;
    normal = normalize(normal);

    // Brillo especular del sol (glint)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specularColor = vec3(1.0, 0.95, 0.85) * spec * 0.65;

    // Luz difusa y ambiente sobre agua
    float NdotL = max(dot(normal, lightDir), 0.0);
    float ambient = 0.60;
    float diffuse = NdotL * 0.40;
    vec3 lighting = vec3(ambient + diffuse);

    vec3 finalRGB = waterDiffuse.rgb * lighting + specularColor;

    // Niebla atmosférica de distancia
    float dist = length(fragPosition - uCameraPos);
    float rawFog = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    float fogFactor = smoothstep(0.0, 1.0, rawFog);

    finalRGB = mix(finalRGB, uFogColor.rgb, fogFactor);

    finalColor = vec4(finalRGB, 0.95);
}
