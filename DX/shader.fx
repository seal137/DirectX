//const buffer
cbuffer ConstantBuffer : register(b0) // b0 - индекс буфера
{
	matrix World;
	matrix View;
	matrix Projection;
}

//vertex shader output format
struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 Color : COLOR0;
};

//vertex shader
VS_OUTPUT VS(float4 Pos : POSITION, float4 Color : COLOR)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	// Transform vertex position
	output.Pos = mul(Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Color = Color;
	return output;
}
//pixel shader
float4 PS(VS_OUTPUT input) : SV_Target
{
	return input.Color;
}
