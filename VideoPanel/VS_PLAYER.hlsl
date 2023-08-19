struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD;
};

VSOutput main(uint VertexIdx : SV_VertexID)
{
    VSOutput output;
    float2 pos;

    // Координаты вершин для полноэкранного прямоугольника
    float2 vertices[6] =
    {
        float2(-1.0f, 1.0f), // Верхний левый угол
        float2(1.0f, 1.0f), // Верхний правый угол
        float2(-1.0f, -1.0f), // Нижний левый угол
        float2(1.0f, 1.0f), // Верхний правый угол
        float2(1.0f, -1.0f), // Нижний правый угол
        float2(-1.0f, -1.0f) // Нижний левый угол
    };

    pos = vertices[VertexIdx];

    output.Position = float4(pos, 0, 1);
    output.UV = (pos + 1.0f) * 0.5f;

    return output;
}