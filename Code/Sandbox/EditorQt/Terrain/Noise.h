// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CDynamicArray2D;

// Parameter structures
struct SNoiseParams
{
	unsigned int iWidth;
	unsigned int iHeight;
	unsigned int iCover;
	unsigned int iPasses;
	float        fFrequencyStep;
	float        fFrequency;
	unsigned int iSmoothness;
	unsigned int iRandom;
	float        fFade;
	float        iSharpness;
	bool         bBlueSky;
	bool         bValid; // Used internally to verify serialized data, no
	                     // need to set it from outside the class
};

// Basis matrix for spline interpolation
#define CR00 -0.5f
#define CR01 1.5f
#define CR02 -1.5f
#define CR03 0.5f
#define CR10 1.0f
#define CR11 -2.5f
#define CR12 2.0f
#define CR13 -0.5f
#define CR20 -0.5f
#define CR21 0.0f
#define CR22 0.5f
#define CR23 0.0f
#define CR30 0.0f
#define CR31 1.0f
#define CR32 0.0f
#define CR33 0.0f

class CNoise
{
public:
	void  FracSynthPass(CDynamicArray2D* hBuf, float freq, float zscale, int xres, int zres, BOOL bLoop);
	float Spline(float x, /*int nknots,*/ float* knot);
};
