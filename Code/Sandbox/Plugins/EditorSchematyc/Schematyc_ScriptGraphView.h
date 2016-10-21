// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <CrySerialization/Forward.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>
#include <Schematyc/Utils/Schematyc_ScopedConnection.h>
#include <Schematyc/Utils/Schematyc_Signal.h>

#include "Schematyc_PluginUtils.h"
#include "Schematyc_GraphView.h"

class QBoxLayout;
class QSplitter;
class QPushButton;
class QPropertyTree;
class QParentWndWidget;
class QWinHost;

namespace Schematyc
{
class CScriptGraphViewNode : public CGraphViewNode
{
public:

	CScriptGraphViewNode(const SGraphViewGrid& grid, Gdiplus::PointF pos, IScriptGraph& scriptGraph, IScriptGraphNode& scriptGraphNode);

	// CGraphViewNode
	virtual const char*          GetName() const override;
	virtual Gdiplus::Color       GetHeaderColor() const override;
	virtual bool                 IsRemoveable() const override;
	virtual uint32               GetInputCount() const override;
	virtual uint32               FindInputById(const CGraphPortId& id) const override;
	virtual CGraphPortId         GetInputId(uint32 inputIdx) const override;
	virtual const char*          GetInputName(uint32 inputIdx) const override;
	virtual ScriptGraphPortFlags GetInputFlags(uint32 inputIdx) const override;
	virtual Gdiplus::Color       GetInputColor(uint32 inputIdx) const override;
	virtual uint32               GetOutputCount() const override;
	virtual uint32               FindOutputById(const CGraphPortId& id) const override;
	virtual CGraphPortId         GetOutputId(uint32 outputIdx) const override;
	virtual const char*          GetOutputName(uint32 outputIdx) const override;
	virtual ScriptGraphPortFlags GetOutputFlags(uint32 outputIdx) const override;
	virtual Gdiplus::Color       GetOutputColor(uint32 outputIdx) const override;
	virtual void                 CopyToXml(IString& output) const override;
	// ~CGraphViewNode

	IScriptGraphNode&       GetScriptGraphNode();
	const IScriptGraphNode& GetScriptGraphNode() const;
	SGUID                   GetGUID() const;
	void                    Serialize(Serialization::IArchive& archive);

protected:

	virtual void OnMove(Gdiplus::RectF paintRect);

private:

	IScriptGraph&     m_scriptGraph;
	IScriptGraphNode& m_scriptGraphNode;
};

DECLARE_SHARED_POINTERS(CScriptGraphViewNode)

struct SScriptGraphViewSelection
{
	SScriptGraphViewSelection(const CScriptGraphViewNodePtr& _pNode, SGraphViewLink* _pLink);

	CScriptGraphViewNodePtr pNode;
	SGraphViewLink*         pLink;
};

typedef CSignal<void, const SScriptGraphViewSelection&> ScriptGraphViewSelectionSignal;
typedef CSignal<void>                                 ScriptGraphViewModificationSignal;

class CScriptGraphView : public CGraphView
{
	DECLARE_MESSAGE_MAP()

private:

	struct SSignals
	{
		ScriptGraphViewSelectionSignal    selection;
		ScriptGraphViewModificationSignal modification;
	};

public:

	CScriptGraphView();

	// CGraphView
	virtual void Refresh() override;
	// ~CGraphView

	void                                      Load(IScriptGraph* pScriptGraph);
	IScriptGraph*                             GetScriptGraph() const;
	void                                      SelectAndFocusNode(const SGUID& nodeGuid);

	ScriptGraphViewSelectionSignal::Slots&    GetSelectionSignalSlots();
	ScriptGraphViewModificationSignal::Slots& GetModificationSignalSlots();

protected:

	// CGraphView
	virtual void                       OnMove(Gdiplus::PointF scrollOffset) override;
	virtual void                       OnSelection(const CGraphViewNodePtr& pSelectedNode, SGraphViewLink* pSelectedLink) override;
	virtual void                       OnLinkModify(const SGraphViewLink& link) override;
	virtual void                       OnNodeRemoved(const CGraphViewNode& node);
	virtual bool                       CanCreateLink(const CGraphViewNode& srcNode, const CGraphPortId& srcOutputId, const CGraphViewNode& dstNode, const CGraphPortId& dstInputId) const override;
	virtual void                       OnLinkCreated(const SGraphViewLink& link) override;
	virtual void                       OnLinkRemoved(const SGraphViewLink& link) override;
	virtual DROPEFFECT                 OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) override;
	virtual BOOL                       OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) override;
	void                               GetPopupMenuItems(CMenu& popupMenu, const CGraphViewNodePtr& pNode, uint32 nodeInputIdx, uint32 nodeOutputIdx, CPoint point) override;
	virtual void                       OnPopupMenuResult(BOOL popupMenuItem, const CGraphViewNodePtr& pNode, uint32 nodeInputIdx, uint32 nodeOutputIdx, CPoint point) override;
	virtual const IQuickSearchOptions* GetQuickSearchOptions(CPoint point, const CGraphViewNodePtr& pNode, uint32 nodeOutputIdx) override;
	virtual void                       OnQuickSearchResult(CPoint point, const CGraphViewNodePtr& pNode, uint32 nodeOutputIdx, uint32 optionIdx) override;
	// ~CGraphView

