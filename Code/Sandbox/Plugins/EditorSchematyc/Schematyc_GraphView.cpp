// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_GraphView.h"

#include <CrySerialization/Color.h>
#include <CrySystem/ITimer.h>

#include "Schematyc_CustomMessageBox.h"
#include "Schematyc_GdiplusUtils.h"
#include "Schematyc_GraphViewFormatter.h"
#include "Schematyc_PluginUtils.h"

#pragma warning(disable: 4355)

namespace Schematyc
{
const float CDefaultGraphViewPainter::NODE_WIDTH_MIN = 100.0f;
const float CDefaultGraphViewPainter::NODE_HEIGHT_MIN = 50.0f;
const float CDefaultGraphViewPainter::NODE_BEVEL = 15.0f;
const BYTE CDefaultGraphViewPainter::NODE_HEADER_ALPHA = 200;
const BYTE CDefaultGraphViewPainter::NODE_HEADER_ALPHA_DISABLED = 80;
const float CDefaultGraphViewPainter::NODE_HEADER_TEXT_BORDER_X = 8.0f;
const float CDefaultGraphViewPainter::NODE_HEADER_TEXT_BORDER_Y = 2.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_HEADER_TEXT_COLOR = Gdiplus::Color(60, 60, 60);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_ERROR_FILL_COLOR = Gdiplus::Color(210, 66, 66);
const char* CDefaultGraphViewPainter::NODE_ERROR_TEXT = "Error!";
const float CDefaultGraphViewPainter::NODE_ERROR_TEXT_BORDER_X = 8.0f;
const float CDefaultGraphViewPainter::NODE_ERROR_TEXT_BORDER_Y = 2.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_ERROR_TEXT_COLOR = Gdiplus::Color(60, 60, 60);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_WARNING_FILL_COLOR = Gdiplus::Color(250, 232, 12);
const char* CDefaultGraphViewPainter::NODE_WARNING_TEXT = "Warning!";
const float CDefaultGraphViewPainter::NODE_WARNING_TEXT_BORDER_X = 8.0f;
const float CDefaultGraphViewPainter::NODE_WARNING_TEXT_BORDER_Y = 2.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_WARNING_TEXT_COLOR = Gdiplus::Color(60, 60, 60);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_BODY_FILL_COLOR = Gdiplus::Color(200, 40, 40, 40);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_BODY_FILL_COLOR_DISABLED = Gdiplus::Color(80, 40, 40, 40);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_BODY_OUTLINE_COLOR = Gdiplus::Color(200, 40, 40, 40);
const Gdiplus::Color CDefaultGraphViewPainter::NODE_BODY_OUTLINE_COLOR_HIGHLIGHT = Gdiplus::Color(250, 232, 12);
const float CDefaultGraphViewPainter::NODE_BODY_OUTLINE_WIDTH = 1.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_OUPUT_HORZ_SPACING = 40.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_OUPUT_VERT_SPACING = 5.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_ICON_BORDER = 5.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_ICON_WIDTH = 11.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_ICON_HEIGHT = 11.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_INPUT_ICON_OUTLINE_COLOR = Gdiplus::Color(200, 20, 20, 20);
const float CDefaultGraphViewPainter::NODE_INPUT_ICON_OUTLINE_WIDTH = 1.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_NAME_BORDER = 2.0f;
const float CDefaultGraphViewPainter::NODE_INPUT_NAME_WIDTH_MAX = 400.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_INPUT_NAME_COLOR = Gdiplus::Color(240, 240, 240);
const float CDefaultGraphViewPainter::NODE_OUTPUT_ICON_BORDER = 5.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_ICON_WIDTH = 11.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_ICON_HEIGHT = 11.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_OUTPUT_ICON_OUTLINE_COLOR = Gdiplus::Color(200, 20, 20, 20);
const float CDefaultGraphViewPainter::NODE_OUTPUT_ICON_OUTLINE_WIDTH = 1.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_NAME_BORDER = 2.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_NAME_WIDTH_MAX = 400.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_OUTPUT_NAME_COLOR = Gdiplus::Color(240, 240, 240);
const float CDefaultGraphViewPainter::NODE_OUTPUT_SPACER_OFFSET = 3.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_SPACER_WIDTH = 80.0f;
const float CDefaultGraphViewPainter::NODE_OUTPUT_SPACER_LINE_WIDTH = 1.0f;
const Gdiplus::Color CDefaultGraphViewPainter::NODE_OUTPUT_SPACER_COLOR = Gdiplus::Color(140, 120, 120, 120);
const float CDefaultGraphViewPainter::LINK_WIDTH = 2.0f;
const float CDefaultGraphViewPainter::LINK_CURVE_OFFSET = 45.0f;
const float CDefaultGraphViewPainter::LINK_DODGE_X = 15.0f;
const float CDefaultGraphViewPainter::LINK_DODGE_Y = 15.0f;
const float CDefaultGraphViewPainter::ALPHA_HIGHLIGHT_MIN = 0.3f;
const float CDefaultGraphViewPainter::ALPHA_HIGHLIGHT_MAX = 1.0f;
const float CDefaultGraphViewPainter::ALPHA_HIGHLIGHT_SPEED = 0.8f;
const Gdiplus::Color CDefaultGraphViewPainter::SELECTION_FILL_COLOR = Gdiplus::Color(100, 0, 142, 212);
const Gdiplus::Color CDefaultGraphViewPainter::SELECTION_OUTLINE_COLOR = Gdiplus::Color(0, 162, 232);

const float CGraphView::MIN_ZOOM = 0.28f;
const float CGraphView::MAX_ZOOM = 1.6f;
const float CGraphView::DELTA_ZOOM = 0.001f;
const float CGraphView::DEFAULT_NODE_SPACING = 40.0f;

const char* g_szGraphNodeClipboardPrefix = "[schematyc_graph_node] ";

namespace Assets
{
static const wchar_t* FONT_NAME = L"Tahoma";
static const float FONT_SIZE = 8.0f;
static const Gdiplus::FontStyle FONT_STYLE = Gdiplus::FontStyleBold;

static inline const Gdiplus::Font& GetFont()
{
	static const Gdiplus::Font font(FONT_NAME, FONT_SIZE, FONT_STYLE);
	return font;
}
}

static const UINT UPDATE_TIMER_EVENT_ID = 1;

SGraphViewFormatSettings::SGraphViewFormatSettings()
	: horzSpacing(40.0f)
	, vertSpacing(10.0f)
	, bSnapToGrid(true)
{}

void SGraphViewFormatSettings::Serialize(Serialization::IArchive& archive)
{
	archive(horzSpacing, "horzSpacing", "Horizontal Spacing");
	archive(vertSpacing, "vertSpacing", "Vertical Spacing");
	archive(bSnapToGrid, "bSnapToGrid", "Snap To Grid");
}

void SGraphViewSettings::Serialize(Serialization::IArchive& archive)
{
	archive(format, "format", "Format");
}

CGraphViewNode::CGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos)
	: m_grid(grid)
	, m_enabled(false)
	, m_selected(false)
	, m_paintRect(grid.SnapPos(pos), Gdiplus::SizeF())
{}

CGraphViewNode::~CGraphViewNode() {}

const SGraphViewGrid& CGraphViewNode::GetGrid() const
{
	return m_grid;
}

void CGraphViewNode::SetPos(Gdiplus::PointF pos, bool bSnapToGrid)
{
	if (bSnapToGrid)
	{
		pos = m_grid.SnapPos(pos);
	}
	m_paintRect.X = pos.X;
	m_paintRect.Y = pos.Y;
	OnMove(m_paintRect);
}

Gdiplus::PointF CGraphViewNode::GetPos() const
{
	return Gdiplus::PointF(m_paintRect.X, m_paintRect.Y);
}

void CGraphViewNode::SetPaintRect(Gdiplus::RectF paintRect)
{
	m_paintRect = paintRect;
	OnMove(m_paintRect);
}

Gdiplus::RectF CGraphViewNode::GetPaintRect() const
{
	return m_paintRect;
}

void CGraphViewNode::Enable(bool enable)
{
	m_enabled = enable;
}

bool CGraphViewNode::IsEnabled() const
{
	return m_enabled;
}

void CGraphViewNode::Select(bool select)
{
	m_selected = select;
}

bool CGraphViewNode::IsSelected() const
{
	return m_selected;
}

void CGraphViewNode::SetStatusFlags(const GraphViewNodeStatusFlags& statusFlags)
{
	m_statusFlags = statusFlags;
}

GraphViewNodeStatusFlags CGraphViewNode::GetStatusFlags() const
{
	return m_statusFlags;
}

uint32 CGraphViewNode::FindInput(Gdiplus::PointF pos) const
{
	// #SchematycTODO : Do we need to double check that the paint rects are valid? Maybe we should just reference the ports by name to be safe?
	for (TRectVector::const_iterator iBeginInputPaintRect = m_inputPaintRects.begin(), iEndInputPaintRect = m_inputPaintRects.end(), iInputPaintRect = iBeginInputPaintRect; iInputPaintRect != iEndInputPaintRect; ++iInputPaintRect)
	{
		if (iInputPaintRect->Contains(pos))
		{
			return static_cast<uint32>(iInputPaintRect - iBeginInputPaintRect);
		}
	}
	return InvalidIdx;
}

void CGraphViewNode::SetInputPaintRect(uint32 inputIdx, Gdiplus::RectF paintRect)
{
	if (inputIdx >= m_inputPaintRects.size())
	{
		m_inputPaintRects.resize(inputIdx + 1);
	}
	m_inputPaintRects[inputIdx] = paintRect;
}

Gdiplus::RectF CGraphViewNode::GetInputPaintRect(uint32 inputIdx) const
{
	return inputIdx < m_inputPaintRects.size() ? m_inputPaintRects[inputIdx] : Gdiplus::RectF();
}

Gdiplus::PointF CGraphViewNode::GetInputLinkPoint(uint32 inputIdx) const
{
	if (inputIdx < m_inputPaintRects.size())
	{
		const Gdiplus::RectF paintRect = m_inputPaintRects[inputIdx];
		return Gdiplus::PointF(paintRect.GetLeft(), paintRect.Y + (paintRect.Height / 2.0f));
	}
	else
	{
		return Gdiplus::PointF();
	}
}

uint32 CGraphViewNode::FindOutput(Gdiplus::PointF pos) const
{
	// #SchematycTODO : Do we need to double check that the paint rects are valid? Maybe we should just reference the ports by name to be safe?
	for (TRectVector::const_iterator iBeginOutputPaintRect = m_outputPaintRects.begin(), iEndOutputPaintRect = m_outputPaintRects.end(), iOutputPaintRect = iBeginOutputPaintRect; iOutputPaintRect != iEndOutputPaintRect; ++iOutputPaintRect)
	{
		if (iOutputPaintRect->Contains(pos))
		{
			return static_cast<uint32>(iOutputPaintRect - iBeginOutputPaintRect);
		}
	}
	return InvalidIdx;
}

