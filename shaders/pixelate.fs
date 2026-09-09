#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 uResolution; // Resolución de pantalla (1280, 720)
uniform vec2 uPixelSize;  // Cuadrícula virtual retro (640, 360)
uniform float uDistortion; // Nivel de aberración en Afterburner y pulsos de Beat (0.0 .. 1.0)

void main()
{
    // Discretización de coordenadas UV al tamaño de píxel virtual arcade
    vec2 pixelCoords = floor(fragTexCoord * uPixelSize) / uPixelSize;
    vec2 halfTexel = 0.5 / uPixelSize;

    // Vector radial desde el centro de la pantalla
    vec2 uvOffset = fragTexCoord - vec2(0.5);
    float dist = length(uvOffset);
    float distSq = dist * dist;

    // Dispersión cromática radial cuadrática (rojo hacia afuera, azul hacia adentro)
    vec2 chromaOffset = uvOffset * (0.016 * uDistortion * distSq);

    float r = texture(texture0, pixelCoords + chromaOffset + halfTexel).r;
    float g = texture(texture0, pixelCoords + halfTexel).g;
    float b = texture(texture0, pixelCoords - chromaOffset + halfTexel).b;

    // Compresión sutil de viñeteado periférico en Afterburner extremo
    float vignette = 1.0 - (0.18 * uDistortion * distSq);
    vec3 color = vec3(r, g, b) * vignette;

    finalColor = vec4(color, 1.0) * fragColor;
}
