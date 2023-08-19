#define D3D_SHADER_REQUIRES_RESOURCE_HEAP_INDEXING  0x02000000

struct Params
{
    uint videoFrameIndex;
    uint brightness;
};

ConstantBuffer<Params> params : register(b0);
SamplerState videoframeSampler : register(s0);

float4 main(in noperspective float4 Position : SV_Position, in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    Texture2D<float4> texture = ResourceDescriptorHeap[params.videoFrameIndex];
    float4 output = texture.Sample(videoframeSampler, UV);
    output.rgb *= params.brightness / 100;
    output.rgb = saturate(output.rgb);
    return output;
}