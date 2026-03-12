// FullScreenPassSceneViewExtension.cpp

#include "FullScreenPassSceneViewExtension.h"
#include "FullScreenPassShaders.h"

#include "RenderGraph.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "PixelShaderUtils.h"
#include "CommonRenderResources.h"
#include "SceneView.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderTargetPool.h"

//=============================================================================
// Console Variables
//=============================================================================
static TAutoConsoleVariable<int32> CVarEnabled(
    TEXT("r.ADOF"),
    1,
    TEXT("Controls ADOF plugin\n")
    TEXT(" 0: disabled\n")
    TEXT(" 1: enabled (default)"));

static TAutoConsoleVariable<int32> CVarAutofocusEnable(
    TEXT("r.ADOF.AutofocusEnable"),
    1,
    TEXT("Enable autofocus"));

static TAutoConsoleVariable<float> CVarAutofocusCenterX(
    TEXT("r.ADOF.AutofocusCenterX"),
    0.5f,
    TEXT("Autofocus center X"));

static TAutoConsoleVariable<float> CVarAutofocusCenterY(
    TEXT("r.ADOF.AutofocusCenterY"),
    0.5f,
    TEXT("Autofocus center Y"));

static TAutoConsoleVariable<float> CVarAutofocusRadius(
    TEXT("r.ADOF.AutofocusRadius"),
    0.6f,
    TEXT("Autofocus sample radius"));

static TAutoConsoleVariable<float> CVarAutofocusSpeed(
    TEXT("r.ADOF.AutofocusSpeed"),
    0.1f,
    TEXT("Autofocus adjustment speed"));

static TAutoConsoleVariable<float> CVarManualfocusDepth(
    TEXT("r.ADOF.ManualfocusDepth"),
    0.001f,
    TEXT("Manual focus depth"));

static TAutoConsoleVariable<float> CVarNearBlurCurve(
    TEXT("r.ADOF.NearBlurCurve"),
    6.0f,
    TEXT("Near blur curve"));

static TAutoConsoleVariable<float> CVarFarBlurCurve(
    TEXT("r.ADOF.FarBlurCurve"),
    1.5f,
    TEXT("Far blur curve"));

static TAutoConsoleVariable<float> CVarHyperFocus(
    TEXT("r.ADOF.HyperFocus"),
    0.10f,
    TEXT("Hyperfocal depth distance"));

static TAutoConsoleVariable<float> CVarRenderResolutionMult(
    TEXT("r.ADOF.RenderResolutionMult"),
    0.5f,
    TEXT("Resolution scale of bokeh blur"));

static TAutoConsoleVariable<float> CVarShapeRadius(
    TEXT("r.ADOF.ShapeRadius"),
    20.5f,
    TEXT("Bokeh maximal blur size"));

static TAutoConsoleVariable<float> CVarSmootheningAmount(
    TEXT("r.ADOF.SmootheningAmount"),
    4.0f,
    TEXT("Gaussian blur width"));

static TAutoConsoleVariable<float> CVarBokehIntensity(
    TEXT("r.ADOF.BokehIntensity"),
    0.3f,
    TEXT("Bokeh intensity"));

static TAutoConsoleVariable<int32> CVarBokehMode(
    TEXT("r.ADOF.BokehMode"),
    2,
    TEXT("Bokeh highlight type (0-3)"));

static TAutoConsoleVariable<int32> CVarShapeVertices(
    TEXT("r.ADOF.ShapeVertices"),
    6,
    TEXT("Bokeh shape vertices (3-9)"));

static TAutoConsoleVariable<int32> CVarShapeQuality(
    TEXT("r.ADOF.ShapeQuality"),
    5,
    TEXT("Bokeh shape quality (2-25)"));

static TAutoConsoleVariable<float> CVarShapeCurvatureAmount(
    TEXT("r.ADOF.ShapeCurvatureAmount"),
    1.0f,
    TEXT("Bokeh shape roundness (-1 to 1)"));

static TAutoConsoleVariable<float> CVarShapeRotation(
    TEXT("r.ADOF.ShapeRotation"),
    0.0f,
    TEXT("Bokeh shape rotation (0-360)"));

static TAutoConsoleVariable<float> CVarShapeAnamorphRatio(
    TEXT("r.ADOF.ShapeAnamorphRatio"),
    1.0f,
    TEXT("Bokeh shape aspect ratio"));

