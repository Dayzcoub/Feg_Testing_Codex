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
    PID_PATTERN,

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
    PID_GLOW
};

float clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

float mapRange(float value, float outMin, float outMax)
{
    return outMin + clamp01(value) * (outMax - outMin);
}

float unmapRange(float value, float outMin, float outMax)
{
    if (outMax == outMin)
        return 0.0f;
    return clamp01((value - outMin) / (outMax - outMin));
}
}

static CFFGLPluginInfo PluginInfo(
    PluginFactory<PackItLEDPattern>,
    "PKL1",
    "PK LED Pattern",
    2,
    1,
    0,
    100,
    FF_SOURCE,
    "Procedural LED-dot pattern generator with four colours, selectable figures, stepped animation FPS and temporal blur.",
    "PACK.IT test FFGL source"
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

in vec2 uv;
out vec4 fragColor;

const float PI = 3.14159265358979323846;

mat2 rotate2d(float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

float safeAtan(float y, float x)
{
    return atan(y, x);
}

float patternValue(vec2 p, float t)
{
    float r = length(p);
    float a = safeAtan(p.y, p.x);
    float sym = max(2.0, uSymmetry);
    float sector = PI * 2.0 / sym;
    float folded = abs(mod(a + sector * 0.5, sector) - sector * 0.5);
    float type = floor(uPattern + 0.5);
    float value = 0.0;

    if (type < 0.5)
    {
        float spokes = cos(folded * sym * 1.5 + r * 8.0 - t * 1.7);
        float rings = sin(r * 15.0 - t * 2.2);
        value = 0.5 + 0.25 * spokes + 0.25 * rings;
    }
    else if (type < 1.5)
    {
        value = 0.5 + 0.5 * sin(r * 20.0 - t * 3.0 + sin(a * sym) * 0.8);
    }
    else if (type < 2.5)
    {
        float starRadius = r * (1.0 + 0.35 * cos(a * sym));
        value = 0.5 + 0.5 * sin(starRadius * 17.0 - t * 2.4);
    }
    else if (type < 3.5)
    {
        vec2 q = p * 5.0;
        q.x += t * 0.9;
        q.y += sin(t * 0.7) * 0.8;
        float checker = mod(floor(q.x) + floor(q.y), 4.0);
        value = checker / 3.0;
    }
    else if (type < 4.5)
    {
        float w1 = sin(p.x * 12.0 + sin(p.y * 5.0 + t) * 2.0 - t * 2.0);
        float w2 = cos(p.y * 13.0 + sin(p.x * 4.0 - t) * 2.2 + t * 1.5);
        value = 0.5 + 0.25 * w1 + 0.25 * w2;
    }
    else
    {
        float spiral = a * sym * 0.55 + log(max(r, 0.03)) * 8.0 - t * 2.5;
        value = 0.5 + 0.5 * sin(spiral);
    }

    return clamp(value, 0.0, 1.0);
}

vec3 palette4(float value)
{
    float x = fract(value) * 4.0;
    if (x < 1.0)
        return mix(uColor1, uColor2, smoothstep(0.08, 0.92, x));
    if (x < 2.0)
        return mix(uColor2, uColor3, smoothstep(0.08, 0.92, x - 1.0));
    if (x < 3.0)
        return mix(uColor3, uColor4, smoothstep(0.08, 0.92, x - 2.0));
    return mix(uColor4, uColor1, smoothstep(0.08, 0.92, x - 3.0));
}

void main()
{
    float fps = max(1.0, uFps);
    float steppedTime = floor(uTime * fps) / fps;

    float columns = max(8.0, uDensity);
    float rows = max(6.0, columns / max(uAspect, 0.25));
    vec2 grid = vec2(columns, rows);

    vec2 cell = floor(uv * grid);
    vec2 cellCenterUv = (cell + 0.5) / grid;
    vec2 local = fract(uv * grid) - 0.5;

    float radius = mix(0.10, 0.47, clamp(uDotSize, 0.0, 1.0));
    float distanceToDot = length(local);
    float dotMask = 1.0 - smoothstep(radius - 0.035, radius + 0.025, distanceToDot);

    float glowRadius = radius + mix(0.02, 0.26, clamp(uGlow, 0.0, 1.0));
    float glowMask = 1.0 - smoothstep(radius, glowRadius, distanceToDot);
    glowMask *= clamp(uGlow, 0.0, 1.0) * 0.75;

    vec2 p = cellCenterUv * 2.0 - 1.0;
    p.x *= uAspect;
    p /= max(0.15, uScale);

    vec3 accumulated = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 8; ++i)
    {
        float fi = float(i) / 7.0;
        float sampleTime = steppedTime - fi * clamp(uMotionBlur, 0.0, 1.0) * 0.38;
        float animTime = sampleTime * uSpeed;
        vec2 q = rotate2d(sampleTime * uRotationSpeed) * p;
        float v = patternValue(q, animTime);
        float weight = mix(1.0, 0.30, fi);
        accumulated += palette4(v) * weight;
        totalWeight += weight;
    }

    vec3 colour = accumulated / max(totalWeight, 0.001);
    float intensity = max(dotMask, glowMask);
    colour *= dotMask + glowMask;
    fragColor = vec4(colour, intensity);
}
)GLSL";

