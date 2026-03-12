// FullScreenPassShaders.cpp

#include "FullScreenPassShaders.h"

// Path must match: AddShaderSourceDirectoryMapping(TEXT("/Plugin/ADOF"), ...)
IMPLEMENT_GLOBAL_SHADER(FADOFVertexShader, "/Plugin/ADOF/ADOF.usf", "VS_ADOF", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FPS_ReadFocus, "/Plugin/ADOF/ADOF.usf", "PS_ReadFocus", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_CopyFocus, "/Plugin/ADOF/ADOF.usf", "PS_CopyFocus", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_CoC, "/Plugin/ADOF/ADOF.usf", "PS_CoC", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_DoF_Main, "/Plugin/ADOF/ADOF.usf", "PS_DoF_Main", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_DoF_Combine, "/Plugin/ADOF/ADOF.usf", "PS_DoF_Combine", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_DoF_Gauss1, "/Plugin/ADOF/ADOF.usf", "PS_DoF_Gauss1", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_DoF_Gauss2, "/Plugin/ADOF/ADOF.usf", "PS_DoF_Gauss2", SF_Pixel);
IMPLEMENT_GLOBAL_SHADER(FPS_DoF_ChromaticAberration, "/Plugin/ADOF/ADOF.usf", "PS_DoF_ChromaticAberration", SF_Pixel);