static TAutoConsoleVariable<int32> CVarOpticalVignetteEnable(
    TEXT("r.ADOF.OpticalVignetteEnable"),
    0,
    TEXT("Enable optical vignette"));

static TAutoConsoleVariable<float> CVarShapeVignetteCurve(
    TEXT("r.ADOF.ShapeVignetteCurve"),
    0.75f,
    TEXT("Bokeh shape vignette curve"));

static TAutoConsoleVariable<float> CVarShapeVignetteAmount(
    TEXT("r.ADOF.ShapeVignetteAmount"),
    1.0f,
    TEXT("Bokeh shape vignette amount"));

static TAutoConsoleVariable<int32> CVarChromaticAberrationEnable(
    TEXT("r.ADOF.ChromaticAberrationEnable"),
    1,
    TEXT("Enable chromatic aberration"));

static TAutoConsoleVariable<float> CVarShapeChromaAmount(
    TEXT("r.ADOF.ShapeChromaAmount"),
    -0.1f,
    TEXT("Chromatic aberration amount"));

static TAutoConsoleVariable<int32> CVarShapeChromaMode(
    TEXT("r.ADOF.ShapeChromaMode"),
    2,
    TEXT("Chromatic aberration type (0-2)"));

//=============================================================================
// Helper function to get settings from CVars
//=============================================================================
static FADOFSettings GetADOFSettingsFromCVars()
{
    FADOFSettings Settings;
    Settings.bAutofocusEnable = CVarAutofocusEnable.GetValueOnRenderThread() != 0;
    Settings.AutofocusCenter = FVector2f(CVarAutofocusCenterX.GetValueOnRenderThread(), CVarAutofocusCenterY.GetValueOnRenderThread());
    Settings.AutofocusRadius = CVarAutofocusRadius.GetValueOnRenderThread();
    Settings.AutofocusSpeed = CVarAutofocusSpeed.GetValueOnRenderThread();
    Settings.ManualfocusDepth = CVarManualfocusDepth.GetValueOnRenderThread();
    Settings.NearBlurCurve = CVarNearBlurCurve.GetValueOnRenderThread();
    Settings.FarBlurCurve = CVarFarBlurCurve.GetValueOnRenderThread();
    Settings.HyperFocus = CVarHyperFocus.GetValueOnRenderThread();
    Settings.RenderResolutionMult = CVarRenderResolutionMult.GetValueOnRenderThread();
    Settings.ShapeRadius = CVarShapeRadius.GetValueOnRenderThread();
    Settings.SmootheningAmount = CVarSmootheningAmount.GetValueOnRenderThread();
    Settings.BokehIntensity = CVarBokehIntensity.GetValueOnRenderThread();
    Settings.BokehMode = CVarBokehMode.GetValueOnRenderThread();
    Settings.ShapeVertices = FMath::Clamp(CVarShapeVertices.GetValueOnRenderThread(), 3, 9);
    Settings.ShapeQuality = FMath::Clamp(CVarShapeQuality.GetValueOnRenderThread(), 2, 25);
    Settings.ShapeCurvatureAmount = CVarShapeCurvatureAmount.GetValueOnRenderThread();
    Settings.ShapeRotation = CVarShapeRotation.GetValueOnRenderThread();
    Settings.ShapeAnamorphRatio = CVarShapeAnamorphRatio.GetValueOnRenderThread();
    Settings.bOpticalVignetteEnable = CVarOpticalVignetteEnable.GetValueOnRenderThread() != 0;
    Settings.ShapeVignetteCurve = CVarShapeVignetteCurve.GetValueOnRenderThread();
    Settings.ShapeVignetteAmount = CVarShapeVignetteAmount.GetValueOnRenderThread();
    Settings.bChromaticAberrationEnable = CVarChromaticAberrationEnable.GetValueOnRenderThread() != 0;
    Settings.ShapeChromaAmount = CVarShapeChromaAmount.GetValueOnRenderThread();
    Settings.ShapeChromaMode = CVarShapeChromaMode.GetValueOnRenderThread();
    return Settings;
}

