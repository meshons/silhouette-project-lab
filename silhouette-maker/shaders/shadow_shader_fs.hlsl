Texture2D diffTexture;
SamplerState SampleType;

float4 main(float4 pos : SV_POSITION, float2 texcoord : TEXCOORD) : SV_TARGET
{
	float4 textureColor = diffTexture.Sample(SampleType, texcoord);

    textureColor = float4(0.0, 0.0, 0.0, 1.0);
	return textureColor;
}