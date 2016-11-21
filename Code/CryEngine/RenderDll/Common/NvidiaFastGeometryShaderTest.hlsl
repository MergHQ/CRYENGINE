
// This is a test shader for the core NVIDIA Multi-Projection feature: 
// pass-through geometry shaders, or FastGS. There is no function in NVAPI
// to explicitly query for FastGS support, so this dummy shader is 
// created just to see if the create call succeeds.

// Compiled to NvidiaFastGeometryShaderTest.h using the following command line:
//    fxc.exe /T gs_5_0 /Fh NvidiaFastGeometryShaderTest.h NvidiaFastGeometryShaderTest.hlsl 

struct VSOutput {
	float4 pos : SV_Position;	
};

struct GSOutput {
	VSOutput Passthrough;
	uint ViewportMask : SV_ViewportArrayIndex;
};

[maxvertexcount(1)]
void main(triangle VSOutput IN[3], inout TriangleStream<GSOutput> OUT)
{
	GSOutput Vertex;
	Vertex.Passthrough = IN[0];
	Vertex.ViewportMask = 0xf;
	OUT.Append(Vertex);
}
