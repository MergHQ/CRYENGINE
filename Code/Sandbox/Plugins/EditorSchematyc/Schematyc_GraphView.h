// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Move CDefaultGraphViewPainter (along with port colors from function and graph views) to a separate file.

#pragma once

#include <Schematyc/Script/Schematyc_IScriptGraph.h> // #SchematycTODO : Can we remove this include? Right now it's only needed for access to ScriptGraphPortFlags.
#include <Schematyc/Utils/Schematyc_EnumFlags.h>

#include "Schematyc_QuickSearchDlg.h"

namespace Schematyc
{
class CGraphView;

typedef std::vector<uint32> UInt32Vector;

struct SGraphViewFormatSettings
{
	SGraphViewFormatSettings();

	void Serialize(Serialization::IArchive& archive);

	float horzSpacing;
	float vertSpacing;
	bool  bSnapToGrid;
};

struct SGraphViewSettings
{
	void Serialize(Serialization::IArchive& archive);

	SGraphViewFormatSettings format;
};

struct SGraphViewGrid // #SchematycTODO : Replace floats with int32s?
{
	inline SGraphViewGrid(Gdiplus::PointF _spacing, Gdiplus::RectF _bounds)
		: spacing(_spacing)
		, bounds(_bounds)
	{}

	inline float Snap(float value, float spacing) const
	{
		const float mod = fmodf(value, spacing);
		if (mod > (spacing * 0.5f))
		{
			return value + (spacing - mod);
		}
		else if (mod < (spacing * -0.5f))
		{
			return value - (spacing + mod);
		}
		else
		{
			return value - mod;
		}
	}

	inline Gdiplus::PointF SnapPos(Gdiplus::PointF pos) const
	{
		pos.X = Snap(pos.X, spacing.X);
		pos.Y = Snap(pos.Y, spacing.Y);
		return pos;
	}

	inline Gdiplus::SizeF SnapSize(Gdiplus::SizeF size) const
	{
		size.Width = size.Width + (spacing.X - fmodf(size.Width, spacing.X));
		size.Height = size.Height + (spacing.X - fmodf(size.Height, spacing.X));
		return size;
	}

	inline Gdiplus::RectF SnapRect(Gdiplus::RectF rect) const
	{
		Gdiplus::PointF pos;
		rect.GetLocation(&pos);
		pos = SnapPos(pos);

		Gdiplus::SizeF size;
		rect.GetSize(&size);
		size = SnapSize(size);

		return Gdiplus::RectF(pos, size);
	}

	Gdiplus::PointF spacing;
	Gdiplus::RectF  bounds;
};

enum class EGraphViewNodeStatusFlags
{
	None             = 0,
	ContainsWarnings = BIT(0),
	ContainsErrors   = BIT(1)
};

typedef CEnumFlags<EGraphViewNodeStatusFlags> GraphViewNodeStatusFlags;

enum class EGraphViewPortStyle
{
	Signal,
	Flow,
	Data
};

class CGraphViewNode
{
public:

	CGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos);
	virtual ~CGraphViewNode();

	const SGraphViewGrid&        GetGrid() const;
	void                         SetPos(Gdiplus::PointF pos, bool bSnapToGrid);
	Gdiplus::PointF              GetPos() const;
	void                         SetPaintRect(Gdiplus::RectF paintRect);
	Gdiplus::RectF               GetPaintRect() const;
	void                         Enable(bool bEnable);
	bool                         IsEnabled() const;
	void                         Select(bool bSelect);
	bool                         IsSelected() const;
	void                         SetStatusFlags(const GraphViewNodeStatusFlags& statusFlags);
	GraphViewNodeStatusFlags     GetStatusFlags() const;
	uint32                       FindInput(Gdiplus::PointF pos) const;
	void                         SetInputPaintRect(uint32 inputIdx, Gdiplus::RectF paintRect);
	Gdiplus::RectF               GetInputPaintRect(uint32 inputIdx) const;
	Gdiplus::PointF              GetInputLinkPoint(uint32 inputIdx) const;
	uint32                       FindOutput(Gdiplus::PointF pos) const;
	void                         SetOutputPaintRect(uint32 outputIdx, Gdiplus::RectF paintRect);
	Gdiplus::RectF               GetOutputPaintRect(uint32 outputIdx) const;
	Gdiplus::PointF              GetOutputLinkPoint(uint32 outputIdx) const;