//=============================================================================
// Helper function to setup common parameters
//=============================================================================
static void SetupCommonParameters(
    FADOFCommonParameters& Common,
    const FSceneView& View,
    const FADOFSettings& Settings,
    const FIntPoint& BufferSize,
    FRDGTextureSRVRef SceneColorSRV,
    FRDGTextureSRVRef SceneDepthSRV)
{
    Common.View = View.ViewUniformBuffer;

    Common.ADOF_BufferSizeAndInvSize = FVector4f(
        (float)BufferSize.X, (float)BufferSize.Y,
        1.0f / (float)BufferSize.X, 1.0f / (float)BufferSize.Y);
    Common.ADOF_ViewSizeAndInvSize = Common.ADOF_BufferSizeAndInvSize;

    Common.ADOF_SceneColorTexture = SceneColorSRV;
    Common.ADOF_SceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Common.ADOF_DepthTexture = SceneDepthSRV;
    Common.ADOF_DepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();

    Common.bADOF_AutofocusEnable = Settings.bAutofocusEnable ? 1.0f : 0.0f;
    Common.fADOF_AutofocusCenter = Settings.AutofocusCenter;
    Common.fADOF_AutofocusRadius = Settings.AutofocusRadius;
    Common.fADOF_AutofocusSpeed = Settings.AutofocusSpeed;
    Common.fADOF_ManualfocusDepth = Settings.ManualfocusDepth;
    Common.fADOF_NearBlurCurve = Settings.NearBlurCurve;
    Common.fADOF_FarBlurCurve = Settings.FarBlurCurve;
    Common.fADOF_HyperFocus = Settings.HyperFocus;
    Common.fADOF_RenderResolutionMult = Settings.RenderResolutionMult;
    Common.fADOF_ShapeRadius = Settings.ShapeRadius;
    Common.fADOF_SmootheningAmount = Settings.SmootheningAmount;
    Common.fADOF_BokehIntensity = Settings.BokehIntensity;
    Common.iADOF_BokehMode = Settings.BokehMode;
    Common.iADOF_ShapeVertices = Settings.ShapeVertices;
    Common.iADOF_ShapeQuality = Settings.ShapeQuality;
    Common.fADOF_ShapeCurvatureAmount = Settings.ShapeCurvatureAmount;
    Common.fADOF_ShapeRotation = Settings.ShapeRotation;
    Common.fADOF_ShapeAnamorphRatio = Settings.ShapeAnamorphRatio;
    Common.fADOF_ShapeVignetteCurve = Settings.ShapeVignetteCurve;
    Common.fADOF_ShapeVignetteAmount = Settings.ShapeVignetteAmount;
    Common.fADOF_ShapeChromaAmount = Settings.ShapeChromaAmount;
    Common.iADOF_ShapeChromaMode = Settings.ShapeChromaMode;

    Common.ADOF_FocusSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Common.ADOF_FocusPrevSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Common.ADOF_CommonSampler0 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
    Common.ADOF_CommonSampler1 = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
}

//=============================================================================
// Constructor
//=============================================================================
FFullScreenPassSceneViewExtension::FFullScreenPassSceneViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
}

//=============================================================================
// Required interface implementations
//=============================================================================
void FFullScreenPassSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FFullScreenPassSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
}

void FFullScreenPassSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

bool FFullScreenPassSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
    return CVarEnabled.GetValueOnAnyThread() != 0;
}

//=============================================================================
// Subscribe to post processing pass
//=============================================================================
void FFullScreenPassSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
    if (Pass == EPostProcessingPass::Tonemap)
    {
        InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FFullScreenPassSceneViewExtension::AfterTonemap_RenderThread));
    }
}

//=============================================================================
// Helper to create a fallback depth texture
//=============================================================================
static FRDGTextureSRVRef CreateFallbackDepthSRV(FRDGBuilder& GraphBuilder, const FIntPoint& BufferSize)
{
    FRDGTextureDesc DummyDepthDesc = FRDGTextureDesc::Create2D(
        BufferSize,
        PF_R32_FLOAT,
        FClearValueBinding::White,
        TexCreate_RenderTargetable | TexCreate_ShaderResource);
    
    FRDGTextureRef DummyDepth = GraphBuilder.CreateTexture(DummyDepthDesc, TEXT("ADOF.FallbackDepth"));
    AddClearRenderTargetPass(GraphBuilder, DummyDepth, FLinearColor::White);
    
    return GraphBuilder.CreateSRV(FRDGTextureSRVDesc(DummyDepth));
}

//=============================================================================
// Main Render Function
//=============================================================================
// FScreenPassTexture FFullScreenPassSceneViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
// {
//     UE_LOG(LogTemp, Error, TEXT("ADOF: AfterTonemap CALLED"));
    
//     FScreenPassTextureSlice SceneColorSlice = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
    
