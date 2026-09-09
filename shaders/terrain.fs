#version 330

// Input vertex attributes from vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform textures (Splatting adaptativo por Bioma)
uniform sampler2D texture0; // Greenhills: Grass1 | Hotsands: Sand1 | Deltastraits: Grass1
uniform sampler2D texture1; // Greenhills: Grass2 | Hotsands: Sand2 | Deltastraits: Grass2
uniform sampler2D texture2; // Greenhills: Grass3 | Hotsands: ---   | Deltastraits: Grass3
uniform sampler2D texture3; // Greenhills: Rocky1 | Hotsands: ---   | Deltastraits: Sand2 (Playa)
uniform sampler2D texture4; // Greenhills: Snow1  | Hotsands: ---   | Deltastraits: Water1 (Agua)

// Uniforms de bioma, iluminacion y camara
uniform int uBiomeType;     // 0: Greenhills, 1: Hotsands, 2: Deltastraits
uniform float uWaterLevel;  // 45.0m en Deltastraits
uniform float uTime;
uniform vec3 uCameraPos;
uniform vec3 uSunDirection;
uniform vec4 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

// Output fragment color
out vec4 finalColor;

// Generador de ruido procedural 2D suave
float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2D(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main()
{
    vec2 worldUV = fragPosition.xz * 0.012;
    float slope = 1.0 - clamp(fragNormal.y, 0.0, 1.0);
    float height = fragPosition.y;

    vec4 terrainDiffuse = vec4(0.5);

    // ========================================================================
    // BIOMA 0: GREEN HILLS (Valles, Pastos, Rocas y Cumbres Nevadas)
    // ========================================================================
    if (uBiomeType == 0) {
        vec4 cGrass1 = texture(texture0, worldUV);
        vec4 cGrass2 = texture(texture1, worldUV * 1.08 + vec2(0.27, 0.63));
        vec4 cGrass3 = texture(texture2, worldUV * 0.92 + vec2(0.54, 0.19));
        vec4 cRocky  = texture(texture3, worldUV * 0.95);
        vec4 cSnow   = texture(texture4, worldUV * 0.85);

        // Inpainting de 3 pastos
        float n1 = noise2D(fragPosition.xz * 0.0022);
        float n2 = noise2D(fragPosition.xz * 0.0055 + vec2(33.1, 77.4));
        float blendNoise = n1 * 0.70 + n2 * 0.30;

        float w1 = smoothstep(0.0, 1.0, clamp(1.0 - abs(blendNoise - 0.20) * 2.2, 0.0, 1.0));
        float w2 = smoothstep(0.0, 1.0, clamp(1.0 - abs(blendNoise - 0.52) * 2.2, 0.0, 1.0));
        float w3 = smoothstep(0.0, 1.0, clamp(1.0 - abs(blendNoise - 0.84) * 2.2, 0.0, 1.0));
        vec4 grassBlend = (cGrass1 * w1 + cGrass2 * w2 + cGrass3 * w3) / (w1 + w2 + w3 + 0.0001);

        // Transición a Roca en laderas (>20°) o altitud media (70m a 180m)
        float rockSlope = smoothstep(0.18, 0.45, slope);
        float rockHeight = smoothstep(80.0, 200.0, height);
        float rockWeight = smoothstep(0.0, 1.0, clamp(rockSlope * 1.2 + rockHeight * 0.7, 0.0, 1.0));
        vec4 groundBase = mix(grassBlend, cRocky, rockWeight);

        // Transición a Nieve en cumbres altas (>190m)
        float snowHeight = smoothstep(190.0, 260.0, height);
        float snowWeight = smoothstep(0.0, 1.0, clamp(snowHeight * (1.0 - slope * 0.5), 0.0, 1.0));
        terrainDiffuse = mix(groundBase, cSnow, snowWeight);
    }
    // ========================================================================
    // BIOMA 1: HOT SANDS (Desierto de Dunas)
    // ========================================================================
    else if (uBiomeType == 1) {
        vec4 cSand1 = texture(texture0, worldUV * 1.15);
        vec4 cSand2 = texture(texture1, worldUV * 0.85 + vec2(0.42, 0.18));

        // Ruido de dunas y ondulaciones de viento
        float duneNoise = noise2D(fragPosition.xz * 0.004) * 0.65 + noise2D(fragPosition.xz * 0.012) * 0.35;
        float sandWeight = smoothstep(0.25, 0.75, duneNoise + slope * 0.4);
        terrainDiffuse = mix(cSand1, cSand2, sandWeight);
    }
    // ========================================================================
    // BIOMA 2: DELTA STRAITS (Ríos, Playas y Archipiélagos)
    // ========================================================================
    else if (uBiomeType == 2) {
        // Coordenadas con sutil flujo de marea
        vec2 waterUV = worldUV * 1.4 + vec2(sin(uTime * 0.8 + fragPosition.x * 0.04) * 0.015, cos(uTime * 0.8 + fragPosition.z * 0.04) * 0.015);
        vec4 cWater  = texture(texture4, waterUV);
        vec4 cSand   = texture(texture3, worldUV * 1.05);
        vec4 cGrass1 = texture(texture0, worldUV);
        vec4 cGrass2 = texture(texture1, worldUV * 1.08 + vec2(0.3, 0.6));
        vec4 cGrass3 = texture(texture2, worldUV * 0.92 + vec2(0.5, 0.2));

        // Mezcla de pastos tropicales en islas
        float islandNoise = noise2D(fragPosition.xz * 0.003);
        float gw1 = smoothstep(0.0, 1.0, clamp(1.0 - abs(islandNoise - 0.25) * 2.0, 0.0, 1.0));
        float gw2 = smoothstep(0.0, 1.0, clamp(1.0 - abs(islandNoise - 0.55) * 2.0, 0.0, 1.0));
        float gw3 = smoothstep(0.0, 1.0, clamp(1.0 - abs(islandNoise - 0.85) * 2.0, 0.0, 1.0));
        vec4 islandGrass = (cGrass1 * gw1 + cGrass2 * gw2 + cGrass3 * gw3) / (gw1 + gw2 + gw3 + 0.0001);

        // Nivel de agua = 45m. Playa = 45m a 48.5m. Pasto = >48.5m
        float waterFade = smoothstep(uWaterLevel - 1.0, uWaterLevel + 1.2, height);
        float beachFade = smoothstep(uWaterLevel + 0.8, uWaterLevel + 3.8, height);

        // Mezclar Agua -> Playa de Arena -> Pasto Isleño
        vec4 coastMix = mix(cWater, cSand, waterFade);
        terrainDiffuse = mix(coastMix, islandGrass, beachFade);
    }
    // ========================================================================
    // BIOMA 3: RED ROCK CANYON (Garganta, Acantilados de Roca y Mesetas)
    // ========================================================================
    else if (uBiomeType == 3) {
        vec4 cGround1 = texture(texture0, worldUV * 1.25);
        vec4 cGround2 = texture(texture1, worldUV * 0.95 + vec2(0.35, 0.45));

        // Coordenadas verticales de estrata para acantilados (evita estiramiento en pendientes)
        vec2 wallUV_X = vec2(fragPosition.z * 0.012, fragPosition.y * 0.022);
        vec2 wallUV_Z = vec2(fragPosition.x * 0.012, fragPosition.y * 0.022);
        float sideBlend = abs(fragNormal.x);
        vec4 cWall1 = mix(texture(texture2, wallUV_Z), texture(texture2, wallUV_X), sideBlend);
        vec4 cWall2 = mix(texture(texture3, wallUV_Z), texture(texture3, wallUV_X), sideBlend);

        float wallMix = noise2D(fragPosition.xz * 0.004) * 0.5 + 0.5;
        vec4 wallColor = mix(cWall1, cWall2, wallMix);

        float groundMix = noise2D(fragPosition.xz * 0.008) * 0.5 + 0.5;
        vec4 groundColor = mix(cGround1, cGround2, groundMix);

        // Transición según pendiente (paredes verticales pasan a roca de estrata)
        float cliffWeight = smoothstep(0.18, 0.42, slope);
        vec4 canyonDiffuse = mix(groundColor, wallColor, cliffWeight);

        // Agua en el lecho si desciende por debajo de uWaterLevel
        if (height < uWaterLevel + 1.5) {
            vec2 waterUV = worldUV * 1.6 + vec2(sin(uTime * 0.6 + fragPosition.x * 0.05) * 0.02, cos(uTime * 0.6 + fragPosition.z * 0.05) * 0.02);
            vec4 cWater = texture(texture4, waterUV);
            float waterFade = smoothstep(uWaterLevel - 1.0, uWaterLevel + 1.2, height);
            canyonDiffuse = mix(cWater, canyonDiffuse, waterFade);
        }

        terrainDiffuse = canyonDiffuse;
    }

    // ========================================================================
    // ILUMINACIÓN SOLAR, ESPECULAR Y SOMBREADO GEOLÓGICO
    // ========================================================================
    vec3 lightDir = normalize(uSunDirection);
    float NdotL = max(dot(fragNormal, lightDir), 0.0);

    // Contraste más rico: sombras más profundas y luz directa nítida
    float ambient = (uBiomeType == 1) ? 0.48 : 0.42;
    float diffuse = NdotL * ((uBiomeType == 1) ? 0.62 : 0.68);

    // Brillo especular sutil en rocas y estratos minerales (Blinn-Phong)
    vec3 viewDir = normalize(uCameraPos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(fragNormal, halfDir), 0.0);
    float spec = pow(NdotH, 22.0) * (0.24 * (1.0 - slope * 0.3));

    vec3 lighting = vec3(ambient + diffuse) + vec3(spec);
    vec3 finalRGB = terrainDiffuse.rgb * lighting;

    // ========================================================================
    // NIEBLA ATMOSFÉRICA DE HORIZONTE // FADE-OUT SUAVE CON DISPERSIÓN SOLAR
    // ========================================================================
    float dist = length(fragPosition - uCameraPos);
    float rawFog = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    float fogFactor = smoothstep(0.0, 1.0, rawFog);

    // Resplandor cálido de bruma hacia el cuadrante solar (Mie forward scattering)
    vec3 camToFrag = normalize(fragPosition - uCameraPos);
    float sunScatter = max(dot(camToFrag, lightDir), 0.0);
    vec3 sunHaze = mix(uFogColor.rgb, vec3(1.0, 0.95, 0.88), pow(sunScatter, 5.0) * 0.28);

    finalRGB = mix(finalRGB, sunHaze, fogFactor);

    finalColor = vec4(finalRGB, 1.0);
}
