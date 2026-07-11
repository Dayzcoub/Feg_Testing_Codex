#include "PackItLEDPattern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace ffglex;

namespace
{
enum ParamID : FFUInt32
{
    // v0.1 IDs: do not reorder or reuse.
    PID_PATTERN = 0,
    PID_COLOR1_R,
    PID_COLOR1_G,
    PID_COLOR1_B,
    PID_COLOR2_R,
    PID_COLOR2_G,
    PID_COLOR2_B,
    PID_COLOR3_R,
    PID_COLOR3_G,
    PID_COLOR3_B,
    PID_COLOR4_R,
    PID_COLOR4_G,
    PID_COLOR4_B,
    PID_DENSITY,
    PID_DOT_SIZE,
    PID_SPEED,
    PID_ROTATION,
    PID_SCALE,
    PID_SYMMETRY,
    PID_FPS,
    PID_MOTION_BLUR,
    PID_GLOW,

    // v0.2a IDs: append-only for future compatibility.
    PID_PATTERN_COMPLEXITY,
    PID_CENTER_X,
    PID_CENTER_Y,
    PID_SEED,
    PID_PIXEL_SHAPE,
    PID_PIXEL_ROTATION,
    PID_PIXEL_ROTATION_SPEED,
    PID_SHAPE_SOFTNESS,
    PID_SHAPE_ROUNDNESS,
    PID_INNER_CUT,
    PID_STRETCH_X,
    PID_STRETCH_Y,
    PID_COLOR_COUNT,
    PID_PALETTE_SHIFT,
    PID_PALETTE_SPEED,
    PID_COLOR_BLEND,
    PID_SATURATION,
    PID_BRIGHTNESS,
    PID_MOTION_MODE,
    PID_ZOOM_SPEED,
    PID_DRIFT_X,
    PID_DRIFT_Y,
    PID_FRAME_INTERPOLATION,
    PID_BPM_DIVISION,
    PID_BPM_MULTIPLIER,
    PID_PHASE_OFFSET,
    PID_BEAT_PULSE,
    PID_BEAT_PULSE_TARGET,
    PID_GLOW_SIZE,
    PID_CONTRAST,
    PID_BLUR_SAMPLES,
    PID_INVERT,
    PID_MIRROR_X,
    PID_MIRROR_Y,
    PID_BACKGROUND_MODE,
    PID_BACKGROUND_R,
    PID_BACKGROUND_G,
    PID_BACKGROUND_B,
    PID_BACKGROUND_ALPHA,
    PID_MASTER_OPACITY
};

constexpr float PI = 3.14159265358979323846f;

float clampUnit(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

float mapRange(float value, float outMin, float outMax)
{
    return outMin + clampUnit(value) * (outMax - outMin);
}

float unmapRange(float value, float outMin, float outMax)
{
    if (outMax == outMin)
        return 0.0f;
    return clampUnit((value - outMin) / (outMax - outMin));
}

float positiveFract(float value)
{
    return value - std::floor(value);
}
}

static CFFGLPluginInfo PluginInfo(
    PluginFactory<PackItLEDPattern>,
    "PKL1",
    "PK LED Pattern",
    2,
    1,
    0,
    200,
    FF_SOURCE,
    "Procedural LED mosaic generator with 12 patterns, 11 pixel shapes, BPM sync, palette animation and temporal blur.",
    "PACK.IT LED Pattern v0.2a"
);

static const char vertexShaderCode[] = R"GLSL(#version 410 core
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec2 vUV;

out vec2 uv;

void main()
{
    gl_Position = vPosition;
    uv = vUV;
}
)GLSL";

static const char fragmentShaderCode[] = R"GLSL(#version 410 core
uniform vec3 uColor1;
uniform vec3 uColor2;
uniform vec3 uColor3;
uniform vec3 uColor4;
uniform vec3 uBackgroundColor;

uniform float uPattern;
uniform float uDensity;
uniform float uDotSize;
uniform float uSpeed;
uniform float uRotationSpeed;
uniform float uScale;
uniform float uSymmetry;
uniform float uFps;
uniform float uMotionBlur;
uniform float uGlow;
uniform float uTime;
uniform float uAspect;

uniform float uComplexity;
uniform vec2 uCenter;
uniform float uSeed;
uniform float uPixelShape;
uniform float uPixelRotation;
uniform float uPixelRotationSpeed;
uniform float uShapeSoftness;
uniform float uShapeRoundness;
uniform float uInnerCut;
uniform vec2 uStretch;

uniform float uColorCount;
uniform float uPaletteShift;
uniform float uPaletteSpeed;
uniform float uColorBlend;
uniform float uSaturation;
uniform float uBrightness;

uniform float uZoomSpeed;
uniform vec2 uDrift;
uniform float uFrameInterpolation;
uniform float uBeatPhase;
uniform float uBeatPulse;
uniform float uBeatPulseTarget;

uniform float uGlowSize;
uniform float uContrast;
uniform float uBlurSamples;
uniform float uInvert;
uniform vec2 uMirror;
uniform float uBackgroundMode;
uniform float uBackgroundAlpha;
uniform float uMasterOpacity;

in vec2 uv;
out vec4 fragColor;

const float PI = 3.14159265358979323846;
const float SQRT3 = 1.7320508075688772;

