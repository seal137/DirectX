//const buffer
cbuffer ConstantBuffer : register(b0)
{
	matrix MatWorld; 
	matrix MatView;
	matrix MatProjection;
}

//vertex shader
float4 VS(float4 Pos : POSITION) : SV_POSITION
{
	// Leave the coordinates unchanged
	return Pos;
}

//pixel shader
float4 PS(float4 Pos : SV_POSITION) : SV_Target
{
	return float4(1.0f, 0.5f, 0.5f, 1.0f);
}
