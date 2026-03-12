// FullScreenPassShaders.h

#pragma once

#include "CoreMinimal.h"
#include "RenderGraph.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "GlobalShader.h"
#include "SceneView.h"

#define ADOF_OPTICAL_VIGNETTE_ENABLE 0
#define ADOF_CHROMATIC_ABERRATION_ENABLE 1

//=============================================================================
// Common Vertex Shader
//=============================================================================
class FADOFVertexShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FADOFVertexShader);

    FADOFVertexShader() = default;
    FADOFVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) 
        : FGlobalShader(Initializer) {}

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

//=============================================================================
// Common Parameter Struct - names must match shader exactly
//=============================================================================
BEGIN_SHADER_PARAMETER_STRUCT(FADOFCommonParameters, )
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    
    // Buffer size parameters
    SHADER_PARAMETER(FVector4f, ADOF_BufferSizeAndInvSize)
    SHADER_PARAMETER(FVector4f, ADOF_ViewSizeAndInvSize)

    // Scene color - renamed to avoid engine conflicts
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_SceneColorTexture)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_SceneColorSampler)

    // Depth texture - renamed to avoid engine conflicts
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_DepthTexture)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_DepthSampler)

    // ADOF parameters
    SHADER_PARAMETER(float, bADOF_AutofocusEnable)
    SHADER_PARAMETER(FVector2f, fADOF_AutofocusCenter)
    SHADER_PARAMETER(float, fADOF_AutofocusRadius)
    SHADER_PARAMETER(float, fADOF_AutofocusSpeed)
    SHADER_PARAMETER(float, fADOF_ManualfocusDepth)
    SHADER_PARAMETER(float, fADOF_NearBlurCurve)
    SHADER_PARAMETER(float, fADOF_FarBlurCurve)
    SHADER_PARAMETER(float, fADOF_HyperFocus)

    SHADER_PARAMETER(float, fADOF_RenderResolutionMult)
    SHADER_PARAMETER(float, fADOF_ShapeRadius)
    SHADER_PARAMETER(float, fADOF_SmootheningAmount)

    SHADER_PARAMETER(float, fADOF_BokehIntensity)
    SHADER_PARAMETER(int32, iADOF_BokehMode)
    SHADER_PARAMETER(int32, iADOF_ShapeVertices)
    SHADER_PARAMETER(int32, iADOF_ShapeQuality)
    SHADER_PARAMETER(float, fADOF_ShapeCurvatureAmount)
    SHADER_PARAMETER(float, fADOF_ShapeRotation)
    SHADER_PARAMETER(float, fADOF_ShapeAnamorphRatio)

    SHADER_PARAMETER(float, fADOF_ShapeVignetteCurve)
    SHADER_PARAMETER(float, fADOF_ShapeVignetteAmount)

    SHADER_PARAMETER(float, fADOF_ShapeChromaAmount)
    SHADER_PARAMETER(int32, iADOF_ShapeChromaMode)

    // ADOF textures - renamed to avoid engine conflicts
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_FocusTexture)
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_FocusTexturePrev)
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_CommonTexture0)
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, ADOF_CommonTexture1)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_FocusSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_FocusPrevSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_CommonSampler0)
    SHADER_PARAMETER_SAMPLER(SamplerState, ADOF_CommonSampler1)
END_SHADER_PARAMETER_STRUCT()

//=============================================================================
// Pixel Shader Classes
//=============================================================================
class FPS_ReadFocus : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_ReadFocus);
    SHADER_USE_PARAMETER_STRUCT(FPS_ReadFocus, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_CopyFocus : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_CopyFocus);
    SHADER_USE_PARAMETER_STRUCT(FPS_CopyFocus, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_CoC : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_CoC);
    SHADER_USE_PARAMETER_STRUCT(FPS_CoC, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_DoF_Main : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_DoF_Main);
    SHADER_USE_PARAMETER_STRUCT(FPS_DoF_Main, FGlobalShader);

    class FOpticalVignette : SHADER_PERMUTATION_BOOL("ADOF_OPTICAL_VIGNETTE_ENABLE");
    using FPermutationDomain = TShaderPermutationDomain<FOpticalVignette>;

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_DoF_Combine : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_DoF_Combine);
    SHADER_USE_PARAMETER_STRUCT(FPS_DoF_Combine, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_DoF_Gauss1 : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_DoF_Gauss1);
    SHADER_USE_PARAMETER_STRUCT(FPS_DoF_Gauss1, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_DoF_Gauss2 : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_DoF_Gauss2);
    SHADER_USE_PARAMETER_STRUCT(FPS_DoF_Gauss2, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

class FPS_DoF_ChromaticAberration : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FPS_DoF_ChromaticAberration);
    SHADER_USE_PARAMETER_STRUCT(FPS_DoF_ChromaticAberration, FGlobalShader);

    class FChromaticAberration : SHADER_PERMUTATION_BOOL("ADOF_CHROMATIC_ABERRATION_ENABLE");
    using FPermutationDomain = TShaderPermutationDomain<FChromaticAberration>;

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FADOFCommonParameters, Common)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

//=============================================================================
// Settings Structure
//=============================================================================
struct FADOFSettings
{
    bool bAutofocusEnable = true;
    FVector2f AutofocusCenter = FVector2f(0.5f, 0.5f);
    float AutofocusRadius = 0.6f;
    float AutofocusSpeed = 0.1f;
    float ManualfocusDepth = 0.001f;
    float NearBlurCurve = 6.0f;
    float FarBlurCurve = 1.5f;
    float HyperFocus = 0.10f;
    float RenderResolutionMult = 0.5f;
    float ShapeRadius = 20.5f;
    float SmootheningAmount = 4.0f;
    float BokehIntensity = 0.3f;
    int32 BokehMode = 2;
    int32 ShapeVertices = 6;
    int32 ShapeQuality = 5;
    float ShapeCurvatureAmount = 1.0f;
    float ShapeRotation = 0.0f;
    float ShapeAnamorphRatio = 1.0f;
    bool bOpticalVignetteEnable = false;
    float ShapeVignetteCurve = 0.75f;
    float ShapeVignetteAmount = 1.0f;
    bool bChromaticAberrationEnable = true;
    float ShapeChromaAmount = -0.1f;
    int32 ShapeChromaMode = 2;
};