mat2 rotate2d(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

float hash11(float p)
{
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float sdRoundBox(vec2 p, vec2 halfSize, float radius)
{
    vec2 q = abs(p) - halfSize + radius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

float sdEquilateralTriangle(vec2 p, float radius)
{
    p.x = abs(p.x) - radius;
    p.y = p.y + radius / SQRT3;
    if (p.x + SQRT3 * p.y > 0.0)
        p = vec2(p.x - SQRT3 * p.y, -SQRT3 * p.x - p.y) * 0.5;
    p.x -= clamp(p.x, -2.0 * radius, 0.0);
    return -length(p) * sign(p.y);
}

float sdHexagon(vec2 p, float radius)
{
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * radius, k.z * radius), radius);
    return length(p) * sign(p.y);
}

float sdCross(vec2 p, float radius, float roundness)
{
    float arm = mix(radius * 0.22, radius * 0.48, roundness);
    float horizontal = sdRoundBox(p, vec2(radius, arm), arm * roundness);
    float vertical = sdRoundBox(p, vec2(arm, radius), arm * roundness);
    return min(horizontal, vertical);
}

float radialFlowerDistance(vec2 p, float radius, float lobes, float depth)
{
    float angle = atan(p.y, p.x);
    float boundary = radius * (1.0 - depth + depth * (0.5 + 0.5 * cos(angle * lobes)));
    return length(p) - boundary;
}

float shapeDistance(vec2 p, float shape, float radius, float roundness)
{
    float type = floor(shape + 0.5);

    if (type < 0.5)
        return length(p) - radius;
    if (type < 1.5)
        return max(abs(p.x), abs(p.y)) - radius;
    if (type < 2.5)
        return sdRoundBox(p, vec2(radius), radius * clamp(roundness, 0.0, 0.95));
    if (type < 3.5)
    {
        vec2 q = rotate2d(PI * 0.25) * p;
        return sdRoundBox(q, vec2(radius * 0.78), radius * 0.35 * roundness);
    }
    if (type < 4.5)
        return sdEquilateralTriangle(p, radius * 1.05);
    if (type < 5.5)
        return sdHexagon(p, radius);
    if (type < 6.5)
        return radialFlowerDistance(p, radius, 5.0, 0.58);
    if (type < 7.5)
        return radialFlowerDistance(p, radius, 4.0, 0.42);
    if (type < 8.5)
        return sdCross(p, radius, roundness);
    if (type < 9.5)
    {
        vec2 q = p;
        float halfLength = radius * 0.62;
        q.x -= clamp(q.x, -halfLength, halfLength);
        return length(q) - radius * 0.52;
    }

    return abs(length(p) - radius * 0.68) - radius * 0.18;
}

float patternValue(vec2 p, float t, float beatEnvelope)
{
    float complexity = mix(0.55, 2.35, clamp(uComplexity, 0.0, 1.0));
    float r = length(p);
    float a = atan(p.y, p.x);
    float sym = max(2.0, uSymmetry);
    float sector = PI * 2.0 / sym;
    float folded = abs(mod(a + sector * 0.5, sector) - sector * 0.5);
    float type = floor(uPattern + 0.5);
    float seedA = hash11(uSeed + 1.7) * PI * 2.0;
    float seedB = hash11(uSeed + 8.3) * 4.0 - 2.0;
    float value = 0.0;

    if (type < 0.5)
    {
        float spokes = cos(folded * sym * (1.0 + complexity) + r * 8.0 * complexity - t * 1.7 + seedA);
        float rings = sin(r * 14.0 * complexity - t * 2.2 + seedB);
        value = 0.5 + 0.27 * spokes + 0.23 * rings;
    }
    else if (type < 1.5)
    {
        value = 0.5 + 0.5 * sin(r * 18.0 * complexity - t * 3.0 + sin(a * sym) * 0.65);
    }
    else if (type < 2.5)
    {
        float starRadius = r * (1.0 + 0.34 * cos(a * sym + seedA));
        value = 0.5 + 0.5 * sin(starRadius * 16.0 * complexity - t * 2.4);
    }
    else if (type < 3.5)
    {
        vec2 q = p * (3.0 + 4.0 * complexity);
        q += vec2(t * 0.9, sin(t * 0.7 + seedA) * 0.8);
        float checker = mod(floor(q.x) + floor(q.y), 4.0);
        value = checker / 3.0;
    }
    else if (type < 4.5)
    {
        float w1 = sin(p.x * 11.0 * complexity + sin(p.y * 5.0 + t) * 2.0 - t * 2.0);
        float w2 = cos(p.y * 12.0 * complexity + sin(p.x * 4.0 - t) * 2.2 + t * 1.5);
        value = 0.5 + 0.25 * w1 + 0.25 * w2;
    }
    else if (type < 5.5)
    {
        float spiral = a * sym * 0.55 + log(max(r, 0.025)) * 7.5 * complexity - t * 2.5 + seedA;
        value = 0.5 + 0.5 * sin(spiral);
    }
    else if (type < 6.5)
    {
        vec2 q = rotate2d(PI * 0.25) * p;
        float grid = sin(q.x * 13.0 * complexity - t * 1.7) * sin(q.y * 13.0 * complexity + t * 1.3);
        value = 0.5 + 0.5 * grid;
    }
    else if (type < 7.5)
    {
        float petals = cos(a * 6.0 + sin(r * 4.0 - t) * 0.8);
        float flower = sin(r * 15.0 * complexity - t * 2.0 + petals * 1.8);
        value = 0.5 + 0.5 * flower;
    }
    else if (type < 8.5)
    {
        float plasma = sin((p.x + seedB) * 8.0 * complexity + t);
        plasma += sin((p.y - seedB) * 9.0 * complexity - t * 1.2);
        plasma += sin((p.x + p.y) * 6.0 * complexity + t * 0.7);
        plasma += cos(length(p + vec2(sin(t), cos(t))) * 10.0 * complexity);
        value = 0.5 + plasma * 0.125;
    }
    else if (type < 9.5)
    {
        float orbit1 = sin(r * 20.0 * complexity - t * 2.0);
        float orbit2 = cos(a * sym + t * 1.4 + r * 4.0);
        value = 0.5 + 0.30 * orbit1 + 0.20 * orbit2;
    }
    else if (type < 10.5)
    {
        float pulseRadius = r * (1.0 + beatEnvelope * 0.28);
        value = 0.5 + 0.5 * sin(pulseRadius * 19.0 * complexity - t * 2.2);
    }
    else
    {
        float radial = log(max(r, 0.02)) * 8.0 * complexity - t * 2.8;
        float angular = floor((a + PI) / (2.0 * PI) * sym * 2.0);
        value = 0.5 + 0.35 * sin(radial) + 0.15 * cos(angular * PI + radial * 0.45);
    }

    value = clamp(value, 0.0, 1.0);
    return mix(value, 1.0 - value, step(0.5, uInvert));
}

vec3 paletteColor(float value, float t)
{
    float count = clamp(floor(uColorCount + 0.5), 2.0, 4.0);
    float shifted = fract(value + uPaletteShift + t * uPaletteSpeed * 0.08);
    float x = shifted * count;
    float segment = floor(x);
    float f = fract(x);
    float hardF = step(0.5, f);
    f = mix(hardF, smoothstep(0.0, 1.0, f), clamp(uColorBlend, 0.0, 1.0));

    vec3 a;
    vec3 b;
    if (count < 2.5)
    {
        a = segment < 1.0 ? uColor1 : uColor2;
        b = segment < 1.0 ? uColor2 : uColor1;
    }
    else if (count < 3.5)
    {
        if (segment < 1.0) { a = uColor1; b = uColor2; }
        else if (segment < 2.0) { a = uColor2; b = uColor3; }
        else { a = uColor3; b = uColor1; }
    }
    else
    {
        if (segment < 1.0) { a = uColor1; b = uColor2; }
        else if (segment < 2.0) { a = uColor2; b = uColor3; }
        else if (segment < 3.0) { a = uColor3; b = uColor4; }
        else { a = uColor4; b = uColor1; }
    }

    vec3 colour = mix(a, b, f);
    float luminance = dot(colour, vec3(0.2126, 0.7152, 0.0722));
    colour = mix(vec3(luminance), colour, uSaturation);
    return max(colour * uBrightness, vec3(0.0));
}

float samplePattern(vec2 baseP, float sampleTime, float beatEnvelope)
{
    float pulseScaleTarget = step(0.5, uBeatPulseTarget) * (1.0 - step(1.5, uBeatPulseTarget));
    pulseScaleTarget += step(3.5, uBeatPulseTarget);
    float pulseScale = 1.0 + beatEnvelope * uBeatPulse * 0.32 * pulseScaleTarget;

    float breathingZoom = exp(sin(sampleTime * 0.32) * uZoomSpeed * 0.72);
    vec2 q = baseP / max(0.05, pulseScale * breathingZoom);
    q += uDrift * sampleTime * 0.12;
    q = rotate2d(sampleTime * uRotationSpeed) * q;
    return patternValue(q, sampleTime * uSpeed, beatEnvelope);
}

void main()
{
    vec2 sourceUv = uv;
    if (uMirror.x > 0.5) sourceUv.x = 1.0 - sourceUv.x;
    if (uMirror.y > 0.5) sourceUv.y = 1.0 - sourceUv.y;

    float beatEnvelope = exp(-max(uBeatPhase, 0.0) * 7.5);

    float columns = max(8.0, uDensity);
    float rows = max(6.0, columns / max(uAspect, 0.25));
    vec2 grid = vec2(columns, rows);
    vec2 cell = floor(sourceUv * grid);
    vec2 cellCenterUv = (cell + 0.5) / grid;
    vec2 local = fract(sourceUv * grid) - 0.5;

    float pulsePixelTarget = 1.0 - step(0.5, uBeatPulseTarget);
    pulsePixelTarget += step(3.5, uBeatPulseTarget);
    float radius = mix(0.055, 0.47, clamp(uDotSize, 0.0, 1.0));
    radius *= 1.0 + beatEnvelope * uBeatPulse * 0.35 * pulsePixelTarget;

    vec2 shapeP = local;
    shapeP /= max(uStretch, vec2(0.05));
    shapeP = rotate2d(uPixelRotation + uTime * uPixelRotationSpeed) * shapeP;

    float distanceToShape = shapeDistance(shapeP, uPixelShape, radius, uShapeRoundness);
    float softness = mix(0.002, 0.085, clamp(uShapeSoftness, 0.0, 1.0));
    float shapeMask = 1.0 - smoothstep(-softness, softness, distanceToShape);

    if (uInnerCut > 0.001 && floor(uPixelShape + 0.5) < 9.5)
    {
        float innerScale = mix(0.06, 0.83, clamp(uInnerCut, 0.0, 1.0));
        float innerDistance = shapeDistance(shapeP / innerScale, uPixelShape, radius, uShapeRoundness) * innerScale;
        float innerMask = 1.0 - smoothstep(-softness, softness, innerDistance);
        shapeMask *= 1.0 - innerMask;
    }

    float pulseGlowTarget = step(2.5, uBeatPulseTarget) * (1.0 - step(3.5, uBeatPulseTarget));
    pulseGlowTarget += step(3.5, uBeatPulseTarget);
    float animatedGlow = clamp(uGlow * (1.0 + beatEnvelope * uBeatPulse * pulseGlowTarget), 0.0, 2.0);
    float glowFalloff = mix(0.008, 0.18, clamp(uGlowSize * 0.5, 0.0, 1.0));
    float glowMask = exp(-max(distanceToShape, 0.0) / max(glowFalloff, 0.001)) * animatedGlow;
    glowMask *= (1.0 - shapeMask) * 0.72;

    vec2 p = cellCenterUv * 2.0 - 1.0;
    p.x *= uAspect;
    p -= vec2(uCenter.x * uAspect, uCenter.y);
    p /= max(0.12, uScale);

    vec3 accumulated = vec3(0.0);
    float totalWeight = 0.0;
    int requestedSamples = int(clamp(floor(uBlurSamples + 0.5), 1.0, 12.0));

    for (int i = 0; i < 12; ++i)
    {
        if (i >= requestedSamples)
            break;

        float fi = requestedSamples <= 1 ? 0.0 : float(i) / float(requestedSamples - 1);
        float rawTime = uTime - fi * clamp(uMotionBlur, 0.0, 1.0) * 0.42;
        float sampleTime = rawTime;
        float value;

        if (uFps > 0.5)
        {
            float framePosition = rawTime * uFps;
            float frameIndex = floor(framePosition);
            float t0 = frameIndex / uFps;
            if (uFrameInterpolation > 0.5)
            {
                float t1 = (frameIndex + 1.0) / uFps;
                float blend = fract(framePosition);
                float v0 = samplePattern(p, t0, beatEnvelope);
                float v1 = samplePattern(p, t1, beatEnvelope);
                value = mix(v0, v1, blend);
                sampleTime = mix(t0, t1, blend);
            }
            else
            {
                sampleTime = t0;
                value = samplePattern(p, sampleTime, beatEnvelope);
            }
        }
        else
        {
            value = samplePattern(p, sampleTime, beatEnvelope);
        }

        float weight = mix(1.0, 0.28, fi);
        accumulated += paletteColor(value, sampleTime) * weight;
        totalWeight += weight;
    }

    vec3 colour = accumulated / max(totalWeight, 0.001);
    float mask = clamp(shapeMask + glowMask, 0.0, 1.0);

    float pulseBrightnessTarget = step(1.5, uBeatPulseTarget) * (1.0 - step(2.5, uBeatPulseTarget));
    pulseBrightnessTarget += step(3.5, uBeatPulseTarget);
    colour *= 1.0 + beatEnvelope * uBeatPulse * 0.65 * pulseBrightnessTarget;
    colour = (colour - 0.5) * uContrast + 0.5;
    colour = max(colour, vec3(0.0));

    float foregroundAlpha = mask * clamp(uMasterOpacity, 0.0, 1.0);
    vec3 foreground = colour * mask;

    if (uBackgroundMode < 0.5)
    {
        fragColor = vec4(foreground, foregroundAlpha);
    }
    else
    {
        vec3 background = uBackgroundMode < 1.5 ? vec3(0.0) : uBackgroundColor;
        float backgroundA = uBackgroundMode < 1.5 ? 1.0 : clamp(uBackgroundAlpha, 0.0, 1.0);
        vec3 composed = mix(background, foreground, foregroundAlpha);
        float alpha = max(backgroundA, foregroundAlpha);
        fragColor = vec4(composed, alpha * clamp(uMasterOpacity, 0.0, 1.0));
    }
}
)GLSL";

