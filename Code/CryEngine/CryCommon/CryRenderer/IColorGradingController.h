// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SColorChartLayer
{
	int   m_texID;
	float m_blendAmount;

	SColorChartLayer()
		: m_texID(-1)
		, m_blendAmount(-1)
	{
	}

	SColorChartLayer(int texID, float blendAmount)
		: m_texID(texID)
		, m_blendAmount(blendAmount)
	{
	}

	SColorChartLayer(const SColorChartLayer& rhs)
		: m_texID(rhs.m_texID)
		, m_blendAmount(rhs.m_blendAmount)
	{
	}
};

struct IColorGradingController
{
	// <interfuscator:shuffle>
	virtual ~IColorGradingController(){}
	virtual int  LoadColorChart(const char* pChartFilePath) const = 0;
	virtual void UnloadColorChart(int texID) const = 0;

	virtual void SetLayers(const SColorChartLayer* pLayers, uint32 numLayers) = 0;
	// </interfuscator:shuffle>
};
