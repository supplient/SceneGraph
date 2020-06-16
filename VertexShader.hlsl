cbuffer cbPerObject
{
    float4x4 mvpMat;
};

float4 main( float4 pos : POS ) : SV_POSITION
{
	return pos;
}