void CGraphViewNode::SetOutputPaintRect(uint32 outputIdx, Gdiplus::RectF paintRect)
{
	if (outputIdx >= m_outputPaintRects.size())
	{
		m_outputPaintRects.resize(outputIdx + 1);
	}
	m_outputPaintRects[outputIdx] = paintRect;
}

Gdiplus::RectF CGraphViewNode::GetOutputPaintRect(uint32 outputIdx) const
{
	return outputIdx < m_outputPaintRects.size() ? m_outputPaintRects[outputIdx] : Gdiplus::RectF();
}

Gdiplus::PointF CGraphViewNode::GetOutputLinkPoint(uint32 outputIdx) const
{
	if (outputIdx < m_outputPaintRects.size())
	{
		const Gdiplus::RectF paintRect = m_outputPaintRects[outputIdx];
		return Gdiplus::PointF(paintRect.GetRight(), paintRect.Y + (paintRect.Height / 2.0f));
	}
	else
	{
		return Gdiplus::PointF();
	}
}

const char* CGraphViewNode::GetName() const
{
	return "";
}

Gdiplus::Color CGraphViewNode::GetHeaderColor() const
{
	return Gdiplus::Color();
}

bool CGraphViewNode::IsRemoveable() const
{
	return true;
}

uint32 CGraphViewNode::GetInputCount() const
{
	return 0;
}

uint32 CGraphViewNode::FindInputById(const CGraphPortId& id) const
{
	return InvalidIdx;
}

CGraphPortId CGraphViewNode::GetInputId(uint32 inputIdx) const
{
	return CGraphPortId();
}

const char* CGraphViewNode::GetInputName(uint32 inputIdx) const
{
	return "";
}

ScriptGraphPortFlags CGraphViewNode::GetInputFlags(uint32 inputIdx) const
{
	return EScriptGraphPortFlags::None;
}

Gdiplus::Color CGraphViewNode::GetInputColor(uint32 inputIdx) const
{
	return Gdiplus::Color();
}

uint32 CGraphViewNode::GetOutputCount() const
{
	return 0;
}

uint32 CGraphViewNode::FindOutputById(const CGraphPortId& id) const
{
	return InvalidIdx;
}

CGraphPortId CGraphViewNode::GetOutputId(uint32 outputIdx) const
{
	return CGraphPortId();
}

const char* CGraphViewNode::GetOutputName(uint32 outputIdx) const
{
	return "";
}

ScriptGraphPortFlags CGraphViewNode::GetOutputFlags(uint32 outputIdx) const
{
	return EScriptGraphPortFlags::None;
}

Gdiplus::Color CGraphViewNode::GetOutputColor(uint32 inputIdx) const
{
	return Gdiplus::Color();
}

void CGraphViewNode::CopyToXml(IString& output) const {}

void CGraphViewNode::OnMove(Gdiplus::RectF paintRect) {}

SGraphViewLink::SGraphViewLink(CGraphView* _pGraphView, const CGraphViewNodeWeakPtr& _pSrcNode, const CGraphPortId& _srcOutputId, const CGraphViewNodeWeakPtr& _pDstNode, const CGraphPortId& _dstInputId)
	: pGraphView(_pGraphView)
	, pSrcNode(_pSrcNode)
	, srcOutputId(_srcOutputId)
	, pDstNode(_pDstNode)
	, dstInputId(_dstInputId)
	, bSelected(false)
{}

SDefaultGraphViewPainterSettings::SDefaultGraphViewPainterSettings()
	: backgroundColor(60, 60, 60, 255)
{
	gridLineColors[0] = ColorB(68, 68, 68, 255);
	gridLineColors[1] = ColorB(48, 48, 48, 255);
}

void SDefaultGraphViewPainterSettings::Serialize(Serialization::IArchive& archive)
{
	archive(backgroundColor, "backgroundColor", "Background Color");
	archive(gridLineColors, "gridLineColors", "Grid Line Colors");
}

Serialization::SStruct CDefaultGraphViewPainter::GetSettings()
{
	return Serialization::SStruct(m_settings);
}

Gdiplus::Color CDefaultGraphViewPainter::GetBackgroundColor() const
{
	return GdiplusUtils::CreateColor(m_settings.backgroundColor);
}

void CDefaultGraphViewPainter::PaintGrid(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid) const
{
	const int left = static_cast<int>(grid.bounds.X);
	const int right = static_cast<int>(grid.bounds.X + grid.bounds.Width);
	const int top = static_cast<int>(grid.bounds.Y);
	const int bottom = static_cast<int>(grid.bounds.Y + grid.bounds.Height);
	const int horzSpacing = static_cast<int>(grid.spacing.X);
	const int vertSpacing = static_cast<int>(grid.spacing.Y);

	Gdiplus::Pen pen0(GdiplusUtils::CreateColor(m_settings.gridLineColors[0]), 1.0f);
	Gdiplus::Pen pen1(GdiplusUtils::CreateColor(m_settings.gridLineColors[1]), 1.0f);

	for (int x = left + horzSpacing; x < right; x += static_cast<int>(grid.spacing.X))
	{
		if (x % (horzSpacing * 10))
		{
			graphics.DrawLine(&pen0, x, top, x, bottom);
		}
		else
		{
			graphics.DrawLine(&pen1, x, top, x, bottom);
		}
	}

	for (int y = top + vertSpacing; y < bottom; y += static_cast<int>(grid.spacing.Y))
	{
		if (y % (vertSpacing * 10))
		{
			graphics.DrawLine(&pen0, left, y, right, y);
		}
		else
		{
			graphics.DrawLine(&pen1, left, y, right, y);
		}
	}
}