PackItLEDPattern::PackItLEDPattern()
{
    SetMinInputs(0);
    SetMaxInputs(0);

    SetOptionParamInfo(PID_PATTERN, "Pattern", 12, pattern);
    const char* patternNames[] = {
        "Kaleidoscope", "Rings", "Star Burst", "Checker", "Waves", "Tunnel",
        "Diamond Grid", "Hex Flower", "Plasma", "Orbit", "Pulse Rings", "Pixel Tunnel"
    };
    for (unsigned int i = 0; i < 12; ++i)
        SetParamElementInfo(PID_PATTERN, i, patternNames[i], static_cast<float>(i));

    SetParamInfof(PID_COLOR1_R, "Colour 1 Red", FF_TYPE_RED);
    SetParamInfof(PID_COLOR1_G, "Colour 1 Green", FF_TYPE_GREEN);
    SetParamInfof(PID_COLOR1_B, "Colour 1 Blue", FF_TYPE_BLUE);
    SetParamInfof(PID_COLOR2_R, "Colour 2 Red", FF_TYPE_RED);
    SetParamInfof(PID_COLOR2_G, "Colour 2 Green", FF_TYPE_GREEN);
    SetParamInfof(PID_COLOR2_B, "Colour 2 Blue", FF_TYPE_BLUE);
    SetParamInfof(PID_COLOR3_R, "Colour 3 Red", FF_TYPE_RED);
    SetParamInfof(PID_COLOR3_G, "Colour 3 Green", FF_TYPE_GREEN);
    SetParamInfof(PID_COLOR3_B, "Colour 3 Blue", FF_TYPE_BLUE);
    SetParamInfof(PID_COLOR4_R, "Colour 4 Red", FF_TYPE_RED);
    SetParamInfof(PID_COLOR4_G, "Colour 4 Green", FF_TYPE_GREEN);
    SetParamInfof(PID_COLOR4_B, "Colour 4 Blue", FF_TYPE_BLUE);

    SetParamInfof(PID_DENSITY, "LED Density", FF_TYPE_STANDARD);
    SetParamInfof(PID_DOT_SIZE, "Dot Size", FF_TYPE_STANDARD);
    SetParamInfof(PID_SPEED, "Motion Speed", FF_TYPE_STANDARD);
    SetParamInfof(PID_ROTATION, "Rotation Speed", FF_TYPE_STANDARD);
    SetParamInfof(PID_SCALE, "Pattern Scale", FF_TYPE_STANDARD);

    SetOptionParamInfo(PID_SYMMETRY, "Symmetry", 8, symmetry);
    const float symmetryValues[] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f};
    const char* symmetryNames[] = {"2", "3", "4", "5", "6", "8", "10", "12"};
    for (unsigned int i = 0; i < 8; ++i)
        SetParamElementInfo(PID_SYMMETRY, i, symmetryNames[i], symmetryValues[i]);

    SetOptionParamInfo(PID_FPS, "Animation FPS", 11, animationFps);
    const float fpsValues[] = {0.0f, 60.0f, 50.0f, 30.0f, 25.0f, 24.0f, 15.0f, 12.0f, 10.0f, 8.0f, 5.0f};
    const char* fpsNames[] = {"Smooth", "60", "50", "30", "25", "24", "15", "12", "10", "8", "5"};
    for (unsigned int i = 0; i < 11; ++i)
        SetParamElementInfo(PID_FPS, i, fpsNames[i], fpsValues[i]);

    SetParamInfof(PID_MOTION_BLUR, "Motion Blur", FF_TYPE_STANDARD);
    SetParamInfof(PID_GLOW, "LED Glow", FF_TYPE_STANDARD);

    SetParamInfof(PID_PATTERN_COMPLEXITY, "PatternComplex", FF_TYPE_STANDARD);
    SetParamInfof(PID_CENTER_X, "Center X", FF_TYPE_STANDARD);
    SetParamInfof(PID_CENTER_Y, "Center Y", FF_TYPE_STANDARD);
    SetParamInfof(PID_SEED, "Seed", FF_TYPE_STANDARD);

    SetOptionParamInfo(PID_PIXEL_SHAPE, "Pixel Shape", 11, pixelShape);
    const char* shapeNames[] = {
        "Circle", "Square", "Rounded Square", "Diamond", "Triangle", "Hexagon",
        "Star", "Clover", "Cross", "Capsule", "Ring"
    };
    for (unsigned int i = 0; i < 11; ++i)
        SetParamElementInfo(PID_PIXEL_SHAPE, i, shapeNames[i], static_cast<float>(i));

    SetParamInfof(PID_PIXEL_ROTATION, "Pixel Rotation", FF_TYPE_STANDARD);
    SetParamInfof(PID_PIXEL_ROTATION_SPEED, "Pixel Rot Speed", FF_TYPE_STANDARD);
    SetParamInfof(PID_SHAPE_SOFTNESS, "Shape Softness", FF_TYPE_STANDARD);
    SetParamInfof(PID_SHAPE_ROUNDNESS, "Shape Roundness", FF_TYPE_STANDARD);
    SetParamInfof(PID_INNER_CUT, "Inner Cut", FF_TYPE_STANDARD);
    SetParamInfof(PID_STRETCH_X, "Stretch X", FF_TYPE_STANDARD);
    SetParamInfof(PID_STRETCH_Y, "Stretch Y", FF_TYPE_STANDARD);

    SetOptionParamInfo(PID_COLOR_COUNT, "Colour Count", 3, colorCount);
    SetParamElementInfo(PID_COLOR_COUNT, 0, "2 Colours", 2.0f);
    SetParamElementInfo(PID_COLOR_COUNT, 1, "3 Colours", 3.0f);
    SetParamElementInfo(PID_COLOR_COUNT, 2, "4 Colours", 4.0f);
    SetParamInfof(PID_PALETTE_SHIFT, "Palette Shift", FF_TYPE_STANDARD);
    SetParamInfof(PID_PALETTE_SPEED, "Palette Speed", FF_TYPE_STANDARD);
    SetParamInfof(PID_COLOR_BLEND, "Colour Blend", FF_TYPE_STANDARD);
    SetParamInfof(PID_SATURATION, "Saturation", FF_TYPE_STANDARD);
    SetParamInfof(PID_BRIGHTNESS, "Brightness", FF_TYPE_STANDARD);

    SetOptionParamInfo(PID_MOTION_MODE, "Motion Mode", 3, motionMode);
    SetParamElementInfo(PID_MOTION_MODE, 0, "Free", 0.0f);
    SetParamElementInfo(PID_MOTION_MODE, 1, "BPM", 1.0f);
    SetParamElementInfo(PID_MOTION_MODE, 2, "Paused", 2.0f);
    SetParamInfof(PID_ZOOM_SPEED, "Zoom Speed", FF_TYPE_STANDARD);
    SetParamInfof(PID_DRIFT_X, "Drift X", FF_TYPE_STANDARD);
    SetParamInfof(PID_DRIFT_Y, "Drift Y", FF_TYPE_STANDARD);
    SetParamInfo(PID_FRAME_INTERPOLATION, "Frame Interp", FF_TYPE_BOOLEAN, frameInterpolation);

    SetOptionParamInfo(PID_BPM_DIVISION, "BPM Division", 8, bpmDivision);
    const float divisionValues[] = {0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};
    const char* divisionNames[] = {"1/16", "1/8", "1/4", "1/2", "1", "2", "4", "8"};
    for (unsigned int i = 0; i < 8; ++i)
        SetParamElementInfo(PID_BPM_DIVISION, i, divisionNames[i], divisionValues[i]);
    SetParamInfof(PID_BPM_MULTIPLIER, "BPM Multiplier", FF_TYPE_STANDARD);
    SetParamInfof(PID_PHASE_OFFSET, "Phase Offset", FF_TYPE_STANDARD);
    SetParamInfof(PID_BEAT_PULSE, "Beat Pulse", FF_TYPE_STANDARD);
    SetOptionParamInfo(PID_BEAT_PULSE_TARGET, "Pulse Target", 5, beatPulseTarget);
    SetParamElementInfo(PID_BEAT_PULSE_TARGET, 0, "Pixel Size", 0.0f);
    SetParamElementInfo(PID_BEAT_PULSE_TARGET, 1, "Pattern Scale", 1.0f);
    SetParamElementInfo(PID_BEAT_PULSE_TARGET, 2, "Brightness", 2.0f);
    SetParamElementInfo(PID_BEAT_PULSE_TARGET, 3, "Glow", 3.0f);
    SetParamElementInfo(PID_BEAT_PULSE_TARGET, 4, "All", 4.0f);

    SetParamInfof(PID_GLOW_SIZE, "Glow Size", FF_TYPE_STANDARD);
    SetParamInfof(PID_CONTRAST, "Contrast", FF_TYPE_STANDARD);
    SetOptionParamInfo(PID_BLUR_SAMPLES, "Blur Samples", 3, blurSamples);
    SetParamElementInfo(PID_BLUR_SAMPLES, 0, "4", 4.0f);
    SetParamElementInfo(PID_BLUR_SAMPLES, 1, "8", 8.0f);
    SetParamElementInfo(PID_BLUR_SAMPLES, 2, "12", 12.0f);
    SetParamInfo(PID_INVERT, "Invert", FF_TYPE_BOOLEAN, invert);
    SetParamInfo(PID_MIRROR_X, "Mirror X", FF_TYPE_BOOLEAN, mirrorX);
    SetParamInfo(PID_MIRROR_Y, "Mirror Y", FF_TYPE_BOOLEAN, mirrorY);

    SetOptionParamInfo(PID_BACKGROUND_MODE, "Background Mode", 3, backgroundMode);
    SetParamElementInfo(PID_BACKGROUND_MODE, 0, "Transparent", 0.0f);
    SetParamElementInfo(PID_BACKGROUND_MODE, 1, "Black", 1.0f);
    SetParamElementInfo(PID_BACKGROUND_MODE, 2, "Custom Colour", 2.0f);
    SetParamInfof(PID_BACKGROUND_R, "Background Red", FF_TYPE_RED);
    SetParamInfof(PID_BACKGROUND_G, "Background Green", FF_TYPE_GREEN);
    SetParamInfof(PID_BACKGROUND_B, "Background Blue", FF_TYPE_BLUE);
    SetParamInfof(PID_BACKGROUND_ALPHA, "Background Alpha", FF_TYPE_STANDARD);
    SetParamInfof(PID_MASTER_OPACITY, "Master Opacity", FF_TYPE_STANDARD);

    const unsigned int colourParams[] = {
        PID_COLOR1_R, PID_COLOR1_G, PID_COLOR1_B, PID_COLOR2_R, PID_COLOR2_G, PID_COLOR2_B,
        PID_COLOR3_R, PID_COLOR3_G, PID_COLOR3_B, PID_COLOR4_R, PID_COLOR4_G, PID_COLOR4_B,
        PID_COLOR_COUNT, PID_PALETTE_SHIFT, PID_PALETTE_SPEED, PID_COLOR_BLEND, PID_SATURATION, PID_BRIGHTNESS
    };
    for (unsigned int id : colourParams) SetParamGroup(id, "Colours");

    const unsigned int patternParams[] = {
        PID_PATTERN, PID_SCALE, PID_SYMMETRY, PID_PATTERN_COMPLEXITY, PID_CENTER_X, PID_CENTER_Y, PID_SEED
    };
    for (unsigned int id : patternParams) SetParamGroup(id, "Pattern");

    const unsigned int pixelParams[] = {
        PID_DENSITY, PID_DOT_SIZE, PID_PIXEL_SHAPE, PID_PIXEL_ROTATION, PID_PIXEL_ROTATION_SPEED,
        PID_SHAPE_SOFTNESS, PID_SHAPE_ROUNDNESS, PID_INNER_CUT, PID_STRETCH_X, PID_STRETCH_Y
    };
    for (unsigned int id : pixelParams) SetParamGroup(id, "Pixel Shape");

    const unsigned int motionParams[] = {
        PID_SPEED, PID_ROTATION, PID_FPS, PID_MOTION_MODE, PID_ZOOM_SPEED, PID_DRIFT_X, PID_DRIFT_Y,
        PID_FRAME_INTERPOLATION
    };
    for (unsigned int id : motionParams) SetParamGroup(id, "Motion");

    const unsigned int bpmParams[] = {
        PID_BPM_DIVISION, PID_BPM_MULTIPLIER, PID_PHASE_OFFSET, PID_BEAT_PULSE, PID_BEAT_PULSE_TARGET
    };
    for (unsigned int id : bpmParams) SetParamGroup(id, "BPM Sync");

    const unsigned int styleParams[] = {
        PID_MOTION_BLUR, PID_GLOW, PID_GLOW_SIZE, PID_CONTRAST, PID_BLUR_SAMPLES,
        PID_INVERT, PID_MIRROR_X, PID_MIRROR_Y
    };
    for (unsigned int id : styleParams) SetParamGroup(id, "Style");

    const unsigned int outputParams[] = {
        PID_BACKGROUND_MODE, PID_BACKGROUND_R, PID_BACKGROUND_G, PID_BACKGROUND_B,
        PID_BACKGROUND_ALPHA, PID_MASTER_OPACITY
    };
    for (unsigned int id : outputParams) SetParamGroup(id, "Output");

    SetParamDisplayName(PID_DENSITY, "Pixel Density", false);
    SetParamDisplayName(PID_DOT_SIZE, "Pixel Size", false);
    SetParamDisplayName(PID_GLOW, "Glow", false);
    SetParamDisplayName(PID_PATTERN_COMPLEXITY, "Pattern Complexity", false);
    SetParamDisplayName(PID_PIXEL_ROTATION_SPEED, "Pixel Rotation Speed", false);
    SetParamDisplayName(PID_FRAME_INTERPOLATION, "Frame Interpolation", false);
    SetParamDisplayName(PID_BEAT_PULSE_TARGET, "Beat Pulse Target", false);
}

