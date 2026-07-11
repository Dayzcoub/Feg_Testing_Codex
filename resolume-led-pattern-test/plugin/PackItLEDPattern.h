#pragma once

#include <FFGLSDK.h>

class PackItLEDPattern : public CFFGLPlugin
{
public:
    PackItLEDPattern();

    FFResult InitGL(const FFGLViewportStruct* vp) override;
    FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
    FFResult DeInitGL() override;

    FFResult SetFloatParameter(unsigned int index, float value) override;
    float GetFloatParameter(unsigned int index) override;
    char* GetParameterDisplay(unsigned int index) override;

private:
    struct RGB
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
    };

    // v0.1-compatible state. Parameter IDs for these values must remain stable.
    RGB color1{0.05f, 0.35f, 1.00f};
    RGB color2{0.00f, 1.00f, 0.85f};
    RGB color3{1.00f, 0.10f, 0.65f};
    RGB color4{1.00f, 0.90f, 0.08f};
    float pattern = 0.0f;
    float density = 64.0f;
    float dotSize = 0.78f;
    float speed = 0.35f;
    float rotationSpeed = 0.12f;
    float scale = 2.6f;
    float symmetry = 8.0f;
    float animationFps = 60.0f;
    float motionBlur = 0.22f;
    float glow = 0.42f;

    // v0.2a pattern controls.
    float patternComplexity = 0.50f;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float seed = 1.0f;

    // v0.2a pixel-shape controls.
    float pixelShape = 0.0f;
    float pixelRotation = 0.0f;
    float pixelRotationSpeed = 0.0f;
    float shapeSoftness = 0.12f;
    float shapeRoundness = 0.20f;
    float innerCut = 0.0f;
    float stretchX = 1.0f;
    float stretchY = 1.0f;

    // v0.2a palette controls.
    float colorCount = 4.0f;
    float paletteShift = 0.0f;
    float paletteSpeed = 0.0f;
    float colorBlend = 0.65f;
    float saturation = 1.0f;
    float brightness = 1.0f;

    // v0.2a motion and BPM controls.
    float motionMode = 0.0f;
    float zoomSpeed = 0.0f;
    float driftX = 0.0f;
    float driftY = 0.0f;
    bool frameInterpolation = false;
    float bpmDivision = 1.0f;
    float bpmMultiplier = 1.0f;
    float phaseOffset = 0.0f;
    float beatPulse = 0.25f;
    float beatPulseTarget = 0.0f;

    // v0.2a style/output controls.
    float glowSize = 0.65f;
    float contrast = 1.0f;
    float blurSamples = 8.0f;
    bool invert = false;
    bool mirrorX = false;
    bool mirrorY = false;
    float backgroundMode = 0.0f;
    RGB backgroundColor{0.0f, 0.0f, 0.0f};
    float backgroundAlpha = 0.0f;
    float masterOpacity = 1.0f;

    // State used to turn the host's repeating bar phase into a stable multi-bar phase.
    float lastBarPhase = 0.0f;
    double barCounter = 0.0;
    bool barPhaseInitialised = false;
    bool pauseLatched = false;
    float pausedTime = 0.0f;

    ffglex::FFGLShader shader;
    ffglex::FFGLScreenQuad quad;

    GLint color1Location = -1;
    GLint color2Location = -1;
    GLint color3Location = -1;
    GLint color4Location = -1;
    GLint backgroundColorLocation = -1;

    GLint patternLocation = -1;
    GLint densityLocation = -1;
    GLint dotSizeLocation = -1;
    GLint speedLocation = -1;
    GLint rotationLocation = -1;
    GLint scaleLocation = -1;
    GLint symmetryLocation = -1;
    GLint fpsLocation = -1;
    GLint motionBlurLocation = -1;
    GLint glowLocation = -1;
    GLint timeLocation = -1;
    GLint aspectLocation = -1;

    GLint complexityLocation = -1;
    GLint centerLocation = -1;
    GLint seedLocation = -1;
    GLint pixelShapeLocation = -1;
    GLint pixelRotationLocation = -1;
    GLint pixelRotationSpeedLocation = -1;
    GLint shapeSoftnessLocation = -1;
    GLint shapeRoundnessLocation = -1;
    GLint innerCutLocation = -1;
    GLint stretchLocation = -1;

    GLint colorCountLocation = -1;
    GLint paletteShiftLocation = -1;
    GLint paletteSpeedLocation = -1;
    GLint colorBlendLocation = -1;
    GLint saturationLocation = -1;
    GLint brightnessLocation = -1;

    GLint zoomSpeedLocation = -1;
    GLint driftLocation = -1;
    GLint frameInterpolationLocation = -1;
    GLint beatPhaseLocation = -1;
    GLint beatPulseLocation = -1;
    GLint beatPulseTargetLocation = -1;

    GLint glowSizeLocation = -1;
    GLint contrastLocation = -1;
    GLint blurSamplesLocation = -1;
    GLint invertLocation = -1;
    GLint mirrorLocation = -1;
    GLint backgroundModeLocation = -1;
    GLint backgroundAlphaLocation = -1;
    GLint masterOpacityLocation = -1;
};