void CDefaultGraphViewPainter::UpdateNodePaintRect(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const
{
	// Calculate node header and status sizes.
	const Gdiplus::PointF headerSize = CalculateNodeHeaderSize(graphics, node);
	const Gdiplus::PointF statusSize = CalculateNodeStatusSize(graphics, node);
	const Gdiplus::PointF headerAndStatusSize(std::max(headerSize.X, statusSize.X), headerSize.Y + statusSize.Y);
	// Calculate total size of all node inputs.
	Gdiplus::PointF totalInputSize;
	const uint32 inputCount = node.GetInputCount();
	for (uint32 inputIdx = 0; inputIdx < inputCount; ++inputIdx)
	{
		const Gdiplus::PointF inputSize = CalculateNodeInputSize(graphics, node.GetInputName(inputIdx));
		if (inputSize.X > totalInputSize.X)
		{
			totalInputSize.X = inputSize.X;
		}
		totalInputSize.Y += inputSize.Y + NODE_INPUT_OUPUT_VERT_SPACING;
	}
	totalInputSize.Y += NODE_INPUT_OUPUT_VERT_SPACING;
	// Calculate total size of all node outputs.
	Gdiplus::PointF totalOutputSize;
	const uint32 outputCount = node.GetOutputCount();
	for (uint32 outputIdx = 0; outputIdx < outputCount; ++outputIdx)
	{
		const Gdiplus::PointF output = CalculateNodeOutputSize(graphics, node.GetOutputName(outputIdx));
		if (output.X > totalOutputSize.X)
		{
			totalOutputSize.X = output.X;
		}
		totalOutputSize.Y += output.Y + NODE_INPUT_OUPUT_VERT_SPACING;
	}
	totalOutputSize.Y += NODE_INPUT_OUPUT_VERT_SPACING;
	// Update node paint rect.
	Gdiplus::SizeF paintSize(std::max(headerAndStatusSize.X, totalInputSize.X + NODE_INPUT_OUPUT_HORZ_SPACING + totalOutputSize.X), headerAndStatusSize.Y + std::max(totalInputSize.Y, totalOutputSize.Y));
	paintSize.Width = std::max(paintSize.Width, NODE_WIDTH_MIN);
	paintSize.Height = std::max(paintSize.Height, NODE_HEIGHT_MIN);
	const Gdiplus::RectF paintRect(node.GetPos(), grid.SnapSize(paintSize));
	node.SetPaintRect(paintRect);
	// Update node input paint rects.
	Gdiplus::PointF inputPos(paintRect.X, paintRect.Y + headerAndStatusSize.Y + NODE_INPUT_OUPUT_VERT_SPACING);
	for (uint32 inputIdx = 0; inputIdx < inputCount; ++inputIdx)
	{
		UpdateNodeInputPaintRect(graphics, node, inputIdx, inputPos, totalInputSize.X);
		inputPos.Y += node.GetInputPaintRect(inputIdx).Height + NODE_INPUT_OUPUT_VERT_SPACING;
	}
	// Update node output paint rects.
	Gdiplus::PointF outputPos(paintRect.X + paintRect.Width - totalOutputSize.X, paintRect.Y + headerAndStatusSize.Y + NODE_INPUT_OUPUT_VERT_SPACING);
	for (uint32 outputIdx = 0; outputIdx < outputCount; ++outputIdx)
	{
		UpdateNodeOutputPaintRect(graphics, node, outputIdx, outputPos, totalOutputSize.X);
		outputPos.Y += node.GetOutputPaintRect(outputIdx).Height + NODE_INPUT_OUPUT_VERT_SPACING;
	}
}

void CDefaultGraphViewPainter::PaintNode(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const
{
	// Paint node body, header and outline
	const Gdiplus::RectF paintRect = node.GetPaintRect();
	const Gdiplus::PointF headerSize = CalculateNodeHeaderSize(graphics, node);
	const Gdiplus::PointF statusSize = CalculateNodeStatusSize(graphics, node);
	PaintNodeBody(graphics, node, Gdiplus::RectF(paintRect.X, paintRect.Y + headerSize.Y + statusSize.Y, paintRect.Width, paintRect.Height - headerSize.Y - statusSize.Y));
	PaintNodeHeader(graphics, node, Gdiplus::RectF(paintRect.X, paintRect.Y, paintRect.Width, headerSize.Y));
	PaintNodeStatus(graphics, node, Gdiplus::RectF(paintRect.X, paintRect.Y + headerSize.Y, paintRect.Width, headerSize.Y));
	PaintNodeOutline(graphics, node, paintRect);
	// Paint node inputs.
	for (uint32 inputIdx = 0, inputCount = node.GetInputCount(); inputIdx < inputCount; ++inputIdx)
	{
		PaintNodeInput(graphics, node, inputIdx);
	}
	// Paint node outputs.
	for (uint32 outputIdx = 0, outputCount = node.GetOutputCount(); outputIdx < outputCount; ++outputIdx)
	{
		PaintNodeOutput(graphics, node, outputIdx);
	}
}

Gdiplus::RectF CDefaultGraphViewPainter::PaintLink(Gdiplus::Graphics& graphics, Gdiplus::PointF startPoint, Gdiplus::PointF endPoint, Gdiplus::Color color, bool highlight, float scale) const
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Part of temporary fix for slow graphs with lots of links, until we can optimize the intersection tests properly.
	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
	{
		scale *= 1.75f;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Animate color?
	if (highlight)
	{
		const float time = gEnv->pTimer->GetAsyncTime().GetSeconds();
		const float sinA = sin_tpl(time * ALPHA_HIGHLIGHT_SPEED * gf_PI2);
		const float alpha = ALPHA_HIGHLIGHT_MIN + clamp_tpl(ALPHA_HIGHLIGHT_MIN + (sinA * ((ALPHA_HIGHLIGHT_MAX - ALPHA_HIGHLIGHT_MIN) * 0.5f)), 0.0f, 1.0f);
		color = Gdiplus::Color::MakeARGB(static_cast<BYTE>(alpha * 255.0f), color.GetRed(), color.GetGreen(), color.GetBlue());
	}
	// Calculate control points.
	const bool leftToRight = startPoint.X < endPoint.X;
	const float verticalSign = startPoint.Y < endPoint.Y ? 1.0f : -1.0f;
	Gdiplus::PointF midPoint((startPoint.X + endPoint.X) / 2.0f, (startPoint.Y + endPoint.Y) / 2.0f);
	const uint32 controlPointCount = 8;
	Gdiplus::PointF controlPoints[controlPointCount];
	controlPoints[0] = startPoint;
	{
		const float offset = fabs_tpl(midPoint.X - startPoint.X) * 0.5f;
		controlPoints[1].X = startPoint.X + offset;
		if (leftToRight)
		{
			controlPoints[1].Y = startPoint.Y;
		}
		else
		{
			const float extent = fabs_tpl(midPoint.Y - startPoint.Y) * 0.95f;
			controlPoints[1].Y = startPoint.Y + clamp_tpl(offset * 0.5f * verticalSign, -extent, extent);
		}
	}
	controlPoints[2].X = Lerp(controlPoints[1].X, midPoint.X, 0.75f);
	controlPoints[2].Y = Lerp(controlPoints[1].Y, midPoint.Y, 0.75f);
	controlPoints[3] = midPoint;
	controlPoints[7] = endPoint;
	{
		const float offset = fabs_tpl(midPoint.X - endPoint.X) * 0.5f;
		controlPoints[6].X = endPoint.X - offset;
		if (leftToRight)
		{
			controlPoints[6].Y = endPoint.Y;
		}
		else
		{
			const float extent = fabs_tpl(midPoint.Y - endPoint.Y) * 0.95f;
			controlPoints[6].Y = endPoint.Y - clamp_tpl(offset * 0.5f * verticalSign, -extent, extent);
		}
	}
	controlPoints[5].X = Lerp(controlPoints[6].X, midPoint.X, 0.75f);
	controlPoints[5].Y = Lerp(controlPoints[6].Y, midPoint.Y, 0.75f);
	controlPoints[4] = midPoint;
	// Catch weird glitch when control points are close to parallel on y axis.
	for (uint32 i = 1; i < controlPointCount; ++i)
	{
		if (fabs_tpl(controlPoints[i].Y - controlPoints[0].Y) <= 1.0f)
		{
			controlPoints[i].Y = controlPoints[0].Y;
		}
	}
	// Draw beziers.
	const float width = LINK_WIDTH * scale;
	Gdiplus::Pen pen(color, width);
	graphics.DrawBezier(&pen, controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3]);
	graphics.DrawBezier(&pen, controlPoints[4], controlPoints[5], controlPoints[6], controlPoints[7]);
#if FALSE
	// Debug draw control points.
	for (uint32 i = 0; i < controlPointCount; ++i)
	{
		graphics.DrawEllipse(&pen, Gdiplus::RectF(controlPoints[i].X - 2.0f, controlPoints[i].Y - 2.0f, 4.0f, 4.0f));
	}
#endif
	// Calculate paint rectangle.
	// N.B. Technically this not how you calculate the bounds of a bezier, but it's sufficient for our purposes.
	Gdiplus::PointF pmin = controlPoints[0];
	Gdiplus::PointF pmax = controlPoints[0];
	for (uint32 i = 1; i < controlPointCount; ++i)
	{
		if (controlPoints[i].X < pmin.X)
		{
			pmin.X = controlPoints[i].X;
		}
		if (controlPoints[i].Y < pmin.Y)
		{
			pmin.Y = controlPoints[i].Y;
		}
		if (controlPoints[i].X > pmax.X)
		{
			pmax.X = controlPoints[i].X;
		}
		if (controlPoints[i].Y > pmax.Y)
		{
			pmax.Y = controlPoints[i].Y;
		}
	}
	return Gdiplus::RectF(pmin.X - width, pmin.Y - width, (pmax.X - pmin.X) + (width * 2.0f), (pmax.Y - pmin.Y) + (width * 2.0f));
}

void CDefaultGraphViewPainter::PaintSelectionRect(Gdiplus::Graphics& graphics, Gdiplus::RectF rect) const
{
	Gdiplus::SolidBrush brush(SELECTION_FILL_COLOR);
	graphics.FillRectangle(&brush, rect);

	Gdiplus::Pen pen(SELECTION_OUTLINE_COLOR, 1.0f);
	graphics.DrawRectangle(&pen, rect);
}

Gdiplus::PointF CDefaultGraphViewPainter::CalculateNodeHeaderSize(Gdiplus::Graphics& graphics, const CGraphViewNode& node) const
{
	CStringW headerText(node.GetName());
	if (headerText.IsEmpty())
	{
		headerText = " ";
	}
	Gdiplus::RectF headerTextRect;
	graphics.MeasureString(headerText, headerText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &headerTextRect);
	return Gdiplus::PointF(NODE_HEADER_TEXT_BORDER_X + headerTextRect.Width + NODE_HEADER_TEXT_BORDER_X, NODE_HEADER_TEXT_BORDER_Y + headerTextRect.Height + NODE_HEADER_TEXT_BORDER_Y);
}

Gdiplus::PointF CDefaultGraphViewPainter::CalculateNodeStatusSize(Gdiplus::Graphics& graphics, const CGraphViewNode& node) const
{
	Gdiplus::RectF statusTextRect;
	const GraphViewNodeStatusFlags statusFlags = node.GetStatusFlags();
	if (statusFlags.Check(EGraphViewNodeStatusFlags::ContainsErrors))
	{
		const CStringW statusText(NODE_ERROR_TEXT);
		graphics.MeasureString(statusText, statusText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &statusTextRect);
		statusTextRect.X += NODE_ERROR_TEXT_BORDER_X + NODE_ERROR_TEXT_BORDER_X;
		statusTextRect.Y += NODE_ERROR_TEXT_BORDER_Y + NODE_ERROR_TEXT_BORDER_Y;
	}
	else if (statusFlags.Check(EGraphViewNodeStatusFlags::ContainsWarnings))
	{
		const CStringW statusText(NODE_WARNING_TEXT);
		graphics.MeasureString(statusText, statusText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &statusTextRect);
		statusTextRect.X += NODE_WARNING_TEXT_BORDER_X + NODE_WARNING_TEXT_BORDER_X;
		statusTextRect.Y += NODE_WARNING_TEXT_BORDER_Y + NODE_WARNING_TEXT_BORDER_Y;
	}
	return Gdiplus::PointF(statusTextRect.Width, statusTextRect.Height);
}

Gdiplus::PointF CDefaultGraphViewPainter::CalculateNodeInputSize(Gdiplus::Graphics& graphics, const char* szName) const
{
	const CStringW stringW(szName ? szName : "");
	Gdiplus::RectF nameRect;
	graphics.MeasureString(stringW, stringW.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &nameRect);

	const Gdiplus::PointF iconSize(NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_WIDTH + NODE_INPUT_ICON_BORDER, NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_HEIGHT + NODE_INPUT_ICON_BORDER);
	const Gdiplus::PointF nameSize(NODE_INPUT_NAME_BORDER + nameRect.Width + NODE_INPUT_NAME_BORDER, NODE_INPUT_NAME_BORDER + nameRect.Height + NODE_INPUT_NAME_BORDER);
	return Gdiplus::PointF(iconSize.X + nameSize.X, std::max(iconSize.Y, nameSize.Y));
}

Gdiplus::PointF CDefaultGraphViewPainter::CalculateNodeOutputSize(Gdiplus::Graphics& graphics, const char* szName) const
{
	const CStringW stringW(szName ? szName : "");
	Gdiplus::RectF nameRect;
	graphics.MeasureString(stringW, stringW.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &nameRect);

	const Gdiplus::PointF iconSize(NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_WIDTH + NODE_OUTPUT_ICON_BORDER, NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_HEIGHT + NODE_OUTPUT_ICON_BORDER);
	const Gdiplus::PointF nameSize(NODE_OUTPUT_NAME_BORDER + nameRect.Width + NODE_OUTPUT_NAME_BORDER, NODE_OUTPUT_NAME_BORDER + nameRect.Height + NODE_OUTPUT_NAME_BORDER);
	return Gdiplus::PointF(iconSize.X + nameSize.X, std::max(iconSize.Y, nameSize.Y));
}

void CDefaultGraphViewPainter::PaintNodeBody(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const
{
	const float bevels[4] = { 0.0f, 0.0f, NODE_BEVEL, NODE_BEVEL };
	const Gdiplus::SolidBrush brush(node.IsEnabled() ? NODE_BODY_FILL_COLOR : NODE_BODY_FILL_COLOR_DISABLED);
	GdiplusUtils::FillBeveledRect(graphics, paintRect, bevels, brush);
}

void CDefaultGraphViewPainter::PaintNodeHeader(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const
{
	const Gdiplus::Color headerColor = node.GetHeaderColor();
	const BYTE alpha = node.IsEnabled() ? NODE_HEADER_ALPHA : NODE_HEADER_ALPHA_DISABLED;
	const Gdiplus::Color fillColorLeft = Gdiplus::Color(alpha, headerColor.GetRed(), headerColor.GetGreen(), headerColor.GetBlue());
	const Gdiplus::Color fillColorRight = GdiplusUtils::LerpColor(fillColorLeft, Gdiplus::Color(alpha, 255, 255, 255), 0.6f);

	const float bevels[4] = { NODE_BEVEL, NODE_BEVEL, 0.0f, 0.0f };
	const Gdiplus::LinearGradientBrush brush(paintRect, fillColorLeft, fillColorRight, Gdiplus::LinearGradientModeForwardDiagonal);
	GdiplusUtils::FillBeveledRect(graphics, paintRect, bevels, brush);

	const CStringW headerText(node.GetName());
	const Gdiplus::SolidBrush textBrush(NODE_HEADER_TEXT_COLOR);
	graphics.DrawString(headerText, headerText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(paintRect.X + NODE_HEADER_TEXT_BORDER_X, paintRect.Y + NODE_HEADER_TEXT_BORDER_Y), &textBrush);
}

void CDefaultGraphViewPainter::PaintNodeStatus(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const
{
	const GraphViewNodeStatusFlags statusFlags = node.GetStatusFlags();
	if (statusFlags.Check(EGraphViewNodeStatusFlags::ContainsErrors))
	{
		Gdiplus::SolidBrush brush(NODE_ERROR_FILL_COLOR);
		graphics.FillRectangle(&brush, paintRect);

		const CStringW statusText(NODE_ERROR_TEXT);
		const Gdiplus::SolidBrush textBrush(NODE_ERROR_TEXT_COLOR);
		graphics.DrawString(statusText, statusText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(paintRect.X + NODE_ERROR_TEXT_BORDER_X, paintRect.Y + NODE_ERROR_TEXT_BORDER_Y), &textBrush);
	}
	else if (statusFlags.Check(EGraphViewNodeStatusFlags::ContainsWarnings))
	{
		Gdiplus::SolidBrush brush(NODE_WARNING_FILL_COLOR);
		graphics.FillRectangle(&brush, paintRect);

		const CStringW statusText(NODE_WARNING_TEXT);
		const Gdiplus::SolidBrush textBrush(NODE_WARNING_TEXT_COLOR);
		graphics.DrawString(statusText, statusText.GetLength(), &Assets::GetFont(), Gdiplus::PointF(paintRect.X + NODE_WARNING_TEXT_BORDER_X, paintRect.Y + NODE_WARNING_TEXT_BORDER_Y), &textBrush);
	}
}

void CDefaultGraphViewPainter::PaintNodeOutline(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const
{
	Gdiplus::Color outlineColor = NODE_BODY_OUTLINE_COLOR;
	if (node.IsSelected())
	{
		const float time = gEnv->pTimer->GetAsyncTime().GetSeconds();
		const float sinA = sin_tpl(time * ALPHA_HIGHLIGHT_SPEED * gf_PI2);
		const float alpha = ALPHA_HIGHLIGHT_MIN + clamp_tpl(ALPHA_HIGHLIGHT_MIN + (sinA * ((ALPHA_HIGHLIGHT_MAX - ALPHA_HIGHLIGHT_MIN) * 0.5f)), 0.0f, 1.0f);
		outlineColor = Gdiplus::Color::MakeARGB(static_cast<BYTE>(alpha * 255.0f), NODE_BODY_OUTLINE_COLOR_HIGHLIGHT.GetRed(), NODE_BODY_OUTLINE_COLOR_HIGHLIGHT.GetGreen(), NODE_BODY_OUTLINE_COLOR_HIGHLIGHT.GetBlue());
	}

	const float bevels[4] = { NODE_BEVEL, NODE_BEVEL, NODE_BEVEL, NODE_BEVEL };
	const Gdiplus::Pen pen(outlineColor, NODE_BODY_OUTLINE_WIDTH);
	GdiplusUtils::PaintBeveledRect(graphics, paintRect, bevels, pen);
}

void CDefaultGraphViewPainter::UpdateNodeInputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 inputIdx, Gdiplus::PointF pos, float maxWidth) const
{
	const CStringW stringW(node.GetInputName(inputIdx));
	Gdiplus::RectF nameRect;
	graphics.MeasureString(stringW, stringW.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &nameRect);

	const Gdiplus::PointF iconSize(NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_WIDTH + NODE_INPUT_ICON_BORDER, NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_HEIGHT + NODE_INPUT_ICON_BORDER);
	const Gdiplus::PointF nameSize(NODE_INPUT_NAME_BORDER + nameRect.Width + NODE_INPUT_NAME_BORDER, NODE_INPUT_NAME_BORDER + nameRect.Height + NODE_INPUT_NAME_BORDER);
	const Gdiplus::RectF paintRect(pos.X, pos.Y, iconSize.X + nameSize.X, std::max(iconSize.Y, nameSize.Y));
	node.SetInputPaintRect(inputIdx, paintRect);
}

void CDefaultGraphViewPainter::PaintNodeInput(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 inputIdx) const
{
	const Gdiplus::RectF paintRect = node.GetInputPaintRect(inputIdx);
	const Gdiplus::PointF iconSize(NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_WIDTH + NODE_INPUT_ICON_BORDER, NODE_INPUT_ICON_BORDER + NODE_INPUT_ICON_HEIGHT + NODE_INPUT_ICON_BORDER);
	Gdiplus::PointF iconPos(paintRect.X + NODE_INPUT_ICON_BORDER, paintRect.Y + NODE_INPUT_ICON_BORDER);
	if (iconSize.Y < paintRect.Height)
	{
		iconPos.Y += (paintRect.Height - iconSize.Y) * 0.5f;
	}

	EGraphViewPortStyle style = EGraphViewPortStyle::Data;
	const ScriptGraphPortFlags flags = node.GetInputFlags(inputIdx);
	if (flags.Check(EScriptGraphPortFlags::Signal))
	{
		style = EGraphViewPortStyle::Signal;
	}
	else if (flags.Check(EScriptGraphPortFlags::Flow))
	{
		style = EGraphViewPortStyle::Flow;
	}

	PaintNodeInputIcon(graphics, iconPos, node.GetInputColor(inputIdx), style);

	const CStringW stringW(node.GetInputName(inputIdx));
	const Gdiplus::RectF alignedNameRect(paintRect.X + iconSize.X, paintRect.Y, paintRect.Width - iconSize.X, paintRect.Height);
	Gdiplus::StringFormat stringFormat;
	stringFormat.SetAlignment(Gdiplus::StringAlignmentNear);
	stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	const Gdiplus::SolidBrush nameBrush(NODE_OUTPUT_NAME_COLOR);
	graphics.DrawString(stringW, stringW.GetLength(), &Assets::GetFont(), alignedNameRect, &stringFormat, &nameBrush);
}

void CDefaultGraphViewPainter::PaintNodeInputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, EGraphViewPortStyle style) const
{
	const Gdiplus::SolidBrush fillBrush(color);
	const Gdiplus::Pen pen(NODE_INPUT_ICON_OUTLINE_COLOR, NODE_INPUT_ICON_OUTLINE_WIDTH);
	switch (style)
	{
	case EGraphViewPortStyle::Signal:
		{
			graphics.FillRectangle(&fillBrush, pos.X + 3.0f, pos.Y, 4.0f, 7.0f);
			graphics.FillRectangle(&fillBrush, pos.X + 3.0f, pos.Y + 8.0f, 4.0f, 3.0f);
			break;
		}
	case EGraphViewPortStyle::Flow:
		{
			const Gdiplus::PointF points[] =
			{
				Gdiplus::PointF(pos.X,                         pos.Y),
				Gdiplus::PointF(pos.X + NODE_INPUT_ICON_WIDTH, pos.Y + (NODE_INPUT_ICON_HEIGHT * 0.5f)),
				Gdiplus::PointF(pos.X,                         pos.Y + NODE_INPUT_ICON_HEIGHT)
			};
			graphics.FillPolygon(&fillBrush, points, CRY_ARRAY_COUNT(points));
			break;
		}
	case EGraphViewPortStyle::Data:
		{
			graphics.FillEllipse(&fillBrush, pos.X, pos.Y, NODE_INPUT_ICON_WIDTH, NODE_INPUT_ICON_HEIGHT);
			break;
		}
	}
}

void CDefaultGraphViewPainter::UpdateNodeOutputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 outputIdx, Gdiplus::PointF pos, float maxWidth) const
{
	const CStringW stringW(node.GetOutputName(outputIdx));
	Gdiplus::RectF nameRect;
	graphics.MeasureString(stringW, stringW.GetLength(), &Assets::GetFont(), Gdiplus::PointF(), &nameRect);

	const Gdiplus::PointF iconSize(NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_WIDTH + NODE_OUTPUT_ICON_BORDER, NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_HEIGHT + NODE_OUTPUT_ICON_BORDER);
	const Gdiplus::PointF nameSize(NODE_OUTPUT_NAME_BORDER + nameRect.Width + NODE_OUTPUT_NAME_BORDER, NODE_OUTPUT_NAME_BORDER + nameRect.Height + NODE_OUTPUT_NAME_BORDER);
	const Gdiplus::RectF paintRect(pos.X + maxWidth - (iconSize.X + nameSize.X), pos.Y, iconSize.X + nameSize.X, std::max(iconSize.Y, nameSize.Y));
	node.SetOutputPaintRect(outputIdx, paintRect);
}

void CDefaultGraphViewPainter::PaintNodeOutput(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 outputIdx) const
{
	const Gdiplus::RectF paintRect = node.GetOutputPaintRect(outputIdx);
	const Gdiplus::PointF iconSize(NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_WIDTH + NODE_OUTPUT_ICON_BORDER, NODE_OUTPUT_ICON_BORDER + NODE_OUTPUT_ICON_HEIGHT + NODE_OUTPUT_ICON_BORDER);
	Gdiplus::PointF iconPos(paintRect.X + paintRect.Width - NODE_OUTPUT_ICON_WIDTH - NODE_OUTPUT_ICON_BORDER, paintRect.Y + NODE_OUTPUT_ICON_BORDER);
	if (iconSize.Y < paintRect.Height)
	{
		iconPos.Y += (paintRect.Height - iconSize.Y) * 0.5f;
	}
	
	EGraphViewPortStyle style = EGraphViewPortStyle::Data;
	const ScriptGraphPortFlags flags = node.GetOutputFlags(outputIdx);
	if (flags.Check(EScriptGraphPortFlags::Signal))
	{
		style = EGraphViewPortStyle::Signal;
	}
	else if(flags.Check(EScriptGraphPortFlags::Flow))
	{
		style = EGraphViewPortStyle::Flow;
	}

	PaintNodeOutputIcon(graphics, iconPos, node.GetOutputColor(outputIdx), style);

	const CStringW stringW(node.GetOutputName(outputIdx));
	const Gdiplus::RectF alignedNameRect(paintRect.X, paintRect.Y, paintRect.Width - iconSize.X, paintRect.Height);
	Gdiplus::StringFormat stringFormat;
	stringFormat.SetAlignment(Gdiplus::StringAlignmentFar);
	stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	const Gdiplus::SolidBrush nameBrush(NODE_OUTPUT_NAME_COLOR);
	graphics.DrawString(stringW, stringW.GetLength(), &Assets::GetFont(), alignedNameRect, &stringFormat, &nameBrush);

	if (node.GetOutputFlags(outputIdx).Check(EScriptGraphPortFlags::SpacerAbove) && (outputIdx > 0))
	{
		const Gdiplus::Pen pen(NODE_OUTPUT_SPACER_COLOR, NODE_OUTPUT_SPACER_LINE_WIDTH);
		const float right = paintRect.GetRight() - NODE_OUTPUT_SPACER_OFFSET;
		const float left = right - NODE_OUTPUT_SPACER_WIDTH;
		const float top = paintRect.GetTop() - (NODE_INPUT_OUPUT_VERT_SPACING * 0.5f);
		graphics.DrawLine(&pen, Gdiplus::PointF(left, top), Gdiplus::PointF(right, top));
	}

	if (node.GetOutputFlags(outputIdx).Check(EScriptGraphPortFlags::SpacerBelow) && (outputIdx < (node.GetOutputCount() - 1)))
	{
		const Gdiplus::Pen pen(NODE_OUTPUT_SPACER_COLOR, NODE_OUTPUT_SPACER_LINE_WIDTH);
		const float right = paintRect.GetRight() - NODE_OUTPUT_SPACER_OFFSET;
		const float left = right - NODE_OUTPUT_SPACER_WIDTH;
		const float bottom = paintRect.GetBottom() + (NODE_INPUT_OUPUT_VERT_SPACING * 0.5f);
		graphics.DrawLine(&pen, Gdiplus::PointF(left, bottom), Gdiplus::PointF(right, bottom));
	}
}

void CDefaultGraphViewPainter::PaintNodeOutputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, EGraphViewPortStyle style) const
{
	const Gdiplus::SolidBrush fillBrush(color);
	const Gdiplus::Pen pen(NODE_OUTPUT_ICON_OUTLINE_COLOR, NODE_OUTPUT_ICON_OUTLINE_WIDTH);
	switch (style)
	{
	case EGraphViewPortStyle::Signal:
		{
			graphics.FillRectangle(&fillBrush, pos.X + 3.0f, pos.Y, 4.0f, 7.0f);
			graphics.FillRectangle(&fillBrush, pos.X + 3.0f, pos.Y + 8.0f, 4.0f, 3.0f);
			break;
		}
	case EGraphViewPortStyle::Flow:
		{
			const Gdiplus::PointF points[] =
			{
				Gdiplus::PointF(pos.X,                          pos.Y),
				Gdiplus::PointF(pos.X + NODE_OUTPUT_ICON_WIDTH, pos.Y + (NODE_OUTPUT_ICON_HEIGHT * 0.5f)),
				Gdiplus::PointF(pos.X,                          pos.Y + NODE_OUTPUT_ICON_HEIGHT)
			};
			graphics.FillPolygon(&fillBrush, points, CRY_ARRAY_COUNT(points));
			break;
		}
	case EGraphViewPortStyle::Data:
		{
			graphics.FillEllipse(&fillBrush, pos.X, pos.Y, NODE_OUTPUT_ICON_WIDTH, NODE_OUTPUT_ICON_HEIGHT);
			break;
		}
	}
}

BEGIN_MESSAGE_MAP(CGraphView, CWnd)
ON_WM_KILLFOCUS()
ON_WM_PAINT()
ON_WM_MOUSEMOVE()
ON_WM_MOUSEWHEEL()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_MBUTTONDOWN()
ON_WM_MBUTTONUP()
ON_WM_KEYDOWN()
ON_WM_KEYUP()
ON_WM_SETCURSOR()
ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CGraphView::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	if (!m_hWnd)
	{
		// Create view.
		LPCTSTR lpClassName = AfxRegisterWndClass(CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL);
		VERIFY(CreateEx(NULL, lpClassName, "SchematycView", dwStyle, CRect(0, 0, 100, 100), pParentWnd, nID));
		if (m_hWnd)
		{
			// Create timer.
			CWnd::SetTimer(UPDATE_TIMER_EVENT_ID, 1000 / 30, NULL);
			// Register drop target.
			m_dropTarget.Register(this);
			return true;
		}
	}
	return false;
}