FFResult PackItLEDPattern::InitGL(const FFGLViewportStruct* vp)
{
    if (!shader.Compile(vertexShaderCode, fragmentShaderCode))
    {
        DeInitGL();
        return FF_FAIL;
    }
    if (!quad.Initialise())
    {
        DeInitGL();
        return FF_FAIL;
    }

    ScopedShaderBinding shaderBinding(shader.GetGLID());
    color1Location = shader.FindUniform("uColor1");
    color2Location = shader.FindUniform("uColor2");
    color3Location = shader.FindUniform("uColor3");
    color4Location = shader.FindUniform("uColor4");
    backgroundColorLocation = shader.FindUniform("uBackgroundColor");

    patternLocation = shader.FindUniform("uPattern");
    densityLocation = shader.FindUniform("uDensity");
    dotSizeLocation = shader.FindUniform("uDotSize");
    speedLocation = shader.FindUniform("uSpeed");
    rotationLocation = shader.FindUniform("uRotationSpeed");
    scaleLocation = shader.FindUniform("uScale");
    symmetryLocation = shader.FindUniform("uSymmetry");
    fpsLocation = shader.FindUniform("uFps");
    motionBlurLocation = shader.FindUniform("uMotionBlur");
    glowLocation = shader.FindUniform("uGlow");
    timeLocation = shader.FindUniform("uTime");
    aspectLocation = shader.FindUniform("uAspect");

    complexityLocation = shader.FindUniform("uComplexity");
    centerLocation = shader.FindUniform("uCenter");
    seedLocation = shader.FindUniform("uSeed");
    pixelShapeLocation = shader.FindUniform("uPixelShape");
    pixelRotationLocation = shader.FindUniform("uPixelRotation");
    pixelRotationSpeedLocation = shader.FindUniform("uPixelRotationSpeed");
    shapeSoftnessLocation = shader.FindUniform("uShapeSoftness");
    shapeRoundnessLocation = shader.FindUniform("uShapeRoundness");
    innerCutLocation = shader.FindUniform("uInnerCut");
    stretchLocation = shader.FindUniform("uStretch");

    colorCountLocation = shader.FindUniform("uColorCount");
    paletteShiftLocation = shader.FindUniform("uPaletteShift");
    paletteSpeedLocation = shader.FindUniform("uPaletteSpeed");
    colorBlendLocation = shader.FindUniform("uColorBlend");
    saturationLocation = shader.FindUniform("uSaturation");
    brightnessLocation = shader.FindUniform("uBrightness");

    zoomSpeedLocation = shader.FindUniform("uZoomSpeed");
    driftLocation = shader.FindUniform("uDrift");
    frameInterpolationLocation = shader.FindUniform("uFrameInterpolation");
    beatPhaseLocation = shader.FindUniform("uBeatPhase");
    beatPulseLocation = shader.FindUniform("uBeatPulse");
    beatPulseTargetLocation = shader.FindUniform("uBeatPulseTarget");

    glowSizeLocation = shader.FindUniform("uGlowSize");
    contrastLocation = shader.FindUniform("uContrast");
    blurSamplesLocation = shader.FindUniform("uBlurSamples");
    invertLocation = shader.FindUniform("uInvert");
    mirrorLocation = shader.FindUniform("uMirror");
    backgroundModeLocation = shader.FindUniform("uBackgroundMode");
    backgroundAlphaLocation = shader.FindUniform("uBackgroundAlpha");
    masterOpacityLocation = shader.FindUniform("uMasterOpacity");

    return CFFGLPlugin::InitGL(vp);
}

