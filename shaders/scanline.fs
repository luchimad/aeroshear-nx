#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 uResolution;      // Screen resolution (e.g. 1920x1080)
uniform float uScanlineAlpha;  // Scanline strength (0.04 for menu, 0.10 for game)
uniform float uTime;           // Animation time
uniform float uDistortion;     // High-G / Afterburner chromatic aberration (0.0 .. 1.0)
uniform float uPhosphorMask;   // RGB aperture grille mask strength (0.0 .. 1.0)
uniform float uBlueGrade;      // BF3 / Cyberpunk futuristic blue color grading (0.0 .. 1.0)

// BF3 Filmic Split-Toning & Color Grading Algorithm
vec3 ApplyBF3ColorGrade(vec3 col, float strength) {
    if (strength <= 0.001) return col;

    // 1. Perceptual Luminance
    float lum = dot(col, vec3(0.2126, 0.7152, 0.0722));

    // 2. High-Contrast S-Curve (Cinematic Tone Curve)
    vec3 sCurve = col * col * (3.0 - 2.0 * col);
    vec3 contrasted = mix(col, sCurve, 0.45);

    // 3. Desaturate midtones to strip gaudy oversaturated greens/yellows
    vec3 desat = mix(contrasted, vec3(lum), 0.32);

    // 4. BF3 Signature Cold Oceanic Split-Toning (Shadows -> Deep Navy, Highlights -> Titanium White)
    vec3 shadowNavy = vec3(0.04, 0.18, 0.38); // Profundo azul abisal/acero
    vec3 midCyan    = vec3(0.60, 0.84, 1.06); // Cyan hielo digital
    vec3 highWhite  = vec3(0.96, 0.99, 1.03); // Blanco titanio nitido

    vec3 tint = mix(shadowNavy, midCyan, smoothstep(0.02, 0.46, lum));
    tint = mix(tint, highWhite, smoothstep(0.46, 0.94, lum));

    vec3 graded = desat * tint;

    // 5. Deep crushed blacks with lifted cool blue shadow floor
    float blackCrush = 0.025;
    graded = max(graded - vec3(blackCrush), vec3(0.0)) / (1.0 - blackCrush);
    graded += vec3(0.010, 0.026, 0.055) * (1.0 - smoothstep(0.0, 0.32, lum));

    return mix(col, graded, strength);
}

void main()
{
    // Radial vector from screen center
    vec2 uvOffset = fragTexCoord - vec2(0.5);
    float distSq = dot(uvOffset, uvOffset);

    // Speed-based subtle chromatic aberration
    vec2 chromaOffset = uvOffset * (0.0025 * uDistortion * distSq);
    float r = texture(texture0, fragTexCoord + chromaOffset).r;
    float g = texture(texture0, fragTexCoord).g;
    float b = texture(texture0, fragTexCoord - chromaOffset).b;
    vec3 baseColor = vec3(r, g, b);

    // Apply BF3 Futuristic Blue Color Grading
    baseColor = ApplyBF3ColorGrade(baseColor, uBlueGrade);

    // Horizontal scanline modulation based on vertical resolution
    float scanline = sin(fragTexCoord.y * uResolution.y * 3.14159265);
    float scanFactor = 1.0 - (0.5 + 0.5 * scanline) * uScanlineAlpha;

    // Faint cathode ray sweep ripple
    float sweep = sin(fragTexCoord.y * 3.0 - uTime * 2.5) * (0.010 * clamp(uScanlineAlpha * 10.0, 0.0, 1.0));
    scanFactor += sweep;

    // Subtle Trinitron / Sony PVM RGB aperture grille mask
    vec3 mask = vec3(1.0);
    if (uPhosphorMask > 0.01) {
        float col = mod(floor(fragTexCoord.x * uResolution.x), 3.0);
        vec3 subpixel = (col < 1.0) ? vec3(1.03, 0.98, 0.98) : ((col < 2.0) ? vec3(0.98, 1.03, 0.98) : vec3(0.98, 0.98, 1.03));
        mask = mix(vec3(1.0), subpixel, uPhosphorMask * 0.40);
    }

    // Gentle CRT curvature glass vignette (darkens slightly towards the outer edges)
    float crtVignette = clamp(1.0 - distSq * (0.12 * clamp(uScanlineAlpha * 8.0 + uBlueGrade * 1.5, 0.0, 1.0)), 0.0, 1.0);

    vec3 finalRgb = baseColor * clamp(scanFactor, 0.0, 1.0) * mask * crtVignette;
    finalColor = vec4(finalRgb, 1.0) * fragColor;
}
