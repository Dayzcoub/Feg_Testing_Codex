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

    ffglex::FFGLShader shader;
    ffglex::FFGLScreenQuad quad;

    GLint color1Location = -1;
    GLint color2Location = -1;
    GLint color3Location = -1;
    GLint color4Location = -1;
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
};