	virtual const char*          GetName() const;
	virtual Gdiplus::Color       GetHeaderColor() const;
	virtual bool                 IsRemoveable() const;
	virtual uint32               GetInputCount() const;
	virtual uint32               FindInputById(const CGraphPortId& id) const;
	virtual CGraphPortId         GetInputId(uint32 inputIdx) const;
	virtual const char*          GetInputName(uint32 inputIdx) const;
	virtual ScriptGraphPortFlags GetInputFlags(uint32 inputIdx) const;
	virtual Gdiplus::Color       GetInputColor(uint32 inputIdx) const;
	virtual uint32               GetOutputCount() const;
	virtual uint32               FindOutputById(const CGraphPortId& id) const;
	virtual CGraphPortId         GetOutputId(uint32 outputIdx) const;
	virtual const char*          GetOutputName(uint32 outputIdx) const;
	virtual ScriptGraphPortFlags GetOutputFlags(uint32 outputIdx) const;
	virtual Gdiplus::Color       GetOutputColor(uint32 outputIdx) const;
	virtual void                 CopyToXml(IString& output) const;

protected:

	virtual void OnMove(Gdiplus::RectF paintRect);

private:

	typedef std::vector<Gdiplus::RectF> TRectVector;

	const SGraphViewGrid&    m_grid;
	bool                     m_enabled;
	bool                     m_selected;
	GraphViewNodeStatusFlags m_statusFlags;
	Gdiplus::RectF           m_paintRect;
	TRectVector              m_inputPaintRects;
	TRectVector              m_outputPaintRects;
};

DECLARE_SHARED_POINTERS(CGraphViewNode)

typedef std::vector<CGraphViewNodePtr> TGraphViewNodePtrVector;

struct SGraphViewLink
{
	SGraphViewLink(CGraphView* _pGraphView, const CGraphViewNodeWeakPtr& _pSrcNode, const CGraphPortId& _srcOutputId, const CGraphViewNodeWeakPtr& _pDstNode, const CGraphPortId& _dstInputId);

	CGraphView*           pGraphView;
	CGraphViewNodeWeakPtr pSrcNode;
	CGraphPortId          srcOutputId;
	CGraphViewNodeWeakPtr pDstNode;
	CGraphPortId          dstInputId;
	Gdiplus::RectF        paintRect;
	bool                  bSelected;
};

typedef std::vector<SGraphViewLink> TGraphViewLinkVector;

struct IGraphViewPainter
{
	virtual ~IGraphViewPainter() {}

	virtual Serialization::SStruct GetSettings() = 0; // #SchematycTODO : Replace Serialization::SStruct with CAnyRef/CAnyPtr?
	virtual Gdiplus::Color         GetBackgroundColor() const = 0;
	virtual void                   PaintGrid(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid) const = 0;
	virtual void                   UpdateNodePaintRect(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const = 0;
	virtual void                   PaintNode(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const = 0;
	virtual Gdiplus::RectF         PaintLink(Gdiplus::Graphics& graphics, Gdiplus::PointF startPoint, Gdiplus::PointF endPoint, Gdiplus::Color color, bool bHighlight, float scale) const = 0;
	virtual void                   PaintSelectionRect(Gdiplus::Graphics& graphics, Gdiplus::RectF rect) const = 0;
};

DECLARE_SHARED_POINTERS(IGraphViewPainter)

struct IGraphViewDragState
{
	virtual ~IGraphViewDragState() {}

	virtual void Update(CGraphView& graphView, Gdiplus::PointF pos) = 0;
	virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter) = 0;
	virtual void Exit(CGraphView& graphView) = 0;
};

DECLARE_SHARED_POINTERS(IGraphViewDragState)

struct SDefaultGraphViewPainterSettings
{
	SDefaultGraphViewPainterSettings();

