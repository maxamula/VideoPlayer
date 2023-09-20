struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD;
};

VSOutput main(uint VertexIdx : SV_VertexID)
{
    VSOutput output;
    float2 pos;

    float2 vertices[6] =
    {
        float2(-1.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(-1.0f, -1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, -1.0f),
        float2(-1.0f, -1.0f)
    };

    pos = vertices[VertexIdx];

    output.Position = float4(pos, 0, 1);
    output.UV.x = (pos.x + 1.0f) * 0.5f;
    output.UV.y = 1 - ((pos.y + 1.0f) * 0.5f);

    return output;
}