void CGraphView::Refresh()
{
	__super::Invalidate(TRUE);
}

SGraphViewSettings& CGraphView::GetSettings()
{
	return m_settings;
}

const SGraphViewGrid& CGraphView::GetGrid() const
{
	return m_grid;
}

const IGraphViewPainterPtr& CGraphView::GetPainter() const
{
	return m_pPainter;
}

TGraphViewLinkVector& CGraphView::GetLinks()
{
	return m_links;
}

const TGraphViewLinkVector& CGraphView::GetLinks() const
{
	return m_links;
}

void CGraphView::DoQuickSearch(const char* szDefaultFilter, const CGraphViewNodePtr& pNode, uint32 outputIdx, const CPoint* pPoint)
{
	SCHEMATYC_EDITOR_MFC_RESOURCE_SCOPE
	CPoint cursorPoint;
	GetCursorPos(&cursorPoint);
	if (const IQuickSearchOptions* pQuickSearchOptions = GetQuickSearchOptions(cursorPoint, pNode, outputIdx))
	{
		CQuickSearchDlg quickSearchDlg(this, CPoint(cursorPoint.x - 20, cursorPoint.y - 20), *pQuickSearchOptions, nullptr, szDefaultFilter);
		if (quickSearchDlg.DoModal() == IDOK)
		{
			SetFocus();   // Ensure we regain focus from quick search dialog.
			OnQuickSearchResult(pPoint ? *pPoint : cursorPoint, pNode, outputIdx, quickSearchDlg.GetResult());
		}
	}
}

