#include <string.h>
#include <stdlib.h>
#include "gbi.h"


/*********************************
              Globals
*********************************/

// List of valid display list commands as of F3DEX2
const DListCommand commands_f3dex2[] = {
    {DPFillRectangle, "DPFillRectangle", 4, 1},
    {DPScisFillRectangle, "DPScisFillRectangle", 4, 1},
    {DPFullSync, "DPFullSync", 0, 1},
    {DPLoadSync, "DPLoadSync", 0, 1},
    {DPTileSync, "DPTileSync", 0, 1},
    {DPPipeSync, "DPPipeSync", 0, 1},
    {DPLoadTLUT_pal16, "DPLoadTLUT_pal16", 2, 6},
    {DPLoadTLUT_pal256, "DPLoadTLUT_pal256", 1, 6},
    {DPLoadTextureBlock, "DPLoadTextureBlock", 12, 7},
    {DPLoadTextureBlock_4b, "DPLoadTextureBlock_4b", 11, 7},
    {DPLoadTextureTile, "DPLoadTextureTile", 16, 7},
    {DPLoadTextureTile_4b, "DPLoadTextureTile_4b", 15, 7},
    {DPLoadBlock, "DPLoadBlock", 5, 1},
    {DPNoOp, "DPNoOp", 0, 1},
    {DPNoOpTag, "DPNoOpTag", 1, 1},
    {DPPipelineMode, "DPPipelineMode", 1, 1},
    {DPSetBlendColor, "DPSetBlendColor", 4, 1},
    {DPSetEnvColor, "DPSetEnvColor", 4, 1},
    {DPSetFillColor, "DPSetFillColor", 1, 1},
    {DPSetFogColor, "DPSetFogColor", 4, 1},
    {DPSetPrimColor, "DPSetPrimColor", 6, 1},
    {DPSetColorImage, "DPSetColorImage", 4, 1},
    {DPSetDepthImage, "DPSetDepthImage", 1, 1},
    {DPSetTextureImage, "DPSetTextureImage", 4, 1},
    {DPSetHilite1Tile, "DPSetHilite1Tile", 4, 1},
    {DPSetHilite2Tile, "DPSetHilite2Tile", 4, 1},
    {DPSetAlphaCompare, "DPSetAlphaCompare", 1, 1},
    {DPSetAlphaDither, "DPSetAlphaDither", 1, 1},
    {DPSetColorDither, "DPSetColorDither", 1, 1},
    {DPSetCombineMode, "DPSetCombineMode", 2, 1},
    {DPSetCombineLERP, "DPSetCombineLERP", 16, 1},
    {DPSetConvert, "DPSetConvert", 6, 1},
    {DPSetTextureConvert, "DPSetTextureConvert", 1, 1},
    {DPSetCycleType, "DPSetCycleType", 1, 1},
    {DPSetDepthSource, "DPSetDepthSource", 1, 1},
    {DPSetCombineKey, "DPSetCombineKey", 1, 1},
    {DPSetKeyGB, "DPSetKeyGB", 6, 1},
    {DPSetKeyR, "DPSetKeyR", 3, 1},
    {DPSetPrimDepth, "DPSetPrimDepth", 2, 1},
    {DPSetRenderMode, "DPSetRenderMode", 2, 1},
    {DPSetScissor, "DPSetScissor", 5, 1},
    {DPSetTextureDetail, "DPSetTextureDetail", 1, 1},
    {DPSetTextureFilter, "DPSetTextureFilter", 1, 1},
    {DPSetTextureLOD, "DPSetTextureLOD", 1, 1},
    {DPSetTextureLUT, "DPSetTextureLUT", 1, 1},
    {DPSetTexturePersp, "DPSetTexturePersp", 1, 1},
    {DPSetTile, "DPSetTile", 12, 1},
    {DPSetTileSize, "DPSetTileSize", 5, 1},
    {SP1Triangle, "SP1Triangle", 4, 1},
    {SP2Triangles, "SP2Triangles", 8, 1},
    {SPBranchLessZ, "SPBranchLessZ", 6, 2},
    {SPBranchLessZrg, "SPBranchLessZrg", 8, 2},
    {SPBranchList, "SPBranchList", 1, 1},
    {SPClipRatio, "SPClipRatio", 1, 4},
    {SPCullDisplayList, "SPCullDisplayList", 2, 1},
    {SPDisplayList, "SPDisplayList", 1, 1},
    {SPEndDisplayList, "SPEndDisplayList", 0, 1},
    {SPFogPosition, "SPFogPosition", 2, 1},
    {SPForceMatrix, "SPForceMatrix", 1, 2},
    {SPSetGeometryMode, "SPSetGeometryMode", 1, 1},
    {SPClearGeometryMode, "SPClearGeometryMode", 1, 1},
    {SPInsertMatrix, "SPInsertMatrix", 2, 1},
    {SPLine3D, "SPLine3D", 3, 1},
    {SPLineW3D, "SPLineW3D", 4, 1},
    {SPLoadUcode, "SPLoadUcode", 2, 2},
    {SPLoadUcodeL, "SPLoadUcodeL", 1, 2},
    {SPLookAt, "SPLookAt", 1, 2},
    {SPMatrix, "SPMatrix", 2, 1},
    {SPModifyVertex, "SPModifyVertex", 3, 1},
    {SPPerspNormalize, "SPPerspNormalize", 1, 1},
    {SPPopMatrix, "SPPopMatrix", 1, 1},
    {SPSegment, "SPSegment", 2, 1},
    {SPSetLights0, "SPSetLights0", 1, 3},
    {SPSetLights1, "SPSetLights1", 1, 3},
    {SPSetLights2, "SPSetLights2", 1, 4},
    {SPSetLights3, "SPSetLights3", 1, 5},
    {SPSetLights4, "SPSetLights4", 1, 6},
    {SPSetLights5, "SPSetLights5", 1, 7},
    {SPSetLights6, "SPSetLights6", 1, 8},
    {SPSetLights7, "SPSetLights7", 1, 9},
    {SPSetStatus, "SPSetStatus", 2, 1},
    {SPNumLights, "SPNumLights", 1, 1},
    {SPLight, "SPLight", 2, 1},
    {SPLightColor, "SPLightColor", 2, 2},
    {SPTexture, "SPTexture", 5, 1},
    {SPTextureRectangle, "SPTextureRectangle", 9, 3},
    {SPScisTextureRectangle, "SPScisTextureRectangle", 9, 3},
    {SPTextureRectangleFlip, "SPTextureRectangleFlip", 9, 3},
    {SPVertex, "SPVertex", 3, 1},
    {SPViewport, "SPViewport", 1, 1},
    {SPBgRectCopy, "SPBgRectCopy", 1, 1},
    {SPBgRect1Cyc, "SPBgRect1Cyc", 1, 1},
    {SPObjRectangle, "SPObjRectangle", 1, 1},
    {SPObjRectangleR, "SPObjRectangleR", 1, 1},
    {SPObjSprite, "SPObjSprite", 1, 1},
    {SPObjMatrix, "SPObjMatrix", 1, 1},
    {SPObjSubMatrix, "SPObjSubMatrix", 1, 1},
    {SPObjRenderMode, "SPObjRenderMode", 1, 1},
    {SPObjLoadTxtr, "SPObjLoadTxtr", 1, 1},
    {SPObjLoadTxRect, "SPObjLoadTxRect", 1, 1},
    {SPObjLoadTxRectR, "SPObjLoadTxRectR", 1, 1},
    {SPObjLoadTxSprite, "SPObjLoadTxSprite", 1, 1},
    {SPSelectDL, "SPSelectDL", 4, 2},
    {SPSelectBranchDL, "SPSelectBranchDL", 4, 2},
};

