struct FrameVertexOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

FrameVertexOutput FrameVS(uint vertexId : SV_VertexID)
{
    float2 positions[3] = {
        float2(-1.0f, -1.0f),
        float2(-1.0f,  3.0f),
        float2( 3.0f, -1.0f)
    };
    float2 texCoords[3] = {
        float2(0.0f, 1.0f),
        float2(0.0f, -1.0f),
        float2(2.0f, 1.0f)
    };

    FrameVertexOutput output;
    output.Position = float4(positions[vertexId], 0.0f, 1.0f);
    output.TexCoord = texCoords[vertexId];
    return output;
}

Texture2D FrameTexture : register(t0);
SamplerState FrameSampler : register(s0);

float4 FramePS(FrameVertexOutput input) : SV_Target
{
    return FrameTexture.Sample(FrameSampler, input.TexCoord);
}
