// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2019/05/02)

Texture3D<float4> volumeTexture;
SamplerState volumeSampler;

struct PS_INPUT
{
    float3 vertexTCoord : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 pixelColor : SV_TARGET0;
};

PS_OUTPUT PSMain(PS_INPUT input)
{
    PS_OUTPUT output;
    float4 color = volumeTexture.Sample(volumeSampler, input.vertexTCoord);
    output.pixelColor = float4(color.w * color.rgb, 0.5f * color.w);
    return output;
};