FFResult PackItLEDPattern::ProcessOpenGL(ProcessOpenGLStruct*)
{
    const float hostSeconds = static_cast<float>(hostTime / 1000.0);
    const float width = static_cast<float>(std::max<FFUInt32>(1, currentViewport.width));
    const float height = static_cast<float>(std::max<FFUInt32>(1, currentViewport.height));
    const float aspect = width / height;

    if (!barPhaseInitialised)
    {
        lastBarPhase = barPhase;
        barPhaseInitialised = true;
    }
    else if (barPhase + 0.001f < lastBarPhase)
    {
        if (lastBarPhase > 0.75f && barPhase < 0.25f)
            barCounter += 1.0;
        else
            barCounter = 0.0;
    }
    lastBarPhase = barPhase;

    const float safeDivision = std::max(0.0625f, bpmDivision);
    const float totalBars = static_cast<float>(barCounter) + barPhase;
    const float bpmTime = totalBars * (2.0f * PI / safeDivision) * bpmMultiplier;
    float effectiveTime = motionMode < 0.5f ? hostSeconds : bpmTime;

    if (motionMode >= 1.5f)
    {
        if (!pauseLatched)
        {
            pausedTime = hostSeconds;
            pauseLatched = true;
        }
        effectiveTime = pausedTime;
    }
    else
    {
        pauseLatched = false;
    }

    const float beatPhase = positiveFract(barPhase * 4.0f + phaseOffset);

    ScopedShaderBinding shaderBinding(shader.GetGLID());
    glUniform3f(color1Location, color1.r, color1.g, color1.b);
    glUniform3f(color2Location, color2.r, color2.g, color2.b);
    glUniform3f(color3Location, color3.r, color3.g, color3.b);
    glUniform3f(color4Location, color4.r, color4.g, color4.b);
    glUniform3f(backgroundColorLocation, backgroundColor.r, backgroundColor.g, backgroundColor.b);

    glUniform1f(patternLocation, pattern);
    glUniform1f(densityLocation, density);
    glUniform1f(dotSizeLocation, dotSize);
    glUniform1f(speedLocation, speed);
    glUniform1f(rotationLocation, rotationSpeed);
    glUniform1f(scaleLocation, scale);
    glUniform1f(symmetryLocation, symmetry);
    glUniform1f(fpsLocation, animationFps);
    glUniform1f(motionBlurLocation, motionBlur);
    glUniform1f(glowLocation, glow);
    glUniform1f(timeLocation, effectiveTime);
    glUniform1f(aspectLocation, aspect);

    glUniform1f(complexityLocation, patternComplexity);
    glUniform2f(centerLocation, centerX, centerY);
    glUniform1f(seedLocation, seed);
    glUniform1f(pixelShapeLocation, pixelShape);
    glUniform1f(pixelRotationLocation, pixelRotation);
    glUniform1f(pixelRotationSpeedLocation, pixelRotationSpeed);
    glUniform1f(shapeSoftnessLocation, shapeSoftness);
    glUniform1f(shapeRoundnessLocation, shapeRoundness);
    glUniform1f(innerCutLocation, innerCut);
    glUniform2f(stretchLocation, stretchX, stretchY);

    glUniform1f(colorCountLocation, colorCount);
    glUniform1f(paletteShiftLocation, paletteShift);
    glUniform1f(paletteSpeedLocation, paletteSpeed);
    glUniform1f(colorBlendLocation, colorBlend);
    glUniform1f(saturationLocation, saturation);
    glUniform1f(brightnessLocation, brightness);

    glUniform1f(zoomSpeedLocation, zoomSpeed);
    glUniform2f(driftLocation, driftX, driftY);
    glUniform1f(frameInterpolationLocation, frameInterpolation ? 1.0f : 0.0f);
    glUniform1f(beatPhaseLocation, beatPhase);
    glUniform1f(beatPulseLocation, beatPulse);
    glUniform1f(beatPulseTargetLocation, beatPulseTarget);

    glUniform1f(glowSizeLocation, glowSize);
    glUniform1f(contrastLocation, contrast);
    glUniform1f(blurSamplesLocation, blurSamples);
    glUniform1f(invertLocation, invert ? 1.0f : 0.0f);
    glUniform2f(mirrorLocation, mirrorX ? 1.0f : 0.0f, mirrorY ? 1.0f : 0.0f);
    glUniform1f(backgroundModeLocation, backgroundMode);
    glUniform1f(backgroundAlphaLocation, backgroundAlpha);
    glUniform1f(masterOpacityLocation, masterOpacity);

    quad.Draw();
    return FF_SUCCESS;
}