// List of valid macros as of F3DEX2
const GBIMacros macros_f3dex2[] = {
    // Matrix
    {enum_G_MTX_MODELVIEW, "G_MTX_MODELVIEW", 0x00},
    {enum_G_MTX_PROJECTION, "G_MTX_PROJECTION", 0x04},
    {enum_G_MTX_MUL, "G_MTX_MUL", 0x00},
    {enum_G_MTX_LOAD, "G_MTX_LOAD", 0x02},
    {enum_G_MTX_NOPUSH, "G_MTX_NOPUSH", 0x00},
    {enum_G_MTX_PUSH, "G_MTX_PUSH", 0x01},

    // Geometry Modes
    {enum_G_ZBUFFER, "G_ZBUFFER", 0x00000001},
    {enum_G_SHADE, "G_SHADE", 0x00000004},
    {enum_G_TEXTURE_ENABLE, "G_TEXTURE_ENABLE", 0x00000000},
    {enum_G_SHADING_SMOOTH, "G_SHADING_SMOOTH", 0x00200000},
    {enum_G_CULL_FRONT, "G_CULL_FRONT", 0x00000200},
    {enum_G_CULL_BACK, "G_CULL_BACK", 0x00000400},
    {enum_G_CULL_BOTH, "G_CULL_BOTH", 0x00000600},
    {enum_G_FOG, "G_FOG", 0x00010000},
    {enum_G_LIGHTING, "G_LIGHTING", 0x00020000},
    {enum_G_TEXTURE_GEN, "G_TEXTURE_GEN", 0x00040000},
    {enum_G_TEXTURE_GEN_LINEAR, "G_TEXTURE_GEN_LINEAR", 0x00080000},
    {enum_G_LOD, "G_LOD", 0x00100000},
    {enum_G_CLIPPING, "G_CLIPPING", 0x00800000},

    // Texture loading
    {enum_G_TX_LOADTILE, "G_TX_LOADTILE", 7},
    {enum_G_TX_RENDERTILE, "G_TX_RENDERTILE", 0},
    {enum_G_TX_NOMIRROR, "G_TX_NOMIRROR", 0},
    {enum_G_TX_WRAP, "G_TX_WRAP", 0},
    {enum_G_TX_MIRROR, "G_TX_MIRROR", 0x1},
    {enum_G_TX_CLAMP, "G_TX_CLAMP", 0x2},
    {enum_G_TX_NOMASK, "G_TX_NOMASK", 0},
    {enum_G_TX_NOLOD, "G_TX_NOLOD", 0},
    {enum_G_IM_FMT_RGBA, "G_IM_FMT_RGBA", 0},
    {enum_G_IM_FMT_YUV, "G_IM_FMT_YUV", 1},
    {enum_G_IM_FMT_CI, "G_IM_FMT_CI", 2},
    {enum_G_IM_FMT_IA, "G_IM_FMT_IA", 3},
    {enum_G_IM_FMT_I, "G_IM_FMT_I", 4},
    {enum_G_IM_SIZ_4b, "G_IM_SIZ_4b", 0},
    {enum_G_IM_SIZ_8b, "G_IM_SIZ_8b", 1},
    {enum_G_IM_SIZ_16b, "G_IM_SIZ_16b", 2},
    {enum_G_IM_SIZ_32b, "G_IM_SIZ_32b", 3},

    // Pipeline Modes
    {enum_G_PM_1PRIMITIVE, "G_PM_1PRIMITIVE", (1 << 23)},
    {enum_G_PM_NPRIMITIVE, "G_PM_NPRIMITIVE", (0 << 23)},

    // Cycle modes
    {enum_G_CYC_1CYCLE, "G_CYC_1CYCLE", (0 << 20)},
    {enum_G_CYC_2CYCLE, "G_CYC_2CYCLE", (1 << 20)},
    {enum_G_CYC_COPY, "G_CYC_COPY", (2 << 20)},
    {enum_G_CYC_FILL, "G_CYC_FILL", (3 << 20)},

    // Texture perspective modes
    {enum_G_TP_NONE, "G_TP_NONE", (0 << 19)},
    {enum_G_TP_PERSP, "G_TP_PERSP", (1 << 19)},

    // Texture detail
    {enum_G_TD_CLAMP, "G_TD_CLAMP", (0 << 17)},
    {enum_G_TD_SHARPEN, "G_TD_SHARPEN", (1 << 17)},
    {enum_G_TD_DETAIL, "G_TD_DETAIL", (2 << 17)},

    // Texture LOD
    {enum_G_TL_TILE, "G_TL_TILE", (0 << 16)},
    {enum_G_TL_LOD, "G_TL_LOD", (1 << 16)},

    // Texture LUT
    {enum_G_TT_NONE, "G_TT_NONE", (0 << 14)},
    {enum_G_TT_RGBA16, "G_TT_RGBA16", (2 << 14)},
    {enum_G_TT_IA16, "G_TT_IA16", (3 << 14)},

    // Texture Filtering
    {enum_G_TF_POINT, "G_TF_POINT", (0 << 12)},
    {enum_G_TF_AVERAGE, "G_TF_AVERAGE", (3 << 12)},
    {enum_G_TF_BILERP, "G_TF_BILERP", (2 << 12)},

    // Texture Convert
    {enum_G_TC_CONV, "G_TC_CONV", (0 << 9)},
    {enum_G_TC_FILTCONV, "G_TC_FILTCONV", (5 << 9)},
    {enum_G_TC_FILT, "G_TC_FILT", (6 << 9)},

    // Combine Key
    {enum_G_CK_NONE, "G_CK_NONE", (0 << 8)},
    {enum_G_CK_KEY, "G_CK_KEY", (1 << 8)},

    // Color Dither
    {enum_G_CD_MAGICSQ, "G_CD_MAGICSQ", (0 << 6)},
    {enum_G_CD_BAYER, "G_CD_BAYER", (1 << 6)},
    {enum_G_CD_NOISE, "G_CD_NOISE", (2 << 6)},
    {enum_G_CD_ENABLE, "G_CD_ENABLE", (1 << 2)},
    {enum_G_CD_DISABLE, "G_CD_DISABLE", (0 << 2)},

    // Alpha Dither
    {enum_G_AD_PATTERN, "G_AD_PATTERN", (0 << 4)},
    {enum_G_AD_NOTPATTERN, "G_AD_NOTPATTERN", (1 << 4)},
    {enum_G_AD_NOISE, "G_AD_NOISE", (2 << 4)},
    {enum_G_AD_DISABLE, "G_AD_DISABLE", (3 << 4)},

    // Alpha Compare
    {enum_G_AC_NONE, "G_AC_NONE", (0 << 0)},
    {enum_G_AC_THRESHOLD, "G_AC_THRESHOLD", (1 << 0)},
    {enum_G_AC_DITHER, "G_AC_DITHER", (3 << 0)},

    // Depth Source
    {enum_G_ZS_PIXEL, "G_ZS_PIXEL", (0 << 2)},
    {enum_G_ZS_PRIM, "G_ZS_PRIM", (1 << 2)},

    // Set Convert
    {enum_G_CV_K0, "G_CV_K0", 175},
    {enum_G_CV_K1, "G_CV_K1", -43},
    {enum_G_CV_K2, "G_CV_K2", -89},
    {enum_G_CV_K3, "G_CV_K3", 222},
    {enum_G_CV_K4, "G_CV_K4", 114},
    {enum_G_CV_K5, "G_CV_K5", 42},

    // Set Scissor
    {enum_G_SC_NON_INTERLACE, "G_SC_NON_INTERLACE", 0},
    {enum_G_SC_ODD_INTERLACE, "G_SC_ODD_INTERLACE", 3},
    {enum_G_SC_EVEN_INTERLACE, "G_SC_EVEN_INTERLACE", 2},

    // CC Modes are not included since they're complicated

    // CC Constants
    {enum_G_CCMUX_COMBINED, "G_CCMUX_COMBINED", 0},
    {enum_G_CCMUX_TEXEL0, "G_CCMUX_TEXEL0", 1},
    {enum_G_CCMUX_TEXEL1, "G_CCMUX_TEXEL1", 2},
    {enum_G_CCMUX_PRIMITIVE, "G_CCMUX_PRIMITIVE", 3},
    {enum_G_CCMUX_SHADE, "G_CCMUX_SHADE", 4},
    {enum_G_CCMUX_ENVIRONMENT, "G_CCMUX_ENVIRONMENT", 5},
    {enum_G_CCMUX_CENTER, "G_CCMUX_CENTER", 6},
    {enum_G_CCMUX_SCALE, "G_CCMUX_SCALE", 6},
    {enum_G_CCMUX_COMBINED_ALPHA, "G_CCMUX_COMBINED_ALPHA", 7},
    {enum_G_CCMUX_TEXEL0_ALPHA, "G_CCMUX_TEXEL0_ALPHA", 8},
    {enum_G_CCMUX_TEXEL1_ALPHA, "G_CCMUX_TEXEL1_ALPHA", 9},
    {enum_G_CCMUX_PRIMITIVE_ALPHA, "G_CCMUX_PRIMITIVE_ALPHA", 10},
    {enum_G_CCMUX_SHADE_ALPHA, "G_CCMUX_SHADE_ALPHA", 11},
    {enum_G_CCMUX_ENV_ALPHA, "G_CCMUX_ENV_ALPHA", 12},
    {enum_G_CCMUX_LOD_FRACTION, "G_CCMUX_LOD_FRACTION", 13},
    {enum_G_CCMUX_PRIM_LOD_FRAC, "G_CCMUX_PRIM_LOD_FRAC", 14},
    {enum_G_CCMUX_NOISE, "G_CCMUX_NOISE", 7},
    {enum_G_CCMUX_K4, "G_CCMUX_K4", 7},
    {enum_G_CCMUX_K5, "G_CCMUX_K5", 15},
    {enum_G_CCMUX_1, "G_CCMUX_1", 6},
    {enum_G_CCMUX_0, "G_CCMUX_0", 31},

    // CC Alpha Constants
    {enum_G_ACMUX_COMBINED, "G_ACMUX_COMBINED", 0},
    {enum_G_ACMUX_TEXEL0, "G_ACMUX_TEXEL0", 1},
    {enum_G_ACMUX_TEXEL1, "G_ACMUX_TEXEL1", 2},
    {enum_G_ACMUX_PRIMITIVE, "G_ACMUX_PRIMITIVE", 3},
    {enum_G_ACMUX_SHADE, "G_ACMUX_SHADE", 4},
    {enum_G_ACMUX_ENVIRONMENT, "G_ACMUX_ENVIRONMENT", 5},
    {enum_G_ACMUX_LOD_FRACTION, "G_ACMUX_LOD_FRACTION", 0},
    {enum_G_ACMUX_PRIM_LOD_FRAC, "G_ACMUX_PRIM_LOD_FRAC", 6},
    {enum_G_ACMUX_1, "G_ACMUX_1", 6},
    {enum_G_ACMUX_0, "G_ACMUX_0", 7},

    // Render modes
    {enum_G_RM_AA_ZB_OPA_SURF, "G_RM_AA_ZB_OPA_SURF", 0x00442078},
    {enum_G_RM_AA_ZB_OPA_SURF2, "G_RM_AA_ZB_OPA_SURF2", 0x00112078},
    {enum_G_RM_AA_ZB_XLU_SURF, "G_RM_AA_ZB_XLU_SURF", 0x004049d8},
    {enum_G_RM_AA_ZB_XLU_SURF2, "G_RM_AA_ZB_XLU_SURF2", 0x001049d8},
    {enum_G_RM_AA_ZB_OPA_DECAL, "G_RM_AA_ZB_OPA_DECAL", 0x00442d58},
    {enum_G_RM_AA_ZB_OPA_DECAL2, "G_RM_AA_ZB_OPA_DECAL2", 0x00112d58},
    {enum_G_RM_AA_ZB_XLU_DECAL, "G_RM_AA_ZB_XLU_DECAL", 0x00404dd8},
    {enum_G_RM_AA_ZB_XLU_DECAL2, "G_RM_AA_ZB_XLU_DECAL2", 0x00104dd8},
    {enum_G_RM_AA_ZB_OPA_INTER, "G_RM_AA_ZB_OPA_INTER", 0x00442478},
    {enum_G_RM_AA_ZB_OPA_INTER2, "G_RM_AA_ZB_OPA_INTER2", 0x00112478},
    {enum_G_RM_AA_ZB_XLU_INTER, "G_RM_AA_ZB_XLU_INTER", 0x004045d8},
    {enum_G_RM_AA_ZB_XLU_INTER2, "G_RM_AA_ZB_XLU_INTER2", 0x001045d8},
    {enum_G_RM_AA_ZB_XLU_LINE, "G_RM_AA_ZB_XLU_LINE", 0x00407858},
    {enum_G_RM_AA_ZB_XLU_LINE2, "G_RM_AA_ZB_XLU_LINE2", 0x00107858},
    {enum_G_RM_AA_ZB_DEC_LINE, "G_RM_AA_ZB_DEC_LINE", 0x00407f58},
    {enum_G_RM_AA_ZB_DEC_LINE2, "G_RM_AA_ZB_DEC_LINE2", 0x00107f58},
    {enum_G_RM_AA_ZB_TEX_EDGE, "G_RM_AA_ZB_TEX_EDGE", 0x00443078},
    {enum_G_RM_AA_ZB_TEX_EDGE2, "G_RM_AA_ZB_TEX_EDGE2", 0x00113078},
    {enum_G_RM_AA_ZB_TEX_INTER, "G_RM_AA_ZB_TEX_INTER", 0x00443478},
    {enum_G_RM_AA_ZB_TEX_INTER2, "G_RM_AA_ZB_TEX_INTER2", 0x00113478},
    {enum_G_RM_AA_ZB_SUB_SURF, "G_RM_AA_ZB_SUB_SURF", 0x00442278},
    {enum_G_RM_AA_ZB_SUB_SURF2, "G_RM_AA_ZB_SUB_SURF2", 0x00112278},
    {enum_G_RM_AA_ZB_PCL_SURF, "G_RM_AA_ZB_PCL_SURF", 0x0040007b},
    {enum_G_RM_AA_ZB_PCL_SURF2, "G_RM_AA_ZB_PCL_SURF2", 0x0010007b},
    {enum_G_RM_AA_ZB_OPA_TERR, "G_RM_AA_ZB_OPA_TERR", 0x00402078},
    {enum_G_RM_AA_ZB_OPA_TERR2, "G_RM_AA_ZB_OPA_TERR2", 0x00102078},
    {enum_G_RM_AA_ZB_TEX_TERR, "G_RM_AA_ZB_TEX_TERR", 0x00403078},
    {enum_G_RM_AA_ZB_TEX_TERR2, "G_RM_AA_ZB_TEX_TERR2", 0x00103078},
    {enum_G_RM_AA_ZB_SUB_TERR, "G_RM_AA_ZB_SUB_TERR", 0x00402278},
    {enum_G_RM_AA_ZB_SUB_TERR2, "G_RM_AA_ZB_SUB_TERR2", 0x00102278},
    {enum_G_RM_RA_ZB_OPA_SURF, "G_RM_RA_ZB_OPA_SURF", 0x00442038},
    {enum_G_RM_RA_ZB_OPA_SURF2, "G_RM_RA_ZB_OPA_SURF2", 0x00112038},
    {enum_G_RM_RA_ZB_OPA_DECAL, "G_RM_RA_ZB_OPA_DECAL", 0x00442d18},
    {enum_G_RM_RA_ZB_OPA_DECAL2, "G_RM_RA_ZB_OPA_DECAL2", 0x00112d18},
    {enum_G_RM_RA_ZB_OPA_INTER, "G_RM_RA_ZB_OPA_INTER", 0x00442438},
    {enum_G_RM_RA_ZB_OPA_INTER2, "G_RM_RA_ZB_OPA_INTER2", 0x00112438},
    {enum_G_RM_AA_OPA_SURF, "G_RM_AA_OPA_SURF", 0x00442048},
    {enum_G_RM_AA_OPA_SURF2, "G_RM_AA_OPA_SURF2", 0x00112048},
    {enum_G_RM_AA_XLU_SURF, "G_RM_AA_XLU_SURF", 0x004041c8},
    {enum_G_RM_AA_XLU_SURF2, "G_RM_AA_XLU_SURF2", 0x001041c8},
    {enum_G_RM_AA_XLU_LINE, "G_RM_AA_XLU_LINE", 0x00407048},
    {enum_G_RM_AA_XLU_LINE2, "G_RM_AA_XLU_LINE2", 0x00107048},
    {enum_G_RM_AA_DEC_LINE, "G_RM_AA_DEC_LINE", 0x00407248},
    {enum_G_RM_AA_DEC_LINE2, "G_RM_AA_DEC_LINE2", 0x00107248},
    {enum_G_RM_AA_TEX_EDGE, "G_RM_AA_TEX_EDGE", 0x00443048},
    {enum_G_RM_AA_TEX_EDGE2, "G_RM_AA_TEX_EDGE2", 0x00113048},
    {enum_G_RM_AA_SUB_SURF, "G_RM_AA_SUB_SURF", 0x00442248},
    {enum_G_RM_AA_SUB_SURF2, "G_RM_AA_SUB_SURF2", 0x00112248},
    {enum_G_RM_AA_PCL_SURF, "G_RM_AA_PCL_SURF", 0x0040004b},
    {enum_G_RM_AA_PCL_SURF2, "G_RM_AA_PCL_SURF2", 0x0010004b},
    {enum_G_RM_AA_OPA_TERR, "G_RM_AA_OPA_TERR", 0x00402048},
    {enum_G_RM_AA_OPA_TERR2, "G_RM_AA_OPA_TERR2", 0x00102048},
    {enum_G_RM_AA_TEX_TERR, "G_RM_AA_TEX_TERR", 0x00403048},
    {enum_G_RM_AA_TEX_TERR2, "G_RM_AA_TEX_TERR2", 0x00103048},
    {enum_G_RM_AA_SUB_TERR, "G_RM_AA_SUB_TERR", 0x00402248},
    {enum_G_RM_AA_SUB_TERR2, "G_RM_AA_SUB_TERR2", 0x00102248},
    {enum_G_RM_RA_OPA_SURF, "G_RM_RA_OPA_SURF", 0x00442008},
    {enum_G_RM_RA_OPA_SURF2, "G_RM_RA_OPA_SURF2", 0x00112008},
    {enum_G_RM_ZB_OPA_SURF, "G_RM_ZB_OPA_SURF", 0x00442230},
    {enum_G_RM_ZB_OPA_SURF2, "G_RM_ZB_OPA_SURF2", 0x00112230},
    {enum_G_RM_ZB_XLU_SURF, "G_RM_ZB_XLU_SURF", 0x00404a50},
    {enum_G_RM_ZB_XLU_SURF2, "G_RM_ZB_XLU_SURF2", 0x00104a50},
    {enum_G_RM_ZB_OPA_DECAL, "G_RM_ZB_OPA_DECAL", 0x00442e10},
    {enum_G_RM_ZB_OPA_DECAL2, "G_RM_ZB_OPA_DECAL2", 0x00112e10},
    {enum_G_RM_ZB_XLU_DECAL, "G_RM_ZB_XLU_DECAL", 0x00404e50},
    {enum_G_RM_ZB_XLU_DECAL2, "G_RM_ZB_XLU_DECAL2", 0x00104e50},
    {enum_G_RM_ZB_CLD_SURF, "G_RM_ZB_CLD_SURF", 0x00404b50},
    {enum_G_RM_ZB_CLD_SURF2, "G_RM_ZB_CLD_SURF2", 0x00104b50},
    {enum_G_RM_ZB_OVL_SURF, "G_RM_ZB_OVL_SURF", 0x00404f50},
    {enum_G_RM_ZB_OVL_SURF2, "G_RM_ZB_OVL_SURF2", 0x00104f50},
    {enum_G_RM_ZB_PCL_SURF, "G_RM_ZB_PCL_SURF", 0x0c080233},
    {enum_G_RM_ZB_PCL_SURF2, "G_RM_ZB_PCL_SURF2", 0x03020233},
    {enum_G_RM_OPA_SURF, "G_RM_OPA_SURF", 0x0c084000},
    {enum_G_RM_OPA_SURF2, "G_RM_OPA_SURF2", 0x03024000},
    {enum_G_RM_XLU_SURF, "G_RM_XLU_SURF", 0x00404240},
    {enum_G_RM_XLU_SURF2, "G_RM_XLU_SURF2", 0x00104240},
    {enum_G_RM_CLD_SURF, "G_RM_CLD_SURF", 0x00404340},
    {enum_G_RM_CLD_SURF2, "G_RM_CLD_SURF2", 0x00104340},
    {enum_G_RM_TEX_EDGE, "G_RM_TEX_EDGE", 0x0c087008},
    {enum_G_RM_TEX_EDGE2, "G_RM_TEX_EDGE2", 0x03027008},
    {enum_G_RM_PCL_SURF, "G_RM_PCL_SURF", 0x0c084203},
    {enum_G_RM_PCL_SURF2, "G_RM_PCL_SURF2", 0x03024203},
    {enum_G_RM_ADD, "G_RM_ADD", 0x04484340},
    {enum_G_RM_ADD2, "G_RM_ADD2", 0x01124340},
    {enum_G_RM_NOOP, "G_RM_NOOP", 0x00000000},
    {enum_G_RM_NOOP2, "G_RM_NOOP2", 0x00000000},
    {enum_G_RM_VISCVG, "G_RM_VISCVG", 0x0c844040},
    {enum_G_RM_VISCVG2, "G_RM_VISCVG2", 0x03214040},
    {enum_G_RM_OPA_CI, "G_RM_OPA_CI", 0x0c080000},
    {enum_G_RM_OPA_CI2, "G_RM_OPA_CI2", 0x03020000},
    {enum_G_RM_FOG_SHADE_A, "G_RM_FOG_SHADE_A", 0xc8000000},
    {enum_G_RM_FOG_PRIM_A, "G_RM_FOG_PRIM_A", 0xc4000000},
    {enum_G_RM_PASS, "G_RM_PASS", 0x0c080000},
};