CGraphView::CGraphView(const SGraphViewGrid& grid, const IGraphViewPainterPtr& pPainter)
	: m_grid(grid)
	, m_pPainter(pPainter ? pPainter : IGraphViewPainterPtr(new CDefaultGraphViewPainter()))
	, m_enabled(false)
	, m_prevMousePoint(0, 0)
	, m_zoom(1.0f)
	, m_iSelectedLink(InvalidIdx)
	, m_dropTarget(*this)
{}

void CGraphView::Enable(bool enable)
{
	if (enable != m_enabled)
	{
		ExitDragState();
	}
	m_enabled = enable;
}

bool CGraphView::IsEnabled() const
{
	return m_enabled;
}

void CGraphView::ClearSelection()
{
	ClearNodeSelection();
	ClearLinkSelection();
	OnSelection(CGraphViewNodePtr(), nullptr);
	__super::Invalidate(TRUE);
}

void CGraphView::SetScrollOffset(Gdiplus::PointF scrollOffset)
{
	m_scrollOffset = scrollOffset;
	OnMove(m_scrollOffset);
}

void CGraphView::AddNode(const CGraphViewNodePtr& pNode, bool select, bool scrollToFit)
{
	CRY_ASSERT(pNode);
	if (pNode)
	{
		m_nodes.push_back(pNode);
		if (select)
		{
			ClearNodeSelection();
			SelectNode(pNode);
		}
		if (scrollToFit)
		{
			CPaintDC paintDC(this);
			Gdiplus::Graphics mainGraphics(paintDC.GetSafeHdc());
			m_pPainter->UpdateNodePaintRect(mainGraphics, m_grid, *pNode);
			ScrollToFit(pNode->GetPaintRect());
		}
		__super::Invalidate(TRUE);
	}
}

void CGraphView::RemoveNode(CGraphViewNodePtr pNode)
{
	CRY_ASSERT(pNode != NULL);
	if (pNode != NULL)
	{
		// Make sure node is not selected.
		ClearSelection();
		// Remove links connected to node.
		RemoveLinksConnectedToNode(pNode);
		// Remove node.
		TGraphViewNodePtrVector::iterator iEndNode = m_nodes.end();
		TGraphViewNodePtrVector::iterator iEraseNode = std::find(m_nodes.begin(), iEndNode, pNode);
		CRY_ASSERT(iEraseNode != iEndNode);
		if (iEraseNode != iEndNode)
		{
			m_nodes.erase(iEraseNode);
			OnNodeRemoved(*pNode);
		}
		__super::Invalidate(TRUE);
	}
}

CGraphViewNodePtr CGraphView::FindNode(Gdiplus::PointF pos)
{
	for (TGraphViewNodePtrVector::const_iterator iNode = m_nodes.begin(), iEndNode = m_nodes.end(); iNode != iEndNode; ++iNode)
	{
		const CGraphViewNodePtr& pNode = *iNode;
		if (pNode->GetPaintRect().Contains(pos))
		{
			return pNode;
		}
	}
	return CGraphViewNodePtr();
}

TGraphViewNodePtrVector& CGraphView::GetNodes()
{
	return m_nodes;
}

const TGraphViewNodePtrVector& CGraphView::GetNodes() const
{
	return m_nodes;
}

void CGraphView::ClearNodeSelection()
{
	for (TGraphViewNodePtrVector::iterator iSelectedNode = m_selectedNodes.begin(), iEndSelectedNode = m_selectedNodes.end(); iSelectedNode != iEndSelectedNode; ++iSelectedNode)
	{
		(*iSelectedNode)->Select(false);
	}
	m_selectedNodes.clear();
}

void CGraphView::SelectNode(const CGraphViewNodePtr& pNode)
{
	if ((pNode != NULL) && (pNode->IsSelected() == false))
	{
		ClearLinkSelection();
		m_selectedNodes.push_back(pNode);
		pNode->Select(true);
		OnSelection(pNode, nullptr);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::SelectNodesInRect(const Gdiplus::RectF& rect)
{
	ClearSelection();
	for (TGraphViewNodePtrVector::iterator iNode = m_nodes.begin(), iEndNode = m_nodes.end(); iNode != iEndNode; ++iNode)
	{
		CGraphViewNodePtr pNode = *iNode;
		if (rect.IntersectsWith(pNode->GetPaintRect()) != 0)
		{
			SelectNode(pNode);
		}
	}
}

const TGraphViewNodePtrVector& CGraphView::GetSelectedNodes() const
{
	return m_selectedNodes;
}

void CGraphView::MoveSelectedNodes(Gdiplus::PointF transform, bool bSnapToGrid)
{
	for (TGraphViewNodePtrVector::iterator iNode = m_selectedNodes.begin(), iEndNode = m_selectedNodes.end(); iNode != iEndNode; ++iNode)
	{
		CGraphViewNodePtr pNode = *iNode;
		pNode->SetPos(pNode->GetPos() + transform, m_settings.format.bSnapToGrid && bSnapToGrid);
	}
}

bool CGraphView::CreateLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId)
{
	SCHEMATYC_EDITOR_ASSERT(pSrcNode && !srcOutputId.IsEmpty() && pDstNode && !dstInputId.IsEmpty());
	if (pSrcNode && !srcOutputId.IsEmpty() && pDstNode && !dstInputId.IsEmpty())
	{
		if (CanCreateLink(*pSrcNode, srcOutputId, *pDstNode, dstInputId))
		{
			m_links.push_back(SGraphViewLink(this, pSrcNode, srcOutputId, pDstNode, dstInputId));
			OnLinkCreated(m_links.back());
			__super::Invalidate(TRUE);
			return true;
		}
	}
	return false;
}

void CGraphView::AddLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId)
{
	SCHEMATYC_EDITOR_ASSERT(pSrcNode && !srcOutputId.IsEmpty() && pDstNode && !dstInputId.IsEmpty());
	if (pSrcNode && !srcOutputId.IsEmpty() && pDstNode && !dstInputId.IsEmpty())
	{
		m_links.push_back(SGraphViewLink(this, pSrcNode, srcOutputId, pDstNode, dstInputId));
		__super::Invalidate(TRUE);
	}
}