	void Serialize(Serialization::IArchive& archive);

	ColorB backgroundColor;
	ColorB gridLineColors[2];
};

class CDefaultGraphViewPainter : public IGraphViewPainter
{
public:

	// IGraphViewPainter
	virtual Serialization::SStruct GetSettings() override;
	virtual Gdiplus::Color         GetBackgroundColor() const override;
	virtual void                   PaintGrid(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid) const override;
	virtual void                   UpdateNodePaintRect(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const override;
	virtual void                   PaintNode(Gdiplus::Graphics& graphics, const SGraphViewGrid& grid, CGraphViewNode& node) const override;
	virtual Gdiplus::RectF         PaintLink(Gdiplus::Graphics& graphics, Gdiplus::PointF startPoint, Gdiplus::PointF endPoint, Gdiplus::Color color, bool bHighlight, float scale) const override;
	virtual void                   PaintSelectionRect(Gdiplus::Graphics& graphics, Gdiplus::RectF rect) const override;
	// ~IGraphViewPainter

private:

	Gdiplus::PointF CalculateNodeHeaderSize(Gdiplus::Graphics& graphics, const CGraphViewNode& node) const;
	Gdiplus::PointF CalculateNodeStatusSize(Gdiplus::Graphics& graphics, const CGraphViewNode& node) const;
	Gdiplus::PointF CalculateNodeInputSize(Gdiplus::Graphics& graphics, const char* szName) const;
	Gdiplus::PointF CalculateNodeOutputSize(Gdiplus::Graphics& graphics, const char* szName) const;
	void            PaintNodeBody(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
	void            PaintNodeHeader(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
	void            PaintNodeStatus(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
	void            PaintNodeOutline(Gdiplus::Graphics& graphics, CGraphViewNode& node, Gdiplus::RectF paintRect) const;
	void            UpdateNodeInputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 inputIdx, Gdiplus::PointF pos, float maxWidth) const;
	void            PaintNodeInput(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 inputIdx) const;
	void            PaintNodeInputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, EGraphViewPortStyle style) const;
	void            UpdateNodeOutputPaintRect(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 outputIdx, Gdiplus::PointF pos, float maxWidth) const;
	void            PaintNodeOutput(Gdiplus::Graphics& graphics, CGraphViewNode& node, uint32 outputIdx) const;
	void            PaintNodeOutputIcon(Gdiplus::Graphics& graphics, Gdiplus::PointF pos, Gdiplus::Color color, EGraphViewPortStyle style) const;

private:

	// #SchematycTODO : Do these variables really need to live in the header?

	static const float          NODE_WIDTH_MIN;
	static const float          NODE_HEIGHT_MIN;
	static const float          NODE_BEVEL;
	static const BYTE           NODE_HEADER_ALPHA;
	static const BYTE           NODE_HEADER_ALPHA_DISABLED;
	static const float          NODE_HEADER_TEXT_BORDER_X;
	static const float          NODE_HEADER_TEXT_BORDER_Y;
	static const Gdiplus::Color NODE_HEADER_TEXT_COLOR;
	static const Gdiplus::Color NODE_ERROR_FILL_COLOR;
	static const char*          NODE_ERROR_TEXT;
	static const float          NODE_ERROR_TEXT_BORDER_X;
	static const float          NODE_ERROR_TEXT_BORDER_Y;
	static const Gdiplus::Color NODE_ERROR_TEXT_COLOR;
	static const Gdiplus::Color NODE_WARNING_FILL_COLOR;
	static const char*          NODE_WARNING_TEXT;
	static const float          NODE_WARNING_TEXT_BORDER_X;
	static const float          NODE_WARNING_TEXT_BORDER_Y;
	static const Gdiplus::Color NODE_WARNING_TEXT_COLOR;
	static const Gdiplus::Color NODE_BODY_FILL_COLOR;
	static const Gdiplus::Color NODE_BODY_FILL_COLOR_DISABLED;
	static const Gdiplus::Color NODE_BODY_OUTLINE_COLOR;
	static const Gdiplus::Color NODE_BODY_OUTLINE_COLOR_HIGHLIGHT;
	static const float          NODE_BODY_OUTLINE_WIDTH;
	static const float          NODE_INPUT_OUPUT_HORZ_SPACING;
	static const float          NODE_INPUT_OUPUT_VERT_SPACING;
	static const float          NODE_INPUT_ICON_BORDER;
	static const float          NODE_INPUT_ICON_WIDTH;
	static const float          NODE_INPUT_ICON_HEIGHT;
	static const Gdiplus::Color NODE_INPUT_ICON_OUTLINE_COLOR;
	static const float          NODE_INPUT_ICON_OUTLINE_WIDTH;
	static const float          NODE_INPUT_NAME_BORDER;
	static const float          NODE_INPUT_NAME_WIDTH_MAX;
	static const Gdiplus::Color NODE_INPUT_NAME_COLOR;
	static const float          NODE_OUTPUT_ICON_BORDER;
	static const float          NODE_OUTPUT_ICON_WIDTH;
	static const float          NODE_OUTPUT_ICON_HEIGHT;
	static const Gdiplus::Color NODE_OUTPUT_ICON_OUTLINE_COLOR;
	static const float          NODE_OUTPUT_ICON_OUTLINE_WIDTH;
	static const float          NODE_OUTPUT_NAME_BORDER;
	static const float          NODE_OUTPUT_NAME_WIDTH_MAX;
	static const Gdiplus::Color NODE_OUTPUT_NAME_COLOR;
	static const float          NODE_OUTPUT_SPACER_OFFSET;
	static const float          NODE_OUTPUT_SPACER_WIDTH;
	static const float          NODE_OUTPUT_SPACER_LINE_WIDTH;
	static const Gdiplus::Color NODE_OUTPUT_SPACER_COLOR;
	static const float          LINK_WIDTH;
	static const float          LINK_CURVE_OFFSET;
	static const float          LINK_DODGE_X;
	static const float          LINK_DODGE_Y;
	static const float          ALPHA_HIGHLIGHT_MIN;
	static const float          ALPHA_HIGHLIGHT_MAX;
	static const float          ALPHA_HIGHLIGHT_SPEED;
	static const Gdiplus::Color SELECTION_FILL_COLOR;
	static const Gdiplus::Color SELECTION_OUTLINE_COLOR;

private:

	SDefaultGraphViewPainterSettings m_settings;
};

class CGraphView : public CView
{
	friend struct SGraphViewLink;

	DECLARE_MESSAGE_MAP()

public:

	virtual BOOL                Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	virtual void                Refresh();
	SGraphViewSettings&         GetSettings();
	const SGraphViewGrid&       GetGrid() const;
	const IGraphViewPainterPtr& GetPainter() const;
	TGraphViewLinkVector&       GetLinks();
	const TGraphViewLinkVector& GetLinks() const;

	void                        DoQuickSearch(const char* szDefaultFilter = nullptr, const CGraphViewNodePtr& pNode = CGraphViewNodePtr(), uint32 outputIdx = InvalidIdx, const CPoint* pPoint = nullptr);

protected:

	struct EPopupMenuItem
	{
		enum
		{
			QUICK_SEARCH = 1,
			COPY_NODE,
			REMOVE_NODE,
			CUSTOM
		};
	};

	CGraphView(const SGraphViewGrid& grid, const IGraphViewPainterPtr& pPainter = IGraphViewPainterPtr());

	void                               Enable(bool enable);
	bool                               IsEnabled() const;
	void                               ClearSelection();
	void                               SetScrollOffset(Gdiplus::PointF scrollOffset);
	void                               AddNode(const CGraphViewNodePtr& pNode, bool select, bool scrollToFit);
	void                               RemoveNode(CGraphViewNodePtr pNode);
	CGraphViewNodePtr                  FindNode(Gdiplus::PointF pos);
	TGraphViewNodePtrVector&           GetNodes();
	const TGraphViewNodePtrVector&     GetNodes() const;
	void                               ClearNodeSelection();
	void                               SelectNode(const CGraphViewNodePtr& pNode);
	void                               SelectNodesInRect(const Gdiplus::RectF& rect);
	const TGraphViewNodePtrVector&     GetSelectedNodes() const;
	void                               MoveSelectedNodes(Gdiplus::PointF clientTransform, bool bSnapToGrid);
	bool                               CreateLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId);
	void                               AddLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId);
	void                               RemoveLink(uint32 iLink);
	void                               RemoveLinksConnectedToNode(const CGraphViewNodePtr& pNode);
	void                               RemoveLinksConnectedToNodeInput(const CGraphViewNodePtr& pNode, const CGraphPortId& inputId);
	void                               RemoveLinksConnectedToNodeOutput(const CGraphViewNodePtr& pNode, const CGraphPortId& outputId);
	void                               FindLinks(const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId, UInt32Vector& output) const;
	uint32                             FindLink(Gdiplus::PointF point);
	uint32                             FindLink(const CGraphViewNodePtr& pSrcNode, const CGraphPortId& srcOutputId, const CGraphViewNodePtr& pDstNode, const CGraphPortId& dstInputId);
	void                               ClearLinkSelection();
	void                               SelectLink(uint32 iLink);
	uint32                             GetSelectedLink() const;
	void                               Clear();