// List of what all default CC modes resolve to
const CombineMode ccmodes_f3dex2[] = {
    {"G_CC_PRIMITIVE",              {31, 31, 31, 3,  7, 7, 7, 3}},
    {"G_CC_SHADE",                  {31, 31, 31, 4,  7, 7, 7, 4}},
    {"G_CC_MODULATEI",              {1, 31, 4, 31,   7, 7, 7, 4}},
    {"G_CC_MODULATEIA",             {1, 31, 4, 31,   1, 7, 4, 7}},
    {"G_CC_MODULATEIDECALA",        {1, 31, 4, 31,   7, 7, 7, 1}},
    {"G_CC_MODULATERGB",            {1, 31, 4, 31,   7, 7, 7, 4}},
    {"G_CC_MODULATERGBA",           {1, 31, 4, 31,   1, 7, 4, 7}},
    {"G_CC_MODULATERGBDECALA",      {1, 31, 4, 31,   7, 7, 7, 1}},
    {"G_CC_MODULATEI_PRIM",         {1, 31, 3, 31,   7, 7, 7, 3}},
    {"G_CC_MODULATEIA_PRIM",        {1, 31, 3, 31,   1, 7, 3, 7}},
    {"G_CC_MODULATEIDECALA_PRIM",   {1, 31, 3, 31,   7, 7, 7, 1}},
    {"G_CC_MODULATERGB_PRIM",       {1, 31, 3, 31,   7, 7, 7, 3}},
    {"G_CC_MODULATERGBA_PRIM",      {1, 31, 3, 31,   1, 7, 3, 7}},
    {"G_CC_MODULATERGBDECALA_PRIM", {1, 31, 3, 31,   7, 7, 7, 1}},
    {"G_CC_DECALRGB",               {31, 31, 31, 1,  7, 7, 7, 4}},
    {"G_CC_DECALRGBA",              {31, 31, 31, 1,  7, 7, 7, 1}},
    {"G_CC_BLENDI",                 {5, 4, 1, 4,     7, 7, 7, 4}},
    {"G_CC_BLENDIA",                {5, 4, 1, 4,     1, 7, 4, 7}},
    {"G_CC_BLENDIDECALA",           {5, 4, 1, 4,     7, 7, 7, 1}},
    {"G_CC_BLENDRGBA",              {1, 4, 8, 4,     7, 7, 7, 4}},
    {"G_CC_BLENDRGBDECALA",         {1, 4, 8, 4,     7, 7, 7, 1}},
    {"G_CC_ADDRGB",                 {6, 31, 1, 4,    7, 7, 7, 4}},
    {"G_CC_ADDRGBDECALA",           {6, 31, 1, 4,    7, 7, 7, 1}},
    {"G_CC_REFLECTRGB",             {5, 31, 1, 4,    7, 7, 7, 4}},
    {"G_CC_REFLECTRGBDECALA",       {5, 31, 1, 4,    7, 7, 7, 1}},
    {"G_CC_HILITERGB",              {3, 4, 1, 4,     7, 7, 7, 4}},
    {"G_CC_HILITERGBA",             {3, 4, 1, 4,     3, 4, 1, 4}},
    {"G_CC_HILITERGBDECALA",        {3, 4, 1, 4,     7, 7, 7, 1}},
    {"G_CC_SHADEDECALA",            {31, 31, 31, 4,  7, 7, 7, 1}},
    {"G_CC_BLENDPE",                {3, 5, 1, 5,     1, 7, 4, 7}},
    {"G_CC_BLENDPEDECALA",          {3, 5, 1, 5,     7, 7, 7, 1}},
    {"_G_CC_BLENDPE",               {5, 3, 1, 3,     1, 7, 4, 7}},
    {"_G_CC_BLENDPEDECALA",         {5, 3, 1, 3,     7, 7, 7, 1}},
    {"_G_CC_TWOCOLORTEX",           {3, 4, 1, 4,     7, 7, 7, 4}},
    {"_G_CC_SPARSEST",              {3, 1, 13, 1,    3, 1, 0, 1}},
    {"G_CC_TEMPLERP",               {2, 1, 14, 1,    2, 1, 6, 1}},
    {"G_CC_TRILERP",                {2, 1, 13, 1,    2, 1, 0, 1}},
    {"G_CC_INTERFERENCE",           {1, 31, 2, 31,   1, 7, 2, 7}},
    {"G_CC_1CYUV2RGB",              {1, 7, 15, 1,    7, 7, 7, 4}},
    {"G_CC_YUV2RGB",                {2, 7, 15, 2,    7, 7, 7, 7}},
    {"G_CC_PASS2",                  {31, 31, 31, 0,  7, 7, 7, 0}},
    {"G_CC_MODULATEI2",             {0, 31, 4, 31,   7, 7, 7, 4}},
    {"G_CC_MODULATEIA2",            {0, 31, 4, 31,   0, 7, 4, 7}},
    {"G_CC_MODULATERGB2",           {0, 31, 4, 31,   7, 7, 7, 4}},
    {"G_CC_MODULATERGBA2",          {0, 31, 4, 31,   0, 7, 4, 7}},
    {"G_CC_MODULATEI_PRIM2",        {0, 31, 3, 31,   7, 7, 7, 3}},
    {"G_CC_MODULATEIA_PRIM2",       {0, 31, 3, 31,   0, 7, 3, 7}},
    {"G_CC_MODULATERGB_PRIM2",      {0, 31, 3, 31,   7, 7, 7, 3}},
    {"G_CC_MODULATERGBA_PRIM2",     {0, 31, 3, 31,   0, 7, 3, 7}},
    {"G_CC_DECALRGB2",              {31, 31, 31, 0,  7, 7, 7, 4}},
    {"G_CC_DECALRGBA2",             {0, 4, 7, 4,     7, 7, 7, 4}},
    {"G_CC_BLENDI2",                {5, 4, 0, 4,     7, 7, 7, 4}},
    {"G_CC_BLENDIA2",               {5, 4, 0, 4,     0, 7, 4, 7}},
    {"G_CC_CHROMA_KEY2",            {1, 6, 6, 31,    7, 7, 7, 7}},
    {"G_CC_HILITERGB2",             {5, 0, 1, 0,     7, 7, 7, 4}},
    {"G_CC_HILITERGBA2",            {5, 0, 1, 0,     5, 0, 1, 0}},
    {"G_CC_HILITERGBDECALA2",       {5, 0, 1, 0,     7, 7, 7, 1}},
    {"G_CC_HILITERGBPASSA2",        {5, 0, 1, 0,     7, 7, 7, 0}},
    {"G_CC_PRIMLITE",               {4, 31, 3, 31,   7, 7, 7, 3}},
};