PackItLEDPattern::PackItLEDPattern()
{
    SetMinInputs(0);
    SetMaxInputs(0);

    SetOptionParamInfo(PID_PATTERN, "Pattern", 6, pattern);
    SetParamElementInfo(PID_PATTERN, 0, "Kaleidoscope", 0.0f);
    SetParamElementInfo(PID_PATTERN, 1, "Rings", 1.0f);
    SetParamElementInfo(PID_PATTERN, 2, "Star", 2.0f);
    SetParamElementInfo(PID_PATTERN, 3, "Checker", 3.0f);
    SetParamElementInfo(PID_PATTERN, 4, "Waves", 4.0f);
    SetParamElementInfo(PID_PATTERN, 5, "Tunnel", 5.0f);

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

    SetOptionParamInfo(PID_SYMMETRY, "Symmetry", 6, symmetry);
    SetParamElementInfo(PID_SYMMETRY, 0, "2", 2.0f);
    SetParamElementInfo(PID_SYMMETRY, 1, "4", 4.0f);
    SetParamElementInfo(PID_SYMMETRY, 2, "6", 6.0f);
    SetParamElementInfo(PID_SYMMETRY, 3, "8", 8.0f);
    SetParamElementInfo(PID_SYMMETRY, 4, "10", 10.0f);
    SetParamElementInfo(PID_SYMMETRY, 5, "12", 12.0f);

    SetOptionParamInfo(PID_FPS, "Animation FPS", 7, animationFps);
    SetParamElementInfo(PID_FPS, 0, "60", 60.0f);
    SetParamElementInfo(PID_FPS, 1, "50", 50.0f);
    SetParamElementInfo(PID_FPS, 2, "30", 30.0f);
    SetParamElementInfo(PID_FPS, 3, "25", 25.0f);
    SetParamElementInfo(PID_FPS, 4, "15", 15.0f);
    SetParamElementInfo(PID_FPS, 5, "10", 10.0f);
    SetParamElementInfo(PID_FPS, 6, "5", 5.0f);

    SetParamInfof(PID_MOTION_BLUR, "Motion Blur", FF_TYPE_STANDARD);
    SetParamInfof(PID_GLOW, "LED Glow", FF_TYPE_STANDARD);
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

    return CFFGLPlugin::InitGL(vp);
}