void CGraphView::RemoveLink(uint32 iLink)
{
	const uint32 linkCount = m_links.size();
	CRY_ASSERT(iLink < linkCount);
	if (iLink < linkCount)
	{
		if (m_iSelectedLink == iLink)
		{
			m_iSelectedLink = InvalidIdx;
		}
		OnLinkRemoved(m_links[iLink]);
		m_links.erase(m_links.begin() + iLink);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::RemoveLinksConnectedToNode(const CGraphViewNodePtr& pNode)
{
	if (!m_links.empty())
	{
		TGraphViewLinkVector::iterator iEndLink = m_links.end();
		TGraphViewLinkVector::iterator iLastLink = iEndLink - 1;
		for (TGraphViewLinkVector::iterator iLink = m_links.begin(); iLink <= iLastLink; )
		{
			SGraphViewLink& link = *iLink;
			if ((link.pSrcNode.lock() == pNode) || (link.pDstNode.lock() == pNode))
			{
				OnLinkRemoved(link);
				if (iLink < iLastLink)
				{
					*iLink = *iLastLink;
				}
				--iLastLink;
			}
			else
			{
				++iLink;
			}
		}
		m_links.erase(iLastLink + 1, iEndLink);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::RemoveLinksConnectedToNodeInput(const CGraphViewNodePtr& pNode, const CGraphPortId& inputId)
{
	if (!m_links.empty())
	{
		TGraphViewLinkVector::iterator iEndLink = m_links.end();
		TGraphViewLinkVector::iterator iLastLink = iEndLink - 1;
		for (TGraphViewLinkVector::iterator iLink = m_links.begin(); iLink <= iLastLink; )
		{
			SGraphViewLink& link = *iLink;
			if ((link.pDstNode.lock() == pNode) && (link.dstInputId == inputId))
			{
				OnLinkRemoved(link);
				if (iLink < iLastLink)
				{
					*iLink = *iLastLink;
				}
				--iLastLink;
			}
			else
			{
				++iLink;
			}
		}
		m_links.erase(iLastLink + 1, iEndLink);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::RemoveLinksConnectedToNodeOutput(const CGraphViewNodePtr& pNode, const CGraphPortId& outputId)
{
	if (!m_links.empty())
	{
		TGraphViewLinkVector::iterator iEndLink = m_links.end();
		TGraphViewLinkVector::iterator iLastLink = iEndLink - 1;
		for (TGraphViewLinkVector::iterator iLink = m_links.begin(); iLink <= iLastLink; )
		{
			SGraphViewLink& link = *iLink;
			if ((link.pSrcNode.lock() == pNode) && (link.srcOutputId == outputId))
			{
				OnLinkRemoved(link);
				if (iLink < iLastLink)
				{
					*iLink = *iLastLink;
				}
				--iLastLink;
			}
			else
			{
				++iLink;
			}
		}
		m_links.erase(iLastLink + 1, iEndLink);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::FindLinks(const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId, UInt32Vector& output) const
{
	for (TGraphViewLinkVector::const_iterator iBeginLink = m_links.begin(), iEndLink = m_links.end(), iLink = iBeginLink; iLink != iEndLink; ++iLink)
	{
		const SGraphViewLink& link = *iLink;
		if ((link.pDstNode.lock() == pDstNode) && (link.dstInputId == dstInputId))
		{
			output.push_back(static_cast<uint32>(iLink - iBeginLink));
		}
	}
}

uint32 CGraphView::FindLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId)
{
	for (TGraphViewLinkVector::iterator iBeginLink = m_links.begin(), iEndLink = m_links.end(), iLink = iBeginLink; iLink != iEndLink; ++iLink)
	{
		SGraphViewLink& link = *iLink;
		if ((link.pSrcNode.lock() == pSrcNode) && (link.srcOutputId == srcOutputId) && (link.pDstNode.lock() == pDstNode) && (link.dstInputId == dstInputId))
		{
			return static_cast<uint32>(iLink - iBeginLink);
		}
	}
	return InvalidIdx;
}

uint32 CGraphView::FindLink(Gdiplus::PointF point)
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Part of temporary fix for slow graphs with lots of links, until we can optimize the intersection tests properly.
	if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0)
	{
		return InvalidIdx;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	for (TGraphViewLinkVector::iterator iBeginLink = m_links.begin(), iEndLink = m_links.end(), iLink = iBeginLink; iLink != iEndLink; ++iLink)
	{
		SGraphViewLink& link = *iLink;
		if (link.paintRect.Contains(point))
		{
			const CGraphViewNodePtr pSrcNode = link.pSrcNode.lock();
			const CGraphViewNodePtr pDstNode = link.pDstNode.lock();
			CRY_ASSERT(pSrcNode && pDstNode);
			if (pSrcNode && pDstNode)
			{
				const uint32 srcOutputIdx = pSrcNode->FindOutputById(link.srcOutputId);
				const uint32 dstInputIdx = pDstNode->FindInputById(link.dstInputId);
				CRY_ASSERT((srcOutputIdx != InvalidIdx) && (dstInputIdx != InvalidIdx));
				if ((srcOutputIdx != InvalidIdx) && (dstInputIdx != InvalidIdx))
				{
					// N.B. This hit test is far from optimal and it may be better to implement something along the lines of what is proposed here: https://msdn.microsoft.com/en-us/library/ms969920.aspx.
					Gdiplus::Bitmap bitmap(static_cast<INT>(link.paintRect.Width), static_cast<INT>(link.paintRect.Height), PixelFormat4bppIndexed);
					if (Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromImage(&bitmap))
					{
						const Gdiplus::Color backgroundColor(255, 0, 0, 0);
						pGraphics->Clear(backgroundColor);

						const Gdiplus::PointF topLeft(link.paintRect.X, link.paintRect.Y);
						Gdiplus::PointF startPoint = pSrcNode->GetOutputLinkPoint(srcOutputIdx);
						Gdiplus::PointF endPoint = pDstNode->GetInputLinkPoint(dstInputIdx);
						Gdiplus::PointF testPoint = point;
						startPoint.X -= topLeft.X;
						startPoint.Y -= topLeft.Y;
						endPoint.X -= topLeft.X;
						endPoint.Y -= topLeft.Y;
						testPoint.X -= topLeft.X;
						testPoint.Y -= topLeft.Y;

						m_pPainter->PaintLink(*pGraphics, startPoint, endPoint, pSrcNode->GetOutputColor(srcOutputIdx), link.bSelected, 1.2f);

						Gdiplus::Color testColor = backgroundColor;
						const Gdiplus::Status testStatus = bitmap.GetPixel(static_cast<INT>(testPoint.X), static_cast<INT>(testPoint.Y), &testColor);

						delete pGraphics;

						if ((testStatus == Gdiplus::Ok) && (testColor.GetValue() != backgroundColor.GetValue()))
						{
							return static_cast<uint32>(iLink - iBeginLink);
						}
					}
				}
			}
		}
	}
	return InvalidIdx;
}

void CGraphView::ClearLinkSelection()
{
	if (m_iSelectedLink != InvalidIdx)
	{
		m_links[m_iSelectedLink].bSelected = false;
		m_iSelectedLink = InvalidIdx;
	}
}

void CGraphView::SelectLink(uint32 iLink)
{
	CRY_ASSERT(iLink != InvalidIdx);
	if ((iLink != InvalidIdx) && (iLink != m_iSelectedLink))
	{
		ClearSelection();
		m_iSelectedLink = iLink;
		m_links[m_iSelectedLink].bSelected = true;
		OnSelection(CGraphViewNodePtr(), &m_links[m_iSelectedLink]);
		__super::Invalidate(TRUE);
	}
}

uint32 CGraphView::GetSelectedLink() const
{
	return m_iSelectedLink;
}

void CGraphView::Clear()
{
	m_nodes.clear();
	m_links.clear();
}

Gdiplus::PointF CGraphView::ClientToGraph(LONG x, LONG y) const
{
	return Gdiplus::PointF((x + m_scrollOffset.X) / m_zoom, (y + m_scrollOffset.Y) / m_zoom);
}

Gdiplus::PointF CGraphView::ClientToGraph(Gdiplus::PointF pos) const
{
	return Gdiplus::PointF((pos.X + m_scrollOffset.X) / m_zoom, (pos.Y + m_scrollOffset.Y) / m_zoom);
}

Gdiplus::PointF CGraphView::ClientToGraphTransform(Gdiplus::PointF transform) const
{
	return Gdiplus::PointF(transform.X / m_zoom, transform.Y / m_zoom);
}

Gdiplus::RectF CGraphView::ClientToGraph(Gdiplus::RectF rect) const
{
	return Gdiplus::RectF((rect.X + m_scrollOffset.X) / m_zoom, (rect.Y + m_scrollOffset.Y) / m_zoom, rect.Width / m_zoom, rect.Height / m_zoom);
}

Gdiplus::PointF CGraphView::GraphToClient(Gdiplus::PointF pos) const
{
	return Gdiplus::PointF((pos.X * m_zoom) - m_scrollOffset.X, (pos.Y * m_zoom) - m_scrollOffset.Y);
}

Gdiplus::PointF CGraphView::GraphToClientTransform(Gdiplus::PointF transform) const
{
	return Gdiplus::PointF(transform.X * m_zoom, transform.Y * m_zoom);
}

Gdiplus::RectF CGraphView::GraphToClient(Gdiplus::RectF rect) const
{
	return Gdiplus::RectF((rect.X * m_zoom) - m_scrollOffset.X, (rect.Y * m_zoom) - m_scrollOffset.Y, rect.Width * m_zoom, rect.Height * m_zoom);
}

void CGraphView::ScrollToFit(Gdiplus::RectF rect)
{
	CRect clientRect;
	GetClientRect(&clientRect);
	Gdiplus::RectF graphViewRect = ClientToGraph(Gdiplus::RectF(static_cast<Gdiplus::REAL>(clientRect.left), static_cast<Gdiplus::REAL>(clientRect.top),
	                                                            static_cast<Gdiplus::REAL>(clientRect.Width()), static_cast<Gdiplus::REAL>(clientRect.Height())));
	const float dx = graphViewRect.GetRight() - rect.GetRight();
	if (dx < 0.0f)
	{
		m_scrollOffset.X += (-dx + DEFAULT_NODE_SPACING) * m_zoom;
		OnMove(m_scrollOffset);
	}
	// #SchematycTODO : Check left, top and bottom of view too?
}

void CGraphView::CenterPosition(Gdiplus::PointF point)
{
	CRect clientRect;
	GetClientRect(&clientRect);
	Gdiplus::RectF graphViewRect = ClientToGraph(Gdiplus::RectF(static_cast<Gdiplus::REAL>(clientRect.left), static_cast<Gdiplus::REAL>(clientRect.top),
	                                                            static_cast<Gdiplus::REAL>(clientRect.Width()), static_cast<Gdiplus::REAL>(clientRect.Height())));
	const float dx = graphViewRect.X - point.X + (graphViewRect.Width / 2);
	const float dy = graphViewRect.Y - point.Y + (graphViewRect.Height / 2);

	m_scrollOffset.X += (-dx + DEFAULT_NODE_SPACING) * m_zoom;
	m_scrollOffset.Y += (-dy + DEFAULT_NODE_SPACING) * m_zoom;

	OnMove(m_scrollOffset);
}

BOOL CGraphView::PreTranslateMessage(MSG* pMsg)
{
	if ((pMsg->message == WM_KEYDOWN) && (GetFocus() == this))
	{
		OnKeyDown((UINT)pMsg->wParam, LOWORD(pMsg->lParam), HIWORD(pMsg->lParam));
		return TRUE;
	}
	else
	{
		return __super::PreTranslateMessage(pMsg);
	}
}

void CGraphView::OnDraw(CDC* pDC) {}

void CGraphView::OnKillFocus(CWnd* pNewWnd)
{
	ExitDragState();
}

void CGraphView::OnPaint()
{
	if (m_pPainter)
	{
		CPaintDC paintDC(this);
		const CRect paintRect = paintDC.m_ps.rcPaint;
		Gdiplus::Bitmap backBuffer(paintRect.Width(), paintRect.Height());
		if (Gdiplus::Graphics* pBackBufferGraphics = Gdiplus::Graphics::FromImage(&backBuffer))
		{
			pBackBufferGraphics->TranslateTransform(-m_scrollOffset.X, -m_scrollOffset.Y);
			pBackBufferGraphics->ScaleTransform(m_zoom, m_zoom);
			pBackBufferGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			pBackBufferGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
			pBackBufferGraphics->Clear(m_pPainter->GetBackgroundColor());

			if (m_enabled)
			{
				m_pPainter->PaintGrid(*pBackBufferGraphics, m_grid);

				for (TGraphViewNodePtrVector::iterator iNode = m_nodes.begin(), iEndNode = m_nodes.end(); iNode != iEndNode; ++iNode)
				{
					m_pPainter->UpdateNodePaintRect(*pBackBufferGraphics, m_grid, *(*iNode));
				}

				for (TGraphViewLinkVector::iterator iLink = m_links.begin(), iEndLink = m_links.end(); iLink != iEndLink; ++iLink)
				{
					SGraphViewLink& link = *iLink;
					const CGraphViewNodePtr pSrcNode = link.pSrcNode.lock();
					const CGraphViewNodePtr pDstNode = link.pDstNode.lock();
					if (pSrcNode && pDstNode)
					{
						const uint32 srcOutputIdx = pSrcNode->FindOutputById(link.srcOutputId);
						const uint32 dstInputIdx = pDstNode->FindInputById(link.dstInputId);
						if ((srcOutputIdx != InvalidIdx) && (dstInputIdx != InvalidIdx))
						{
							link.paintRect = m_pPainter->PaintLink(*pBackBufferGraphics, pSrcNode->GetOutputLinkPoint(srcOutputIdx), pDstNode->GetInputLinkPoint(dstInputIdx), pSrcNode->GetOutputColor(srcOutputIdx), link.bSelected, 1.0f);
						}
					}
				}

				if (m_pDragState != NULL)
				{
					m_pDragState->Paint(*this, *pBackBufferGraphics, *m_pPainter);
				}

				for (TGraphViewNodePtrVector::reverse_iterator iNode = m_nodes.rbegin(), iEndNode = m_nodes.rend(); iNode != iEndNode; ++iNode)
				{
					m_pPainter->PaintNode(*pBackBufferGraphics, m_grid, *(*iNode));
				}
			}

			Gdiplus::Graphics mainGraphics(paintDC.GetSafeHdc());
			mainGraphics.DrawImage(&backBuffer, paintRect.left, paintRect.top, paintRect.Width(), paintRect.Height());

			delete pBackBufferGraphics;
		}
	}
}

void CGraphView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_enabled && (GetFocus() == this))
	{
		if (m_pDragState)
		{
			m_pDragState->Update(*this, Gdiplus::PointF(static_cast<float>(point.x), static_cast<float>(point.y)));
			__super::Invalidate(TRUE);
		}
		else if ((point.x != m_prevMousePoint.x) || (point.y != m_prevMousePoint.y))
		{
			if ((nFlags & MK_LBUTTON) != 0)
			{
				OnLeftMouseButtonDrag(m_prevMousePoint);
			}
			else if ((nFlags & MK_RBUTTON) != 0)
			{
				OnRightMouseButtonDrag(m_prevMousePoint);
			}
			else if ((nFlags & MK_MBUTTON) != 0)
			{
				OnMiddleMouseButtonDrag(m_prevMousePoint);
			}
		}
	}
	m_prevMousePoint = point;
	__super::OnMouseMove(nFlags, point);
}

BOOL CGraphView::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	// Store previous focus.
	CPoint cursorPoint;
	GetCursorPos(&cursorPoint);
	ScreenToClient(&cursorPoint);
	const Gdiplus::PointF prevClientFocus((Gdiplus::REAL)cursorPoint.x, (Gdiplus::REAL)cursorPoint.y);
	const Gdiplus::PointF prevGraphFocus = ClientToGraph(prevClientFocus);
	// Increment zoom.
	m_zoom = clamp_tpl(m_zoom + (DELTA_ZOOM * zDelta), MIN_ZOOM, MAX_ZOOM);
	// Scroll to previous focus point.
	const Gdiplus::PointF newClientFocus = GraphToClient(prevGraphFocus);
	m_scrollOffset.X += newClientFocus.X - prevClientFocus.X;
	m_scrollOffset.Y += newClientFocus.Y - prevClientFocus.Y;
	OnMove(m_scrollOffset);
	// Force re-draw.
	__super::Invalidate(TRUE);
	return TRUE;
}

void CGraphView::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
}

void CGraphView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_pDragState != NULL)
	{
		ExitDragState();
	}
	else
	{
		ClearSelection();
		const Gdiplus::PointF graphPos = ClientToGraph(point.x, point.y);
		CGraphViewNodePtr pNode = FindNode(graphPos);
		if (pNode != NULL)
		{
			SelectNode(pNode);
		}
		else
		{
			const uint32 iLink = FindLink(graphPos);
			if (iLink != InvalidIdx)
			{
				SelectLink(iLink);
			}
		}
	}
}

