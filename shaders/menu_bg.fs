#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec2 uResolution;
uniform vec2 uMouse;
uniform float uTime;
uniform vec3 uBiomeTint;
uniform float uPlanetAlpha;

// Procedural hash for micro-particulate and dither
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    // Normalized screen coordinates: (0,0) top-left, (1,1) bottom-right
    vec2 uv = gl_FragCoord.xy / uResolution.xy;
    uv.y = 1.0 - uv.y; // Match raylib screen space

    // Aspect ratio corrected coordinates
    float aspect = uResolution.x / uResolution.y;
    vec2 p = uv;
    p.x *= aspect;

    // Normalized mouse [-0.5 .. 0.5] with aspect
    vec2 m = (uMouse / uResolution.xy) - vec2(0.5);
    vec2 mScreen = uMouse / uResolution.xy;

    // -------------------------------------------------------------
    // 1. DEEP DARK FUTURE BLUE BASE GRADIENT
    // -------------------------------------------------------------
    // Deep midnight abyss: deep oceanic dark future blue to void black
    vec3 colAbyssTop = vec3(0.016, 0.038, 0.078);
    vec3 colAbyssMid = vec3(0.008, 0.018, 0.042);
    vec3 colAbyssBot = vec3(0.003, 0.007, 0.016);

    float vGrad = pow(uv.y, 0.92);
    vec3 baseColor = mix(mix(colAbyssTop, colAbyssMid, smoothstep(0.0, 0.65, vGrad)), colAbyssBot, smoothstep(0.4, 1.0, vGrad));

    // Subtle diagonal illumination bias
    float diag = (uv.x * 0.35 + (1.0 - uv.y) * 0.65);
    baseColor += vec3(0.010, 0.024, 0.052) * diag;

    // -------------------------------------------------------------
    // 2. REACTIVE LIQUID LIGHT RIBBONS (ORGANIC AERO SHEEN)
    // -------------------------------------------------------------
    // Dynamic organic wave harmonics influenced by mouse and time
    float t = uTime * 0.35;
    float wave1 = sin(uv.x * 3.8 + t * 1.2 + m.x * 1.6) * 0.09
                + cos(uv.x * 2.1 - t * 0.8 - m.y * 1.2) * 0.05;
    float wave2 = sin(uv.x * 5.2 - t * 0.9 + m.x * 2.0) * 0.06
                + cos(uv.x * 3.1 + t * 1.4) * 0.04;

    // Primary Liquid Ribbon (mid-upper screen)
    float ribbonY1 = 0.36 + wave1;
    float distRibbon1 = abs(uv.y - ribbonY1);
    float glow1 = exp(-distRibbon1 * 9.5);
    float core1 = exp(-distRibbon1 * 32.0);

    // Secondary Liquid Ribbon (lower diagonal cross-wave)
    float ribbonY2 = 0.68 + wave2;
    float distRibbon2 = abs(uv.y - ribbonY2);
    float glow2 = exp(-distRibbon2 * 7.5);
    float core2 = exp(-distRibbon2 * 26.0);

    // Dark Future Blue Palette for Ribbons
    vec3 colCobalt     = mix(vec3(0.04, 0.18, 0.48), uBiomeTint * 0.55, 0.40);
    vec3 colElectric   = mix(vec3(0.08, 0.52, 0.88), uBiomeTint, 0.55);
    vec3 colSpecular   = vec3(0.75, 0.92, 1.0);

    vec3 liquidColor = (colCobalt * glow1 * 0.65 + colElectric * core1 * 0.45)
                     + (colCobalt * glow2 * 0.40 + colElectric * core2 * 0.30);

    // Caustic micro-refraction streaks within ribbons
    float caustic = sin(uv.x * 16.0 + uv.y * 10.0 + t * 1.5)
                  * sin(uv.x * 11.0 - uv.y * 14.0 - t * 1.2);
    caustic = smoothstep(0.35, 0.95, caustic) * (glow1 + glow2 * 0.6);
    liquidColor += colSpecular * caustic * 0.16;

    // -------------------------------------------------------------
    // 3. REACTIVE MOUSE LIGHT AURA & DYNAMIC AMBIENCE
    // -------------------------------------------------------------
    // Soft organic light follower at mouse position
    vec2 mouseDiff = uv - mScreen;
    mouseDiff.x *= aspect;
    float mouseDist = length(mouseDiff);
    float mouseAura = exp(-mouseDist * 3.6);
    vec3 colMouse = mix(vec3(0.06, 0.30, 0.68), uBiomeTint, 0.65) * mouseAura * 0.28;

    // -------------------------------------------------------------
    // 4. FLOATING ORGANIC BOKEH ORBS (FRUTIGER AERO LIQUID ORBS)
    // -------------------------------------------------------------
    vec3 bokehAccum = vec3(0.0);
    // 4 analytical bokeh orbs with smooth motion and mouse parallax
    for (int i = 0; i < 4; i++) {
        float fi = float(i);
        float seed = fi * 1.73;
        // Periodic motion looping smoothly
        float orbTime = uTime * (0.12 + fi * 0.04) + seed;
        vec2 orbPos = vec2(
            fract(0.18 + fi * 0.27 + sin(orbTime * 0.7) * 0.12) * aspect,
            fract(0.85 - orbTime * 0.15) // Floats upward
        );
        // Parallax reaction
        orbPos += m * (0.04 + fi * 0.03);

        vec2 diffOrb = p - orbPos;
        float dOrb = length(diffOrb);
        float radius = 0.14 + fi * 0.04;

        // Smooth bokeh disc with bright perimeter rim
        float disc = smoothstep(radius, radius * 0.15, dOrb);
        float rim  = exp(-abs(dOrb - radius) * 35.0);
        float orbGlow = exp(-dOrb * (6.0 - fi * 0.8));

        vec3 orbCol = mix(vec3(0.08, 0.38, 0.75), uBiomeTint, 0.45);
        bokehAccum += (orbCol * disc * 0.08 + colElectric * rim * 0.12 + orbCol * orbGlow * 0.09);
    }

    // -------------------------------------------------------------
    // 5. CONTINUOUS ANALYTICAL PLANETARY LIMB & INNER ATMOSPHERE
    // -------------------------------------------------------------
    vec3 planetColor = vec3(0.0);
    if (uPlanetAlpha > 0.01) {
        vec2 pCenter = vec2(1.08 * aspect, 1.42) + m * 0.06;
        vec2 diffP = p - pCenter;
        float distP = length(diffP);
        float radius = 1.34;
        float delta = radius - distP; // delta < 0: space | delta = 0: limb | delta > 0: inside planet

        // 1. Continuous Rayleigh atmospheric profile across the limb
        // Outer haze in space (decays outward as delta goes negative)
        float outerAtmo = smoothstep(-0.22, 0.0, delta);
        // Inner twilight haze inside the planet (decays smoothly inward towards the core)
        float innerAtmo = smoothstep(0.38, 0.0, delta);
        float atmoProfile = (delta < 0.0) ? outerAtmo : innerAtmo;

        // Peak atmosphere color: electric cyan at the horizon, rich sapphire deeper in
        vec3 colAtmoHorizon = mix(vec3(0.35, 0.72, 1.0), uBiomeTint, 0.40);
        vec3 colAtmoDeep    = mix(vec3(0.08, 0.28, 0.65), uBiomeTint * 0.8, 0.35);
        vec3 atmoCol = mix(colAtmoDeep, colAtmoHorizon, pow(atmoProfile, 1.5));
        
        planetColor += atmoCol * atmoProfile * 0.85 * uPlanetAlpha;

        // 2. Ultra-fine specular horizon glint (sharp razor-edge at delta = 0.0)
        float glint = exp(-abs(delta) * 140.0);
        planetColor += vec3(0.92, 0.97, 1.0) * glint * 1.30 * uPlanetAlpha;

        // 3. Spherical planet body interior (smooth gradient into Dark Future Blue abyss)
        if (delta > 0.0) {
            float innerDepth = clamp(delta / 0.55, 0.0, 1.0);
            vec3 colPlanetRim  = mix(vec3(0.025, 0.075, 0.160), uBiomeTint * 0.35, 0.30);
            vec3 colPlanetCore = vec3(0.006, 0.015, 0.034);
            vec3 colPlanetBody = mix(colPlanetRim, colPlanetCore, smoothstep(0.0, 1.0, innerDepth));

            // Subtle harmonic oceanic / planetary cloud currents
            float currents = sin(diffP.y * 22.0 + sin(diffP.x * 14.0 + uTime * 0.18) * 1.5) * 0.5 + 0.5;
            colPlanetBody += vec3(0.010, 0.028, 0.065) * currents * (1.0 - innerDepth * 0.70);

            // Smooth organic transition into the planet volume without any hard line
            float bodyAlpha = smoothstep(0.0, 0.08, delta);
            baseColor = mix(baseColor, colPlanetBody, bodyAlpha * 0.88 * uPlanetAlpha);
        }
    }

    // -------------------------------------------------------------
    // 6. MICRO-PARTICULATE TWINKLE & SUBTLE SPARKLE
    // -------------------------------------------------------------
    vec2 st = floor(gl_FragCoord.xy * 0.65);
    float n = hash(st);
    vec3 sparkle = vec3(0.0);
    if (n > 0.9982) {
        float sAlpha = (n - 0.9982) / 0.0018;
        float twinkle = sin(uTime * 2.4 + n * 65.0) * 0.4 + 0.6;
        sparkle = mix(vec3(0.5, 0.75, 1.0), uBiomeTint, 0.35) * sAlpha * twinkle * 0.32;
    }

    // -------------------------------------------------------------
    // 7. COMPOSITION & HIGH-PRECISION DITHERING
    // -------------------------------------------------------------
    vec3 finalRgb = baseColor + liquidColor + colMouse + bokehAccum + planetColor + sparkle;

    // Subtle 8-bit triangular dither to guarantee zero banding
    float dither = (hash(gl_FragCoord.xy) - 0.5) / 255.0;
    finalRgb += dither;

    finalColor = vec4(clamp(finalRgb, 0.0, 1.0), 1.0);
}
