"\n"
"#include \"AtmosphereShadersCommon.fxh\"\n"
"\n"
"cbuffer cbPostProcessingAttribs\n"
"{\n"
"    EpipolarLightScatteringAttribs g_PPAttribs;\n"
"}\n"
"\n"
"// This function computes entry point of the epipolar line given its exit point\n"
"//                  \n"
"//    g_PPAttribs.f4LightScreenPos\n"
"//       *\n"
"//        \\\n"
"//         \\  f2EntryPoint\n"
"//        __\\/___\n"
"//       |   \\   |\n"
"//       |    \\  |\n"
"//       |_____\\_|\n"
"//           | |\n"
"//           | f2ExitPoint\n"
"//           |\n"
"//        Exit boundary\n"
"float2 GetEpipolarLineEntryPoint(float2 f2ExitPoint)\n"
"{\n"
"    float2 f2EntryPoint;\n"
"\n"
"    //if( IsValidScreenLocation(g_PPAttribs.f4LightScreenPos.xy) )\n"
"    if (g_PPAttribs.bIsLightOnScreen)\n"
"    {\n"
"        // If light source is on the screen, its location is entry point for each epipolar line\n"
"        f2EntryPoint = g_PPAttribs.f4LightScreenPos.xy;\n"
"    }\n"
"    else\n"
"    {\n"
"        // If light source is outside the screen, we need to compute intersection of the ray with\n"
"        // the screen boundaries\n"
"        \n"
"        // Compute direction from the light source to the exit point\n"
"        // Note that exit point must be located on shrinked screen boundary\n"
"        float2 f2RayDir = f2ExitPoint.xy - g_PPAttribs.f4LightScreenPos.xy;\n"
"        float fDistToExitBoundary = length(f2RayDir);\n"
"        f2RayDir /= fDistToExitBoundary;\n"
"        // Compute signed distances along the ray from the light position to all four boundaries\n"
"        // The distances are computed as follows using vector instructions:\n"
"        // float fDistToLeftBoundary   = abs(f2RayDir.x) > 1e-5 ? (-1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;\n"
"        // float fDistToBottomBoundary = abs(f2RayDir.y) > 1e-5 ? (-1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;\n"
"        // float fDistToRightBoundary  = abs(f2RayDir.x) > 1e-5 ? ( 1 - g_PPAttribs.f4LightScreenPos.x) / f2RayDir.x : -FLT_MAX;\n"
"        // float fDistToTopBoundary    = abs(f2RayDir.y) > 1e-5 ? ( 1 - g_PPAttribs.f4LightScreenPos.y) / f2RayDir.y : -FLT_MAX;\n"
"        \n"
"        // Note that in fact the outermost visible screen pixels do not lie exactly on the boundary (+1 or -1), but are biased by\n"
"        // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in\n"
"        // smaller number of pixels which require inscattering correction\n"
"        float4 f4Boundaries = GetOutermostScreenPixelCoords(g_PPAttribs.f4ScreenResolution);\n"
"        bool4 b4IsCorrectIntersectionFlag = Greater( abs(f2RayDir.xyxy), 1e-5 * F4ONE );\n"
"        float4 f4DistToBoundaries = (f4Boundaries - g_PPAttribs.f4LightScreenPos.xyxy) / (f2RayDir.xyxy + BoolToFloat( Not(b4IsCorrectIntersectionFlag) ) );\n"
"        // Addition of !b4IsCorrectIntersectionFlag is required to prevent divison by zero\n"
"        // Note that such incorrect lanes will be masked out anyway\n"
"\n"
"        // We now need to find first intersection BEFORE the intersection with the exit boundary\n"
"        // This means that we need to find maximum intersection distance which is less than fDistToBoundary\n"
"        // We thus need to skip all boundaries, distance to which is greater than the distance to exit boundary\n"
"        // Using -FLT_MAX as the distance to these boundaries will result in skipping them:\n"
"        b4IsCorrectIntersectionFlag = And( b4IsCorrectIntersectionFlag, Less( f4DistToBoundaries, (fDistToExitBoundary - 1e-4) * F4ONE ) );\n"
"        float4 f4CorrectDist   = BoolToFloat( b4IsCorrectIntersectionFlag ) * f4DistToBoundaries;\n"
"        // When working with FLT_MAX, we must make sure that the compiler does not use mad instruction, \n"
"        // which will screw things up due to precision issues. DO NOT use 1.0-Flag\n"
"        float4 f4IncorrectDist = BoolToFloat( Not(b4IsCorrectIntersectionFlag) ) * float4(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);\n"
"        f4DistToBoundaries = f4CorrectDist + f4IncorrectDist;\n"
"        \n"
"        float fFirstIntersecDist = 0.0;\n"
"        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.x);\n"
"        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.y);\n"
"        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.z);\n"
"        fFirstIntersecDist = max(fFirstIntersecDist, f4DistToBoundaries.w);\n"
"        \n"
"        // The code above is equivalent to the following lines:\n"
"        // fFirstIntersecDist = fDistToLeftBoundary   < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToLeftBoundary)   : fFirstIntersecDist;\n"
"        // fFirstIntersecDist = fDistToBottomBoundary < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToBottomBoundary) : fFirstIntersecDist;\n"
"        // fFirstIntersecDist = fDistToRightBoundary  < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToRightBoundary)  : fFirstIntersecDist;\n"
"        // fFirstIntersecDist = fDistToTopBoundary    < fDistToBoundary-1e-4 ? max(fFirstIntersecDist, fDistToTopBoundary)    : fFirstIntersecDist;\n"
"\n"
"        // Now we can compute entry point:\n"
"        f2EntryPoint = g_PPAttribs.f4LightScreenPos.xy + f2RayDir * fFirstIntersecDist;\n"
"\n"
"        // For invalid rays, coordinates are outside [-1,1]x[-1,1] area\n"
"        // and such rays will be discarded\n"
"        //\n"
"        //       g_PPAttribs.f4LightScreenPos\n"
"        //             *\n"
"        //              \\|\n"
"        //               \\-f2EntryPoint\n"
"        //               |\\\n"
"        //               | \\  f2ExitPoint \n"
"        //               |__\\/___\n"
"        //               |       |\n"
"        //               |       |\n"
"        //               |_______|\n"
"        //\n"
"    }\n"
"\n"
"    return f2EntryPoint;\n"
"}\n"
"\n"
"float4 GenerateSliceEndpointsPS(FullScreenTriangleVSOutput VSOut\n"
"                                // IMPORTANT: non-system generated pixel shader input\n"
"                                // arguments must go in the exact same order as VS outputs.\n"
"                                // Moreover, even if the shader is not using the argument,\n"
"                                // it still must be declared\n"
"                                ) : SV_Target\n"
"{\n"
"    float2 f2UV = NormalizedDeviceXYToTexUV(VSOut.f2NormalizedXY);\n"
"\n"
"    // Note that due to the rasterization rules, UV coordinates are biased by 0.5 texel size.\n"
"    //\n"
"    //      0.5     1.5     2.5     3.5\n"
"    //   |   X   |   X   |   X   |   X   |     ....       \n"
"    //   0       1       2       3       4   f2UV * TexDim\n"
"    //   X - locations where rasterization happens\n"
"    //\n"
"    // We need to remove this offset. Also clamp to [0,1] to fix fp32 precision issues\n"
"    float fEpipolarSlice = saturate(f2UV.x - 0.5 / float(g_PPAttribs.uiNumEpipolarSlices) );\n"
"\n"
"    // fEpipolarSlice now lies in the range [0, 1 - 1/NUM_EPIPOLAR_SLICES]\n"
"    // 0 defines location in exacatly left top corner, 1 - 1/NUM_EPIPOLAR_SLICES defines\n"
"    // position on the top boundary next to the top left corner\n"
"    uint uiBoundary = uint( clamp(floor( fEpipolarSlice * 4.0 ), 0.0, 3.0) );\n"
"    float fPosOnBoundary = frac( fEpipolarSlice * 4.0 );\n"
"\n"
"    bool4 b4BoundaryFlags = bool4( \n"
"            uiBoundary == 0u,\n"
"            uiBoundary == 1u,\n"
"            uiBoundary == 2u,\n"
"            uiBoundary == 3u\n"
"        );\n"
"\n"
"    // Note that in fact the outermost visible screen pixels do not lie exactly on the boundary (+1 or -1), but are biased by\n"
"    // 0.5 screen pixel size inwards. Using these adjusted boundaries improves precision and results in\n"
"    // samller number of pixels which require inscattering correction\n"
"    float4 f4OutermostScreenPixelCoords = GetOutermostScreenPixelCoords(g_PPAttribs.f4ScreenResolution);// xyzw = (left, bottom, right, top)\n"
"\n"
"    // Check if there can definitely be no correct intersection with the boundary:\n"
"    //  \n"
"    //  Light.x <= LeftBnd    Light.y <= BottomBnd     Light.x >= RightBnd     Light.y >= TopBnd    \n"
"    //                                                                                 *             \n"
"    //          ____                 ____                    ____                   __/_             \n"
"    //        .|    |               |    |                  |    |  .*             |    |            \n"
"    //      .\' |____|               |____|                  |____|.\'               |____|            \n"
"    //     *                           \\                                                               \n"
"    //                                  *                                                  \n"
"    //     Left Boundary       Bottom Boundary           Right Boundary          Top Boundary \n"
"    //\n"
"    bool4 b4IsInvalidBoundary = LessEqual( (g_PPAttribs.f4LightScreenPos.xyxy - f4OutermostScreenPixelCoords.xyzw) * float4(1,1,-1,-1),  F4ZERO );\n"
"    if( dot( BoolToFloat(b4IsInvalidBoundary), BoolToFloat(b4BoundaryFlags) ) != 0.0 )\n"
"    {\n"
"        return INVALID_EPIPOLAR_LINE;\n"
"    }\n"
"    // Additinal check above is required to eliminate false epipolar lines which can appear is shown below.\n"
"    // The reason is that we have to use some safety delta when performing check in IsValidScreenLocation() \n"
"    // function. If we do not do this, we will miss valid entry points due to precision issues.\n"
"    // As a result there could appear false entry points which fall into the safety region, but in fact lie\n"
"    // outside the screen boundary:\n"
"    //\n"
"    //   LeftBnd-Delta LeftBnd           \n"
"    //                      false epipolar line\n"
"    //          |        |  /\n"
"    //          |        | /          \n"
"    //          |        |/         X - false entry point\n"
"    //          |        *\n"
"    //          |       /|\n"
"    //          |------X-|-----------  BottomBnd\n"
"    //          |     /  |\n"
"    //          |    /   |\n"
"    //          |___/____|___________ BottomBnd-Delta\n"
"    //          \n"
"    //          \n"
"\n"
"\n"
"    //             <------\n"
"    //   +1   0,1___________0.75\n"
"    //          |     3     |\n"
"    //        | |           | A\n"
"    //        | |0         2| |\n"
"    //        V |           | |\n"
"    //   -1     |_____1_____|\n"
"    //       0.25  ------>  0.5\n"
"    //\n"
"    //         -1          +1\n"
"    //\n"
"\n"
"    //                                   Left             Bottom           Right              Top   \n"
"    float4 f4BoundaryXPos = float4(               0.0, fPosOnBoundary,                1.0, 1.0-fPosOnBoundary);\n"
"    float4 f4BoundaryYPos = float4( 1.0-fPosOnBoundary,              0.0,  fPosOnBoundary,                1.0);\n"
"    // Select the right coordinates for the boundary\n"
"    float2 f2ExitPointPosOnBnd = float2( dot(f4BoundaryXPos, BoolToFloat(b4BoundaryFlags)), dot(f4BoundaryYPos, BoolToFloat(b4BoundaryFlags)) );\n"
"    float2 f2ExitPoint = lerp(f4OutermostScreenPixelCoords.xy, f4OutermostScreenPixelCoords.zw, f2ExitPointPosOnBnd);\n"
"    // GetEpipolarLineEntryPoint() gets exit point on SHRINKED boundary\n"
"    float2 f2EntryPoint = GetEpipolarLineEntryPoint(f2ExitPoint);\n"
"    \n"
"#if OPTIMIZE_SAMPLE_LOCATIONS\n"
"    // If epipolar slice is not invisible, advance its exit point if necessary\n"
"    if( IsValidScreenLocation(f2EntryPoint, g_PPAttribs.f4ScreenResolution) )\n"
"    {\n"
"        // Compute length of the epipolar line in screen pixels:\n"
"        float fEpipolarSliceScreenLen = length( (f2ExitPoint - f2EntryPoint) * g_PPAttribs.f4ScreenResolution.xy / 2.0 );\n"
"        // If epipolar line is too short, update epipolar line exit point to provide 1:1 texel to screen pixel correspondence:\n"
"        f2ExitPoint = f2EntryPoint + (f2ExitPoint - f2EntryPoint) * max(float(g_PPAttribs.uiMaxSamplesInSlice) / fEpipolarSliceScreenLen, 1.0);\n"
"    }\n"
"#endif\n"
"\n"
"    return float4(f2EntryPoint, f2ExitPoint);\n"
"}\n"