afx_msg void CGraphView::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
}

afx_msg void CGraphView::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_pDragState != NULL)
	{
		ExitDragState();
	}
	else
	{
		CMenu popupMenu;
		popupMenu.CreatePopupMenu();
		const Gdiplus::PointF graphPos = ClientToGraph(point.x, point.y);
		CGraphViewNodePtr pNode = FindNode(graphPos);
		const uint32 iNodeInput = pNode ? pNode->FindInput(graphPos) : InvalidIdx;
		const uint32 iNodeOutput = pNode ? pNode->FindOutput(graphPos) : InvalidIdx;
		ClientToScreen(&point);
		GetPopupMenuItems(popupMenu, pNode, iNodeInput, iNodeOutput, point);
		const int popupMenuItemCount = popupMenu.GetMenuItemCount();
		if (!pNode && (popupMenuItemCount == 1))
		{
			OnPopupMenuResult(popupMenu.GetMenuItemID(0), pNode, iNodeInput, iNodeOutput, point);
		}
		else if (popupMenuItemCount > 0)
		{
			int popupMenuItem = popupMenu.TrackPopupMenuEx(TPM_RETURNCMD, point.x, point.y, this, NULL);
			OnPopupMenuResult(popupMenuItem, pNode, iNodeInput, iNodeOutput, point);
		}
	}
}

afx_msg void CGraphView::OnMButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
}

afx_msg void CGraphView::OnMButtonUp(UINT nFlags, CPoint point)
{
	if (m_pDragState != NULL)
	{
		ExitDragState();
	}
}

void CGraphView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ((GetFocus() == this) && m_enabled && (nRepCnt == 1))
	{
		const bool bCtrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		if (bCtrlDown)
		{
			switch (nChar)
			{
			case 'F':
				{
					FormatSelectedNodes();
					return;
				}
			}
		}
		else
		{
			switch (nChar)
			{
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Part of temporary fix for slow graphs with lots of links, until we can optimize the intersection tests properly.
			case VK_SHIFT:
				{
					UpdateCursor();
					return;
				}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			case VK_DELETE:
				{
					OnDelete();
					return;
				}
			}
			if ((nChar >= 'A') && (nChar <= 'Z'))
			{
				const char defaultFilter[2] = { static_cast<char>(nChar), '\0' };
				if (m_selectedNodes.size() == 1)
				{
					CGraphViewNodePtr pSelectedNode = m_selectedNodes[0];
					Gdiplus::RectF paintRect = pSelectedNode->GetPaintRect();
					Gdiplus::PointF nextPos = GraphToClient(Gdiplus::PointF(paintRect.GetRight() + DEFAULT_NODE_SPACING, paintRect.GetTop()));
					CPoint screenPoint(static_cast<int>(nextPos.X), static_cast<int>(nextPos.Y));
					ClientToScreen(&screenPoint);
					DoQuickSearch(defaultFilter, pSelectedNode, 0, &screenPoint);   // Rather than passing zero it would be nice to pass the index of the first/selected execution output.
				}
				else
				{
					DoQuickSearch(defaultFilter);
				}
				return;
			}
		}
	}
	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CGraphView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if ((GetFocus() == this) && m_enabled && (nRepCnt == 1))
	{
		switch (nChar)
		{
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Part of temporary fix for slow graphs with lots of links, until we can optimize the intersection tests properly.
		case VK_SHIFT:
			{
				SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
				return;
			}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}
	__super::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CGraphView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (UpdateCursor())
	{
		return TRUE;
	}
	return __super::OnSetCursor(pWnd, nHitTest, message);
}

void CGraphView::OnMove(Gdiplus::PointF scrollOffset)                                               {}

void CGraphView::OnSelection(const CGraphViewNodePtr& pSelectedNode, SGraphViewLink* pSelectedLink) {}

void CGraphView::OnLinkModify(const SGraphViewLink& link)                                           {}

void CGraphView::OnNodeRemoved(const CGraphViewNode& node)                                          {}

bool CGraphView::CanCreateLink(const CGraphViewNode& srcNode, const CGraphPortId& srcOutputId, const CGraphViewNode& dstNode, const CGraphPortId& dstInputId) const
{
	return true;
}

void       CGraphView::OnLinkCreated(const SGraphViewLink& link) {}

void       CGraphView::OnLinkRemoved(const SGraphViewLink& link) {}

DROPEFFECT CGraphView::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

BOOL CGraphView::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	return false;
}

void CGraphView::GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, uint32 iNodeInput, uint32 iNodeOutput, CPoint point)
{
	if (pNode)
	{
		popupMenu.AppendMenu(MF_STRING, EPopupMenuItem::COPY_NODE, "Copy Node");

		if (pNode->IsRemoveable())
		{
			popupMenu.AppendMenu(MF_STRING, EPopupMenuItem::REMOVE_NODE, "Remove Node");
		}
	}
	else
	{
		popupMenu.AppendMenu(MF_STRING, EPopupMenuItem::QUICK_SEARCH, "Quick Search");
	}
}