private:

	struct EPopupMenuItemEx
	{
		enum
		{
			ADD_OUTPUT = EPopupMenuItem::CUSTOM,
			REMOVE_OUTPUT
		};
	};

	class CQuickSearchOptions : public IQuickSearchOptions, public IScriptGraphNodeCreationMenu
	{
	public:

		// IQuickSearchOptions
		virtual uint32      GetCount() const override;
		virtual const char* GetLabel(uint32 optionIdx) const override;
		virtual const char* GetDescription(uint32 optionIdx) const override;
		virtual const char* GetWikiLink(uint32 optionIdx) const override;
		virtual const char* GetHeader() const override;
		virtual const char* GetDelimiter() const override;
		// IQuickSearchOptions

		// IScriptGraphNodeCreationMenuCommand
		virtual bool AddOption(const char* szLabel, const char* szDescription, const char* szWikiLink, const IScriptGraphNodeCreationMenuCommandPtr& pCommand) override;
		// ~IScriptGraphNodeCreationMenuCommand

		void                Refresh(IScriptGraph& scriptGraph);
		SGUID               GetNodeContextGUID(uint32 optionIdx) const;
		SGUID               GetNodeRefGUID(uint32 optionIdx) const;
		IScriptGraphNodePtr ExecuteCommand(uint32 optionIdx, const Vec2& pos);

	private:

		struct SOption
		{
			SOption(const char* _szLabel, const char* _szDescription, const char* _szWikiLink, const IScriptGraphNodeCreationMenuCommandPtr& _pCommand);

			string label;
			string description;
			string wikiLink;
			SGUID  nodeContextGUID;
			SGUID  nodeRefGUID;
			IScriptGraphNodeCreationMenuCommandPtr pCommand;
		};

		typedef std::vector<SOption> Options;

		Options m_options;
	};

	typedef std::vector<SGUID> SequenceNodes;

	EVisitStatus            VisitGraphNode(IScriptGraphNode& graphNode);

	void                    OnScriptRegistryChange(const SScriptRegistryChange& change);

	CScriptGraphViewNodePtr AddNode(IScriptGraphNode& scriptGraphNode);
	CScriptGraphViewNodePtr GetNode(const SGUID& guid) const;
	bool                    AddLink(const IScriptGraphLink& scriptGraphLink);

	void                    InvalidateScriptGraph();
	void                    ValidateNodes();
	void                    EnableConnectedNodes();
	void                    OnScriptGraphLinkRemoved(const IScriptGraphLink& link);

	IScriptGraph*       m_pScriptGraph;
	bool                m_bRemovingLinks; // #SchematycTODO : This is a little hacky and we need to define a clear chain of communication between script graph and script graph view.
	CQuickSearchOptions m_quickSearchOptions;
	CConnectionScope    m_connectionScope;
	SSignals            m_signals;
};

class CGraphSettingsWidget : public QWidget
{
	Q_OBJECT

public:

	CGraphSettingsWidget(QWidget* pParent);

	void SetGraphView(CGraphView* pGraphView);
	void InitLayout();
	void Serialize(Serialization::IArchive& archive);

private:

	CGraphView*    m_pGraphView;
	QBoxLayout*    m_pLayout;
	QPropertyTree* m_pPropertyTree;
};

class CScriptGraphWidget : public QWidget
{
	Q_OBJECT

public:

	CScriptGraphWidget(QWidget* pParent, CWnd* pParentWnd);

	void                                      InitLayout();
	void                                      LoadSettings(const XmlNodeRef& xml);
	XmlNodeRef                                SaveSettings(const char* szName);
	void                                      Refresh();
	void                                      LoadGraph(IScriptGraph* pScriptGraph);
	IScriptGraph*                             GetScriptGraph() const;
	void                                      SelectAndFocusNode(const SGUID& nodeGuid);

	ScriptGraphViewSelectionSignal::Slots&    GetSelectionSignalSlots();
	ScriptGraphViewModificationSignal::Slots& GetModificationSignalSlots();

public slots:

	void OnShowHideSettingsButtonClicked();

private:

	QBoxLayout*           m_pMainLayout;
	QSplitter*            m_pSplitter;
	CGraphSettingsWidget* m_pSettings;
	QWinHost*             m_pGraphViewHost;
	CScriptGraphView*     m_pGraphView;
	QBoxLayout*           m_pControlLayout;
	QPushButton*          m_pShowHideSettingsButton;
};
} // Schematyc