//     if (!SceneColorSlice.IsValid())
//     {
//         UE_LOG(LogTemp, Error, TEXT("ADOF: Invalid input"));
//         return FScreenPassTexture(SceneColorSlice);
//     }
    
//     RDG_EVENT_SCOPE(GraphBuilder, "ADOF_Test");
    
//     FRDGTextureRef SceneColorTexture = SceneColorSlice.TextureSRV->Desc.Texture;
//     const FIntRect ViewRect = SceneColorSlice.ViewRect;
//     const FIntPoint BufferSize = SceneColorTexture->Desc.Extent;
    
//     // Create output texture
//     FRDGTextureDesc OutputDesc = FRDGTextureDesc::Create2D(
//         BufferSize,
//         PF_FloatRGBA,
//         FClearValueBinding::None,
//         TexCreate_RenderTargetable | TexCreate_ShaderResource);
    
//     FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("ADOF.TestOutput"));
    
//     // Just clear to red
//     AddClearRenderTargetPass(GraphBuilder, OutputTexture, FLinearColor::Red);
    
//     UE_LOG(LogTemp, Error, TEXT("ADOF: Returning red texture"));
    
//     return FScreenPassTexture(OutputTexture, ViewRect);
// }

FScreenPassTexture FFullScreenPassSceneViewExtension::AfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
    FScreenPassTextureSlice SceneColorSlice = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
    
    if (CVarEnabled.GetValueOnRenderThread() == 0 || !SceneColorSlice.IsValid())
    {
        return FScreenPassTexture(SceneColorSlice);
    }

    RDG_EVENT_SCOPE(GraphBuilder, "ADOF");

    FRDGTextureRef SceneColorTexture = SceneColorSlice.TextureSRV->Desc.Texture;
    const FIntRect PrimaryViewRect = SceneColorSlice.ViewRect;
    const FIntPoint BufferSize = SceneColorTexture->Desc.Extent;
    
    FADOFSettings Settings = GetADOFSettingsFromCVars();
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(View.FeatureLevel);

    FRDGTextureSRVRef SceneColorSRV = SceneColorSlice.TextureSRV;
    FRDGTextureSRVRef SceneDepthSRV = CreateFallbackDepthSRV(GraphBuilder, BufferSize);

    FRDGTextureDesc FocusTexDesc = FRDGTextureDesc::Create2D(
        FIntPoint(1, 1),
        PF_R16F,
        FClearValueBinding::Black,
        TexCreate_RenderTargetable | TexCreate_ShaderResource);

    FRDGTextureRef FocusTex = GraphBuilder.CreateTexture(FocusTexDesc, TEXT("ADOF.FocusTex"));
    FRDGTextureRef FocusTexPrev = GraphBuilder.CreateTexture(FocusTexDesc, TEXT("ADOF.FocusTexPrev"));
    
    AddClearRenderTargetPass(GraphBuilder, FocusTexPrev, FLinearColor::Black);

    FRDGTextureDesc CommonTexDesc = FRDGTextureDesc::Create2D(
        BufferSize,
        PF_FloatRGBA,
        FClearValueBinding::Black,
        TexCreate_RenderTargetable | TexCreate_ShaderResource);

    FRDGTextureRef CommonTex0 = GraphBuilder.CreateTexture(CommonTexDesc, TEXT("ADOF.CommonTex0"));
    FRDGTextureRef CommonTex1 = GraphBuilder.CreateTexture(CommonTexDesc, TEXT("ADOF.CommonTex1"));
    FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(CommonTexDesc, TEXT("ADOF.Output"));

    FRDGTextureSRVRef FocusTexSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(FocusTex));
    FRDGTextureSRVRef FocusTexPrevSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(FocusTexPrev));
    FRDGTextureSRVRef CommonTex0SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex0));
    FRDGTextureSRVRef CommonTex1SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex1));

    // =========================================================================
    // Pass 1: Read Focus
    // =========================================================================
    {
        FPS_ReadFocus::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_ReadFocus::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexPrevSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(FocusTex, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_ReadFocus> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF ReadFocus"),
            PixelShader,
            Parameters,
            FIntRect(0, 0, 1, 1));
    }

    FocusTexSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(FocusTex));

    // =========================================================================
    // Pass 2: Copy Focus
    // =========================================================================
    {
        FPS_CopyFocus::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_CopyFocus::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(FocusTexPrev, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_CopyFocus> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF CopyFocus"),
            PixelShader,
            Parameters,
            FIntRect(0, 0, 1, 1));
    }

    FocusTexPrevSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(FocusTexPrev));

    // =========================================================================
    // Pass 3: Circle of Confusion
    // =========================================================================
    {
        FPS_CoC::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_CoC::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(CommonTex0, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_CoC> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF CoC"),
            PixelShader,
            Parameters,
            PrimaryViewRect);
    }

    CommonTex0SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex0));

    // =========================================================================
    // Pass 4: Main DoF Bokeh
    // =========================================================================
    {
        FPS_DoF_Main::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_Main::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(CommonTex1, ERenderTargetLoadAction::ENoAction);

        FPS_DoF_Main::FPermutationDomain PermutationVector;
        PermutationVector.Set<FPS_DoF_Main::FOpticalVignette>(Settings.bOpticalVignetteEnable);

        TShaderMapRef<FPS_DoF_Main> PixelShader(GlobalShaderMap, PermutationVector);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF DoF Main"),
            PixelShader,
            Parameters,
            PrimaryViewRect);
    }

    CommonTex1SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex1));

    // =========================================================================
    // Pass 5: Combine
    // =========================================================================
    {
        FPS_DoF_Combine::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_Combine::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(CommonTex0, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_DoF_Combine> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF Combine"),
            PixelShader,
            Parameters,
            PrimaryViewRect);
    }

    CommonTex0SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex0));

    // =========================================================================
    // Pass 6: Gaussian Blur 1 (Horizontal)
    // =========================================================================
    {
        FPS_DoF_Gauss1::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_Gauss1::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(CommonTex1, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_DoF_Gauss1> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF Gauss1"),
            PixelShader,
            Parameters,
            PrimaryViewRect);
    }

    CommonTex1SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex1));

    // =========================================================================
    // Pass 7: Gaussian Blur 2 (Vertical) + Optional Chromatic Aberration
    // =========================================================================
    if (Settings.bChromaticAberrationEnable)
    {
        {
            FPS_DoF_Gauss2::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_Gauss2::FParameters>();
            SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
            Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
            Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
            Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
            Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
            Parameters->RenderTargets[0] = FRenderTargetBinding(CommonTex0, ERenderTargetLoadAction::ENoAction);

            TShaderMapRef<FPS_DoF_Gauss2> PixelShader(GlobalShaderMap);

            FPixelShaderUtils::AddFullscreenPass(
                GraphBuilder,
                GlobalShaderMap,
                RDG_EVENT_NAME("ADOF Gauss2"),
                PixelShader,
                Parameters,
                PrimaryViewRect);
        }

        CommonTex0SRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(CommonTex0));

        {
            FPS_DoF_ChromaticAberration::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_ChromaticAberration::FParameters>();
            SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
            Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
            Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
            Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
            Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
            Parameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ENoAction);

            FPS_DoF_ChromaticAberration::FPermutationDomain PermutationVector;
            PermutationVector.Set<FPS_DoF_ChromaticAberration::FChromaticAberration>(true);

            TShaderMapRef<FPS_DoF_ChromaticAberration> PixelShader(GlobalShaderMap, PermutationVector);

            FPixelShaderUtils::AddFullscreenPass(
                GraphBuilder,
                GlobalShaderMap,
                RDG_EVENT_NAME("ADOF ChromaticAberration"),
                PixelShader,
                Parameters,
                PrimaryViewRect);
        }
    }
    else
    {
        FPS_DoF_Gauss2::FParameters* Parameters = GraphBuilder.AllocParameters<FPS_DoF_Gauss2::FParameters>();
        SetupCommonParameters(Parameters->Common, View, Settings, BufferSize, SceneColorSRV, SceneDepthSRV);
        Parameters->Common.ADOF_FocusTexture = FocusTexSRV;
        Parameters->Common.ADOF_FocusTexturePrev = FocusTexPrevSRV;
        Parameters->Common.ADOF_CommonTexture0 = CommonTex0SRV;
        Parameters->Common.ADOF_CommonTexture1 = CommonTex1SRV;
        Parameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ENoAction);

        TShaderMapRef<FPS_DoF_Gauss2> PixelShader(GlobalShaderMap);

        FPixelShaderUtils::AddFullscreenPass(
            GraphBuilder,
            GlobalShaderMap,
            RDG_EVENT_NAME("ADOF Gauss2"),
            PixelShader,
            Parameters,
            PrimaryViewRect);
    }

    return FScreenPassTexture(OutputTexture, PrimaryViewRect);
}