FFResult PackItLEDPattern::DeInitGL()
{
    shader.FreeGLResources();
    quad.Release();
    return FF_SUCCESS;
}

FFResult PackItLEDPattern::SetFloatParameter(unsigned int index, float value)
{
    switch (index)
    {
    case PID_PATTERN: pattern = value; break;
    case PID_COLOR1_R: color1.r = clampUnit(value); break;
    case PID_COLOR1_G: color1.g = clampUnit(value); break;
    case PID_COLOR1_B: color1.b = clampUnit(value); break;
    case PID_COLOR2_R: color2.r = clampUnit(value); break;
    case PID_COLOR2_G: color2.g = clampUnit(value); break;
    case PID_COLOR2_B: color2.b = clampUnit(value); break;
    case PID_COLOR3_R: color3.r = clampUnit(value); break;
    case PID_COLOR3_G: color3.g = clampUnit(value); break;
    case PID_COLOR3_B: color3.b = clampUnit(value); break;
    case PID_COLOR4_R: color4.r = clampUnit(value); break;
    case PID_COLOR4_G: color4.g = clampUnit(value); break;
    case PID_COLOR4_B: color4.b = clampUnit(value); break;
    case PID_DENSITY: density = mapRange(value, 8.0f, 240.0f); break;
    case PID_DOT_SIZE: dotSize = clampUnit(value); break;
    case PID_SPEED: speed = mapRange(value, -2.0f, 2.0f); break;
    case PID_ROTATION: rotationSpeed = mapRange(value, -2.0f, 2.0f); break;
    case PID_SCALE: scale = mapRange(value, 0.25f, 8.0f); break;
    case PID_SYMMETRY: symmetry = value; break;
    case PID_FPS: animationFps = value; break;
    case PID_MOTION_BLUR: motionBlur = clampUnit(value); break;
    case PID_GLOW: glow = clampUnit(value); break;

    case PID_PATTERN_COMPLEXITY: patternComplexity = clampUnit(value); break;
    case PID_CENTER_X: centerX = mapRange(value, -1.0f, 1.0f); break;
    case PID_CENTER_Y: centerY = mapRange(value, -1.0f, 1.0f); break;
    case PID_SEED: seed = mapRange(value, 0.0f, 9999.0f); break;
    case PID_PIXEL_SHAPE: pixelShape = value; break;
    case PID_PIXEL_ROTATION: pixelRotation = mapRange(value, -PI, PI); break;
    case PID_PIXEL_ROTATION_SPEED: pixelRotationSpeed = mapRange(value, -2.0f * PI, 2.0f * PI); break;
    case PID_SHAPE_SOFTNESS: shapeSoftness = clampUnit(value); break;
    case PID_SHAPE_ROUNDNESS: shapeRoundness = clampUnit(value); break;
    case PID_INNER_CUT: innerCut = mapRange(value, 0.0f, 0.9f); break;
    case PID_STRETCH_X: stretchX = mapRange(value, 0.25f, 2.0f); break;
    case PID_STRETCH_Y: stretchY = mapRange(value, 0.25f, 2.0f); break;

    case PID_COLOR_COUNT: colorCount = value; break;
    case PID_PALETTE_SHIFT: paletteShift = clampUnit(value); break;
    case PID_PALETTE_SPEED: paletteSpeed = mapRange(value, -2.0f, 2.0f); break;
    case PID_COLOR_BLEND: colorBlend = clampUnit(value); break;
    case PID_SATURATION: saturation = mapRange(value, 0.0f, 2.0f); break;
    case PID_BRIGHTNESS: brightness = mapRange(value, 0.0f, 2.0f); break;

    case PID_MOTION_MODE: motionMode = value; break;
    case PID_ZOOM_SPEED: zoomSpeed = mapRange(value, -2.0f, 2.0f); break;
    case PID_DRIFT_X: driftX = mapRange(value, -2.0f, 2.0f); break;
    case PID_DRIFT_Y: driftY = mapRange(value, -2.0f, 2.0f); break;
    case PID_FRAME_INTERPOLATION: frameInterpolation = value != 0.0f; break;
    case PID_BPM_DIVISION: bpmDivision = value; break;
    case PID_BPM_MULTIPLIER: bpmMultiplier = mapRange(value, 0.25f, 4.0f); break;
    case PID_PHASE_OFFSET: phaseOffset = clampUnit(value); break;
    case PID_BEAT_PULSE: beatPulse = clampUnit(value); break;
    case PID_BEAT_PULSE_TARGET: beatPulseTarget = value; break;

    case PID_GLOW_SIZE: glowSize = mapRange(value, 0.0f, 2.0f); break;
    case PID_CONTRAST: contrast = mapRange(value, 0.0f, 2.0f); break;
    case PID_BLUR_SAMPLES: blurSamples = value; break;
    case PID_INVERT: invert = value != 0.0f; break;
    case PID_MIRROR_X: mirrorX = value != 0.0f; break;
    case PID_MIRROR_Y: mirrorY = value != 0.0f; break;
    case PID_BACKGROUND_MODE: backgroundMode = value; break;
    case PID_BACKGROUND_R: backgroundColor.r = clampUnit(value); break;
    case PID_BACKGROUND_G: backgroundColor.g = clampUnit(value); break;
    case PID_BACKGROUND_B: backgroundColor.b = clampUnit(value); break;
    case PID_BACKGROUND_ALPHA: backgroundAlpha = clampUnit(value); break;
    case PID_MASTER_OPACITY: masterOpacity = clampUnit(value); break;
    default: return FF_FAIL;
    }
    return FF_SUCCESS;
}