FFResult PackItLEDPattern::ProcessOpenGL(ProcessOpenGLStruct*)
{
    const float seconds = static_cast<float>(hostTime / 1000.0);
    const float width = static_cast<float>(std::max<FFUInt32>(1, currentViewport.width));
    const float height = static_cast<float>(std::max<FFUInt32>(1, currentViewport.height));
    const float aspect = width / height;

    ScopedShaderBinding shaderBinding(shader.GetGLID());

    glUniform3f(color1Location, color1.r, color1.g, color1.b);
    glUniform3f(color2Location, color2.r, color2.g, color2.b);
    glUniform3f(color3Location, color3.r, color3.g, color3.b);
    glUniform3f(color4Location, color4.r, color4.g, color4.b);
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
    glUniform1f(timeLocation, seconds);
    glUniform1f(aspectLocation, aspect);

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
    case PID_COLOR1_R: color1.r = clamp01(value); break;
    case PID_COLOR1_G: color1.g = clamp01(value); break;
    case PID_COLOR1_B: color1.b = clamp01(value); break;
    case PID_COLOR2_R: color2.r = clamp01(value); break;
    case PID_COLOR2_G: color2.g = clamp01(value); break;
    case PID_COLOR2_B: color2.b = clamp01(value); break;
    case PID_COLOR3_R: color3.r = clamp01(value); break;
    case PID_COLOR3_G: color3.g = clamp01(value); break;
    case PID_COLOR3_B: color3.b = clamp01(value); break;
    case PID_COLOR4_R: color4.r = clamp01(value); break;
    case PID_COLOR4_G: color4.g = clamp01(value); break;
    case PID_COLOR4_B: color4.b = clamp01(value); break;
    case PID_DENSITY: density = mapRange(value, 12.0f, 180.0f); break;
    case PID_DOT_SIZE: dotSize = clamp01(value); break;
    case PID_SPEED: speed = mapRange(value, -2.0f, 2.0f); break;
    case PID_ROTATION: rotationSpeed = mapRange(value, -2.0f, 2.0f); break;
    case PID_SCALE: scale = mapRange(value, 0.45f, 7.0f); break;
    case PID_SYMMETRY: symmetry = value; break;
    case PID_FPS: animationFps = value; break;
    case PID_MOTION_BLUR: motionBlur = clamp01(value); break;
    case PID_GLOW: glow = clamp01(value); break;
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
    case PID_DENSITY: return unmapRange(density, 12.0f, 180.0f);
    case PID_DOT_SIZE: return dotSize;
    case PID_SPEED: return unmapRange(speed, -2.0f, 2.0f);
    case PID_ROTATION: return unmapRange(rotationSpeed, -2.0f, 2.0f);
    case PID_SCALE: return unmapRange(scale, 0.45f, 7.0f);
    case PID_SYMMETRY: return symmetry;
    case PID_FPS: return animationFps;
    case PID_MOTION_BLUR: return motionBlur;
    case PID_GLOW: return glow;
    default: return 0.0f;
    }
}

char* PackItLEDPattern::GetParameterDisplay(unsigned int index)
{
    static char buffer[32];
    std::memset(buffer, 0, sizeof(buffer));

    switch (index)
    {
    case PID_DENSITY:
        std::snprintf(buffer, sizeof(buffer), "%.0f columns", density);
        return buffer;
    case PID_SPEED:
        std::snprintf(buffer, sizeof(buffer), "%.2fx", speed);
        return buffer;
    case PID_ROTATION:
        std::snprintf(buffer, sizeof(buffer), "%.2f rad/s", rotationSpeed);
        return buffer;
    case PID_SCALE:
        std::snprintf(buffer, sizeof(buffer), "%.2fx", scale);
        return buffer;
    case PID_MOTION_BLUR:
        std::snprintf(buffer, sizeof(buffer), "%.0f%%", motionBlur * 100.0f);
        return buffer;
    case PID_GLOW:
        std::snprintf(buffer, sizeof(buffer), "%.0f%%", glow * 100.0f);
        return buffer;
    default:
        return CFFGLPlugin::GetParameterDisplay(index);
    }
}