	Gdiplus::PointF                    ClientToGraph(LONG x, LONG y) const;
	Gdiplus::PointF                    ClientToGraph(Gdiplus::PointF pos) const;
	Gdiplus::PointF                    ClientToGraphTransform(Gdiplus::PointF transform) const;
	Gdiplus::RectF                     ClientToGraph(Gdiplus::RectF rect) const;
	Gdiplus::PointF                    GraphToClient(Gdiplus::PointF pos) const;
	Gdiplus::PointF                    GraphToClientTransform(Gdiplus::PointF transform) const;
	Gdiplus::RectF                     GraphToClient(Gdiplus::RectF rect) const;
	void                               ScrollToFit(Gdiplus::RectF rect);
	void                               CenterPosition(Gdiplus::PointF point);

	virtual BOOL                       PreTranslateMessage(MSG* pMsg);
	virtual void                       OnDraw(CDC* pDC);

	afx_msg void                       OnKillFocus(CWnd* pNewWnd);
	afx_msg void                       OnPaint();
	afx_msg void                       OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL                       OnMouseWheel(UINT nFlags, short zDelta, CPoint point);
	afx_msg void                       OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void                       OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void                       OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void                       OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void                       OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void                       OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void                       OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void                       OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL                       OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	virtual void                       OnMove(Gdiplus::PointF scrollOffset);
	virtual void                       OnSelection(const CGraphViewNodePtr& pSelectedNode, SGraphViewLink* pSelectedLink);
	virtual void                       OnLinkModify(const SGraphViewLink& link);
	virtual void                       OnNodeRemoved(const CGraphViewNode& node);
	virtual bool                       CanCreateLink(const CGraphViewNode& srcNode, const CGraphPortId& srcOutputId, const CGraphViewNode& dstNode, const CGraphPortId& dstInputId) const;
	virtual void                       OnLinkCreated(const SGraphViewLink& link);
	virtual void                       OnLinkRemoved(const SGraphViewLink& link);
	virtual DROPEFFECT                 OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL                       OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual void                       GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, uint32 iNodeInput, uint32 iNodeOutput, CPoint point);
	virtual void                       OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, uint32 iNodeInput, uint32 iNodeOutput, CPoint point);
	virtual const IQuickSearchOptions* GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, uint32 iNodeOutput);
	virtual void                       OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, uint32 iNodeOutput, uint32 iSelectedOption);
	virtual bool                       ShouldUpdate() const;
	virtual void                       OnUpdate();