float PackItLEDPattern::GetFloatParameter(unsigned int index)
{
    switch (index)
    {
    case PID_PATTERN: return pattern;
    case PID_COLOR1_R: return color1.r;
    case PID_COLOR1_G: return color1.g;
    case PID_COLOR1_B: return color1.b;
    case PID_COLOR2_R: return color2.r;
    case PID_COLOR2_G: return color2.g;
    case PID_COLOR2_B: return color2.b;
    case PID_COLOR3_R: return color3.r;
    case PID_COLOR3_G: return color3.g;
    case PID_COLOR3_B: return color3.b;
    case PID_COLOR4_R: return color4.r;
    case PID_COLOR4_G: return color4.g;
    case PID_COLOR4_B: return color4.b;
    case PID_DENSITY: return unmapRange(density, 8.0f, 240.0f);
    case PID_DOT_SIZE: return dotSize;
    case PID_SPEED: return unmapRange(speed, -2.0f, 2.0f);
    case PID_ROTATION: return unmapRange(rotationSpeed, -2.0f, 2.0f);
    case PID_SCALE: return unmapRange(scale, 0.25f, 8.0f);
    case PID_SYMMETRY: return symmetry;
    case PID_FPS: return animationFps;
    case PID_MOTION_BLUR: return motionBlur;
    case PID_GLOW: return glow;

    case PID_PATTERN_COMPLEXITY: return patternComplexity;
    case PID_CENTER_X: return unmapRange(centerX, -1.0f, 1.0f);
    case PID_CENTER_Y: return unmapRange(centerY, -1.0f, 1.0f);
    case PID_SEED: return unmapRange(seed, 0.0f, 9999.0f);
    case PID_PIXEL_SHAPE: return pixelShape;
    case PID_PIXEL_ROTATION: return unmapRange(pixelRotation, -PI, PI);
    case PID_PIXEL_ROTATION_SPEED: return unmapRange(pixelRotationSpeed, -2.0f * PI, 2.0f * PI);
    case PID_SHAPE_SOFTNESS: return shapeSoftness;
    case PID_SHAPE_ROUNDNESS: return shapeRoundness;
    case PID_INNER_CUT: return unmapRange(innerCut, 0.0f, 0.9f);
    case PID_STRETCH_X: return unmapRange(stretchX, 0.25f, 2.0f);
    case PID_STRETCH_Y: return unmapRange(stretchY, 0.25f, 2.0f);

    case PID_COLOR_COUNT: return colorCount;
    case PID_PALETTE_SHIFT: return paletteShift;
    case PID_PALETTE_SPEED: return unmapRange(paletteSpeed, -2.0f, 2.0f);
    case PID_COLOR_BLEND: return colorBlend;
    case PID_SATURATION: return unmapRange(saturation, 0.0f, 2.0f);
    case PID_BRIGHTNESS: return unmapRange(brightness, 0.0f, 2.0f);

    case PID_MOTION_MODE: return motionMode;
    case PID_ZOOM_SPEED: return unmapRange(zoomSpeed, -2.0f, 2.0f);
    case PID_DRIFT_X: return unmapRange(driftX, -2.0f, 2.0f);
    case PID_DRIFT_Y: return unmapRange(driftY, -2.0f, 2.0f);
    case PID_FRAME_INTERPOLATION: return frameInterpolation ? 1.0f : 0.0f;
    case PID_BPM_DIVISION: return bpmDivision;
    case PID_BPM_MULTIPLIER: return unmapRange(bpmMultiplier, 0.25f, 4.0f);
    case PID_PHASE_OFFSET: return phaseOffset;
    case PID_BEAT_PULSE: return beatPulse;
    case PID_BEAT_PULSE_TARGET: return beatPulseTarget;

    case PID_GLOW_SIZE: return unmapRange(glowSize, 0.0f, 2.0f);
    case PID_CONTRAST: return unmapRange(contrast, 0.0f, 2.0f);
    case PID_BLUR_SAMPLES: return blurSamples;
    case PID_INVERT: return invert ? 1.0f : 0.0f;
    case PID_MIRROR_X: return mirrorX ? 1.0f : 0.0f;
    case PID_MIRROR_Y: return mirrorY ? 1.0f : 0.0f;
    case PID_BACKGROUND_MODE: return backgroundMode;
    case PID_BACKGROUND_R: return backgroundColor.r;
    case PID_BACKGROUND_G: return backgroundColor.g;
    case PID_BACKGROUND_B: return backgroundColor.b;
    case PID_BACKGROUND_ALPHA: return backgroundAlpha;
    case PID_MASTER_OPACITY: return masterOpacity;
    default: return 0.0f;
    }
}