void CGraphView::OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, uint32 iNodeInput, uint32 iNodeOutput, CPoint point)
{
	switch (popupMenuItem)
	{
	case EPopupMenuItem::QUICK_SEARCH:
		{
			DoQuickSearch();
			break;
		}
	case EPopupMenuItem::COPY_NODE:
		{
			CRY_ASSERT(pNode);
			CStackString clipboardText;
			pNode->CopyToXml(clipboardText);
			if (!clipboardText.empty())
			{
				PluginUtils::WriteToClipboard(clipboardText.c_str(), g_szGraphNodeClipboardPrefix);
			}
			break;
		}
	case EPopupMenuItem::REMOVE_NODE:
		{
			CRY_ASSERT(pNode);
			RemoveNode(pNode);
			break;
		}
	}
}

const IQuickSearchOptions* CGraphView::GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, uint32 iNodeOutput)
{
	return nullptr;
}

void CGraphView::OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, uint32 iNodeOutput, uint32 iSelectedOption) {}

bool CGraphView::ShouldUpdate() const
{
	return GetFocus() == this;
}

void CGraphView::OnUpdate() {}

CGraphView::CScrollDragState::CScrollDragState(Gdiplus::PointF pos)
	: m_prevPos(pos)
{}

void CGraphView::CScrollDragState::Update(CGraphView& graphView, Gdiplus::PointF pos)
{
	if ((pos.X != m_prevPos.X) || (pos.Y != m_prevPos.Y))
	{
		graphView.Scroll(pos - m_prevPos);
		m_prevPos = pos;
	}
}

void CGraphView::CScrollDragState::Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter) {}

void CGraphView::CScrollDragState::Exit(CGraphView& graphView)                                                           {}

CGraphView::CMoveDragState::CMoveDragState(Gdiplus::PointF pos)
	: m_prevPos(pos)
{}

void CGraphView::CMoveDragState::Update(CGraphView& graphView, Gdiplus::PointF pos)
{
	graphView.MoveSelectedNodes(graphView.ClientToGraphTransform(pos - m_prevPos), false);
	m_prevPos = pos;
}

void CGraphView::CMoveDragState::Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter) {}

void CGraphView::CMoveDragState::Exit(CGraphView& graphView)
{
	graphView.MoveSelectedNodes(Gdiplus::PointF(), true);
}

CGraphView::CLinkDragState::CLinkDragState(const CGraphViewNodeWeakPtr& pSrcNode, const CGraphPortId& srcOutputId, Gdiplus::PointF start, Gdiplus::PointF end, Gdiplus::Color color, bool bNewLink)
	: m_pSrcNode(pSrcNode)
	, m_srcOutputId(srcOutputId)
	, m_start(start)
	, m_end(end)
	, m_color(color)
	, m_bNewLink(bNewLink)
{}

void CGraphView::CLinkDragState::Update(CGraphView& graphView, Gdiplus::PointF pos)
{
	m_end = pos;
}

void CGraphView::CLinkDragState::Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter)
{
	if (CGraphViewNodePtr pSrcNode = m_pSrcNode.lock())
	{
		painter.PaintLink(graphics, m_start, graphView.ClientToGraph(m_end), m_color, true, 1.0f);
	}
}

void CGraphView::CLinkDragState::Exit(CGraphView& graphView)
{
	const Gdiplus::PointF graphPos = graphView.ClientToGraph(m_end);
	if (CGraphViewNodePtr pDstNode = graphView.FindNode(graphPos))
	{
		const uint32 dstInputIdx = pDstNode->FindInput(graphPos);
		if (dstInputIdx != InvalidIdx)
		{
			graphView.CreateLink(m_pSrcNode.lock(), m_srcOutputId, pDstNode, pDstNode->GetInputId(dstInputIdx));
		}
	}
	else if (m_bNewLink)
	{
		CGraphViewNodePtr pSrcNode = m_pSrcNode.lock();
		CRY_ASSERT(pSrcNode);
		if (pSrcNode)
		{
			graphView.DoQuickSearch(NULL, pSrcNode, pSrcNode->FindOutputById(m_srcOutputId));
		}
	}
}

CGraphView::CSelectDragState::CSelectDragState(Gdiplus::PointF pos)
	: m_startPos(pos)
	, m_endPos(pos)
{}

void CGraphView::CSelectDragState::Update(CGraphView& graphView, Gdiplus::PointF pos)
{
	m_endPos = pos;
	graphView.SelectNodesInRect(graphView.ClientToGraph(GetSelectionRect()));
}

void CGraphView::CSelectDragState::Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter)
{
	painter.PaintSelectionRect(graphics, graphView.ClientToGraph(GetSelectionRect()));
}

void           CGraphView::CSelectDragState::Exit(CGraphView& graphView) {}

Gdiplus::RectF CGraphView::CSelectDragState::GetSelectionRect() const
{
	Gdiplus::RectF selectionRect;
	selectionRect.X = std::min(m_startPos.X, m_endPos.X);
	selectionRect.Y = std::min(m_startPos.Y, m_endPos.Y);
	selectionRect.Width = fabs_tpl(m_endPos.X - m_startPos.X);
	selectionRect.Height = fabs_tpl(m_endPos.Y - m_startPos.Y);
	return selectionRect;
}

CGraphView::CDropTarget::CDropTarget(CGraphView& graphView)
	: m_graphView(graphView)
{}

DROPEFFECT CGraphView::CDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return m_graphView.OnDragOver(pWnd, pDataObject, dwKeyState, point);
}

BOOL CGraphView::CDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	return m_graphView.OnDrop(pWnd, pDataObject, dropEffect, point);
}

void CGraphView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == UPDATE_TIMER_EVENT_ID)
	{
		if (ShouldUpdate())
		{
			OnUpdate();
			//if(!m_selectedNodes.empty() || (m_iSelectedLink != InvalidIdx))	// Removed as part of temporary fix for slow graphs with lots of links, until we can optimize the intersection tests properly.
			{
				__super::Invalidate(TRUE);
			}
		}
	}
}

void CGraphView::OnLeftMouseButtonDrag(CPoint point)
{
	const Gdiplus::PointF clientPos(static_cast<float>(point.x), static_cast<float>(point.y));
	const Gdiplus::PointF graphPos = ClientToGraph(clientPos);
	if (CGraphViewNodePtr pNode = FindNode(graphPos))
	{
		if (m_selectedNodes.size() < 2)
		{
			ClearSelection();
			SelectNode(pNode);
		}
		const uint32 srcOutputIdx = pNode->FindOutput(graphPos);
		if (srcOutputIdx != InvalidIdx)
		{
			m_pDragState = IGraphViewDragStatePtr(new CLinkDragState(pNode, pNode->GetOutputId(srcOutputIdx), pNode->GetOutputLinkPoint(srcOutputIdx), clientPos, pNode->GetOutputColor(srcOutputIdx), true));
		}
		else
		{
			const uint32 dstInputIdx = pNode->FindInput(graphPos);
			if (dstInputIdx != InvalidIdx)
			{
				const CGraphPortId dstInputId = pNode->GetInputId(dstInputIdx);
				UInt32Vector links;
				FindLinks(pNode, dstInputId, links);
				if (links.size() == 1)
				{
					const uint32 iLink = links.front();
					const SGraphViewLink& link = m_links[iLink];
					if (CGraphViewNodePtr pSrcNode = link.pSrcNode.lock())
					{
						const uint32 iLinkSrcOutput = pSrcNode->FindOutputById(link.srcOutputId);
						CRY_ASSERT(iLinkSrcOutput != InvalidIdx);
						if (iLinkSrcOutput != InvalidIdx)
						{
							m_pDragState = IGraphViewDragStatePtr(new CLinkDragState(link.pSrcNode, link.srcOutputId, pSrcNode->GetOutputLinkPoint(iLinkSrcOutput), clientPos, pSrcNode->GetOutputColor(iLinkSrcOutput), false));
						}
					}
					RemoveLink(iLink);
				}
			}
			else
			{
				const Gdiplus::RectF nodePaintRect = pNode->GetPaintRect();
				m_pDragState = IGraphViewDragStatePtr(new CMoveDragState(clientPos));
			}
		}
	}
	else
	{
		m_pDragState = IGraphViewDragStatePtr(new CSelectDragState(clientPos));
	}
}

void CGraphView::OnRightMouseButtonDrag(CPoint point)
{
	m_pDragState = IGraphViewDragStatePtr(new CScrollDragState(Gdiplus::PointF(static_cast<float>(point.x), static_cast<float>(point.y))));
}

void CGraphView::OnMiddleMouseButtonDrag(CPoint point)
{
	m_pDragState = IGraphViewDragStatePtr(new CScrollDragState(Gdiplus::PointF(static_cast<float>(point.x), static_cast<float>(point.y))));
}

void CGraphView::Scroll(Gdiplus::PointF delta)
{
	m_scrollOffset.X = clamp_tpl(m_scrollOffset.X - delta.X, m_grid.bounds.X, m_grid.bounds.GetRight());
	m_scrollOffset.Y = clamp_tpl(m_scrollOffset.Y - delta.Y, m_grid.bounds.Y, m_grid.bounds.GetBottom());
	OnMove(m_scrollOffset);
}

void CGraphView::ExitDragState()
{
	if (m_pDragState != NULL)
	{
		IGraphViewDragStatePtr pDragState = m_pDragState;
		m_pDragState.reset();
		pDragState->Exit(*this);
		__super::Invalidate(TRUE);
	}
}

void CGraphView::OnDelete()
{
	if (m_selectedNodes.empty() == false)
	{
		if (CCustomMessageBox::Execute("Remove selected node(s)?", "Remove Node(s)", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			TGraphViewNodePtrVector removeNodes = m_selectedNodes;
			for (TGraphViewNodePtrVector::iterator iRemoveNode = removeNodes.begin(), iEndRemoveNode = removeNodes.end(); iRemoveNode != iEndRemoveNode; ++iRemoveNode)
			{
				RemoveNode(*iRemoveNode);
			}
		}
	}
	else if (m_iSelectedLink)
	{
		if (CCustomMessageBox::Execute("Remove selected link?", "Remove Link", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			RemoveLink(m_iSelectedLink);
		}
	}
}
bool CGraphView::UpdateCursor()
{
	if (!m_pDragState)
	{
		CPoint cursorPoint;
		GetCursorPos(&cursorPoint);
		ScreenToClient(&cursorPoint);
		const Gdiplus::PointF graphPos = ClientToGraph(Gdiplus::PointF(static_cast<float>(cursorPoint.x), static_cast<float>(cursorPoint.y)));
		if (FindNode(graphPos) || (FindLink(graphPos) != InvalidIdx))
		{
			SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HAND));
			return true;
		}
	}
	return false;
}

void CGraphView::FormatSelectedNodes()
{
	if (GetSchematycFramework().IsExperimentalFeatureEnabled("GraphNodeFormat"))
	{
		CGraphViewFormatter formatter(*this, m_settings.format);
		formatter.FormatNodes(m_selectedNodes);
	}
}
} //