private:

	class CScrollDragState : public IGraphViewDragState
	{
	public:

		CScrollDragState(Gdiplus::PointF pos);

		// IDragState
		virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
		virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
		virtual void Exit(CGraphView& graphView);
		// ~IDragState

	private:

		Gdiplus::PointF m_prevPos;
	};

	class CMoveDragState : public IGraphViewDragState
	{
	public:

		CMoveDragState(Gdiplus::PointF pos);

		// IDragState
		virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
		virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
		virtual void Exit(CGraphView& graphView);
		// ~IDragState

	private:

		Gdiplus::PointF m_prevPos;
	};

	class CLinkDragState : public IGraphViewDragState
	{
	public:

		CLinkDragState(const CGraphViewNodeWeakPtr& pSrcNode, const CGraphPortId& srcOutputId, Gdiplus::PointF start, Gdiplus::PointF end, Gdiplus::Color color, bool bNewLink);

		// IDragState
		virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
		virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
		virtual void Exit(CGraphView& graphView);
		// ~IDragState

	private:

		CGraphViewNodeWeakPtr m_pSrcNode;
		CGraphPortId          m_srcOutputId;
		Gdiplus::PointF       m_start;
		Gdiplus::PointF       m_end;
		Gdiplus::Color        m_color;
		bool                  m_bNewLink;
	};

	class CSelectDragState : public IGraphViewDragState
	{
	public:

		CSelectDragState(Gdiplus::PointF pos);

		// IDragState
		virtual void Update(CGraphView& graphView, Gdiplus::PointF pos);
		virtual void Paint(CGraphView& graphView, Gdiplus::Graphics& graphics, IGraphViewPainter& painter);
		virtual void Exit(CGraphView& graphView);
		// ~IDragState

	private:

		Gdiplus::RectF GetSelectionRect() const;

		Gdiplus::PointF m_startPos;
		Gdiplus::PointF m_endPos;
	};

	class CDropTarget : public COleDropTarget
	{
	public:

		CDropTarget(CGraphView& graphView);

		// COleDropTarget
		virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
		virtual BOOL       OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
		// ~COleDropTarget

	private:

		CGraphView& m_graphView;
	};

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	void         OnLeftMouseButtonDrag(CPoint point);
	void         OnRightMouseButtonDrag(CPoint point);
	void         OnMiddleMouseButtonDrag(CPoint point);
	void         Scroll(Gdiplus::PointF delta);
	void         ExitDragState();
	void         OnDelete();
	bool         UpdateCursor();
	void         FormatSelectedNodes();

	SGraphViewSettings      m_settings;
	SGraphViewGrid          m_grid;
	IGraphViewPainterPtr    m_pPainter;
	bool                    m_enabled;
	TGraphViewNodePtrVector m_nodes;
	TGraphViewLinkVector    m_links;
	Gdiplus::PointF         m_scrollOffset;
	CPoint                  m_prevMousePoint;
	float                   m_zoom;
	IGraphViewDragStatePtr  m_pDragState;
	TGraphViewNodePtrVector m_selectedNodes;
	uint32                  m_iSelectedLink;
	CDropTarget             m_dropTarget;

	static const float      MIN_ZOOM;
	static const float      MAX_ZOOM;
	static const float      DELTA_ZOOM;
	static const float      DEFAULT_NODE_SPACING;
};
} // Schematyc