char* PackItLEDPattern::GetParameterDisplay(unsigned int index)
{
    static char buffer[48];
    std::memset(buffer, 0, sizeof(buffer));

    switch (index)
    {
    case PID_DENSITY: std::snprintf(buffer, sizeof(buffer), "%.0f columns", density); break;
    case PID_DOT_SIZE: std::snprintf(buffer, sizeof(buffer), "%.0f%%", dotSize * 100.0f); break;
    case PID_SPEED: std::snprintf(buffer, sizeof(buffer), "%.2fx", speed); break;
    case PID_ROTATION: std::snprintf(buffer, sizeof(buffer), "%.2f turn/s", rotationSpeed); break;
    case PID_SCALE: std::snprintf(buffer, sizeof(buffer), "%.2fx", scale); break;
    case PID_MOTION_BLUR: std::snprintf(buffer, sizeof(buffer), "%.0f%%", motionBlur * 100.0f); break;
    case PID_GLOW: std::snprintf(buffer, sizeof(buffer), "%.0f%%", glow * 100.0f); break;
    case PID_PATTERN_COMPLEXITY: std::snprintf(buffer, sizeof(buffer), "%.0f%%", patternComplexity * 100.0f); break;
    case PID_CENTER_X: std::snprintf(buffer, sizeof(buffer), "%.2f", centerX); break;
    case PID_CENTER_Y: std::snprintf(buffer, sizeof(buffer), "%.2f", centerY); break;
    case PID_SEED: std::snprintf(buffer, sizeof(buffer), "%.0f", seed); break;
    case PID_PIXEL_ROTATION: std::snprintf(buffer, sizeof(buffer), "%.0f deg", pixelRotation * 180.0f / PI); break;
    case PID_PIXEL_ROTATION_SPEED: std::snprintf(buffer, sizeof(buffer), "%.0f deg/s", pixelRotationSpeed * 180.0f / PI); break;
    case PID_SHAPE_SOFTNESS: std::snprintf(buffer, sizeof(buffer), "%.0f%%", shapeSoftness * 100.0f); break;
    case PID_SHAPE_ROUNDNESS: std::snprintf(buffer, sizeof(buffer), "%.0f%%", shapeRoundness * 100.0f); break;
    case PID_INNER_CUT: std::snprintf(buffer, sizeof(buffer), "%.0f%%", innerCut * 100.0f); break;
    case PID_STRETCH_X: std::snprintf(buffer, sizeof(buffer), "%.0f%%", stretchX * 100.0f); break;
    case PID_STRETCH_Y: std::snprintf(buffer, sizeof(buffer), "%.0f%%", stretchY * 100.0f); break;
    case PID_PALETTE_SHIFT: std::snprintf(buffer, sizeof(buffer), "%.0f%%", paletteShift * 100.0f); break;
    case PID_PALETTE_SPEED: std::snprintf(buffer, sizeof(buffer), "%.2fx", paletteSpeed); break;
    case PID_COLOR_BLEND: std::snprintf(buffer, sizeof(buffer), "%.0f%%", colorBlend * 100.0f); break;
    case PID_SATURATION: std::snprintf(buffer, sizeof(buffer), "%.0f%%", saturation * 100.0f); break;
    case PID_BRIGHTNESS: std::snprintf(buffer, sizeof(buffer), "%.0f%%", brightness * 100.0f); break;
    case PID_ZOOM_SPEED: std::snprintf(buffer, sizeof(buffer), "%.2fx", zoomSpeed); break;
    case PID_DRIFT_X: std::snprintf(buffer, sizeof(buffer), "%.2f", driftX); break;
    case PID_DRIFT_Y: std::snprintf(buffer, sizeof(buffer), "%.2f", driftY); break;
    case PID_BPM_MULTIPLIER: std::snprintf(buffer, sizeof(buffer), "%.2fx", bpmMultiplier); break;
    case PID_PHASE_OFFSET: std::snprintf(buffer, sizeof(buffer), "%.0f%%", phaseOffset * 100.0f); break;
    case PID_BEAT_PULSE: std::snprintf(buffer, sizeof(buffer), "%.0f%%", beatPulse * 100.0f); break;
    case PID_GLOW_SIZE: std::snprintf(buffer, sizeof(buffer), "%.0f%%", glowSize * 100.0f); break;
    case PID_CONTRAST: std::snprintf(buffer, sizeof(buffer), "%.0f%%", contrast * 100.0f); break;
    case PID_BACKGROUND_ALPHA: std::snprintf(buffer, sizeof(buffer), "%.0f%%", backgroundAlpha * 100.0f); break;
    case PID_MASTER_OPACITY: std::snprintf(buffer, sizeof(buffer), "%.0f%%", masterOpacity * 100.0f); break;
    default: return CFFGLPlugin::GetParameterDisplay(index);
    }
    return buffer;
}