/*==============================
    gbi_resolveccmode
    Take in a CC mode as a string and return the mode data
    @param   The string with the CC mode
    @returns The mode data
==============================*/

uint8_t* gbi_resolveccmode(char* ccmode)
{
    for (int i=0; i<sizeof(ccmodes_f3dex2)/sizeof(ccmodes_f3dex2[0]); i++)
        if (!strcmp(ccmodes_f3dex2[i].str, ccmode))
            return (uint8_t*)ccmodes_f3dex2[i].values;
    return NULL;
}


/*==============================
    gbi_resolvemacro
    Take in a GBI macro and convert it to a number.
    Also handles a combo of macros with the | value 
    @param   The string with the GBI macro
    @returns The number representation
==============================*/

int32_t gbi_resolvemacro(char* macro)
{
    char* strptr;
    char* macrocpy = malloc(sizeof(char)*(strlen(macro)+1));
    int32_t ret = 0;
    strcpy(macrocpy, macro);
    strptr = strtok (macrocpy," |");
    while (strptr != NULL)
    {
        for (int i=0; i<sizeof(macros_f3dex2)/sizeof(macros_f3dex2[0]); i++)
            if (!strcmp(macros_f3dex2[i].str, strptr))
                ret |= macros_f3dex2[i].value;
        strptr = strtok (NULL, " |");
    }
    free(macrocpy);
    return ret;
}