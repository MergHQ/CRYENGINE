// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// TODO:
// - tweak size of the treeview captions

#include "StdAfx.h"
#include "MainEditorWindow.h"

#include <QDir>
#include <QMenuBar>
#include <QFileDialog>
#include <QDockWidget>
#include <QTabWidget>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QAbstractButton>
#include <QDialogButtonbox>
#include <QLineEdit>
#include <QTreeView>
#include <QSplitter>
#include <QMessageBox>
#include <QFontMetrics>

#include <QtUtil.h>

#include <QPropertyTree/QPropertyTree.h>
#include <Controls/QuestionDialog.h>

// UQS 3d rendering
#include <IDisplayViewport.h>

//#pragma optimize("", off)

//===================================================================================
//
// SQuery - the data of the model
//
//===================================================================================

struct SQuery
{
	// columns
	enum
	{
		Column_QueryIdAndQuerierName = 0,
		Column_QueryBlueprintName,
		// TODO: column for displaying the itemType of the items in the result set
		Column_ItemCounts, // number of resulting items vs. generated items
		Column_ElapsedTime,

		ColumnCount
	};

	static const char* headers[ColumnCount];
	static const char* toolTips[ColumnCount];

	// control data
	SQuery*              pParent;
	std::vector<SQuery*> children;
	uqs::core::CQueryID  queryID;

	// display data
	QVariant dataPerColumn[ColumnCount];

	SQuery() : pParent(nullptr), queryID(uqs::core::CQueryID::CreateInvalid()) {}
	SQuery(SQuery* _pParent, const uqs::core::IQueryHistoryConsumer::SHistoricQueryOverview& overview)
		: pParent(_pParent)
		, queryID(overview.queryID)
	{
		uqs::shared::CUqsString queryIdAsString;
		stack_string queryIdAndQuerierName;
		stack_string itemCountsAsString;
		stack_string elapsedTimeAsString;

		overview.queryID.ToString(queryIdAsString);
		queryIdAndQuerierName.Format("#%s: %s", queryIdAsString.c_str(), overview.querierName);
		itemCountsAsString.Format("%i / %i", (int)overview.numResultingItems, (int)overview.numGeneratedItems);
		elapsedTimeAsString.Format("%.2f ms", overview.timeElapsedUntilResult.GetMilliSeconds());

		dataPerColumn[Column_QueryIdAndQuerierName] = QtUtil::ToQString(queryIdAndQuerierName.c_str());
		dataPerColumn[Column_QueryBlueprintName] = QtUtil::ToQString(overview.queryBlueprintName);
		dataPerColumn[Column_ItemCounts] = QtUtil::ToQString(itemCountsAsString.c_str());
		dataPerColumn[Column_ElapsedTime] = QtUtil::ToQString(elapsedTimeAsString.c_str());
	}

	~SQuery()
	{
		for (SQuery* pChild : children)
			delete pChild;
	}

	static SQuery* FindQueryByQueryID(SQuery* pStart, const uqs::core::CQueryID& queryID)
	{
		if (pStart->queryID == queryID)
			return pStart;

		for (SQuery* pChild : pStart->children)
		{
			if (SQuery* pPotentialFind = FindQueryByQueryID(pChild, queryID))
				return pPotentialFind;
		}

		return nullptr;
	}
};

const char* SQuery::headers[SQuery::ColumnCount] =
{
	"Query ID + querier",         // Column_QueryIdAndQuerierName
	"Query Blueprint",            // Column_QueryBlueprintName
	"Items (accepted/generated)", // Column_ItemCounts
	"Elapsed time"                // Column_ElapsedTime,
};

const char* SQuery::toolTips[SQuery::ColumnCount] =
{
	"Unique id of the query instance and the name of who started that query",                                                                                                                           // Column_QueryIdAndQuerierName
	"Name of the blueprint from which the live query was instantiated",                                                                                                                                 // Column_QueryBlueprintName
	"Number of items that were generated and ended up in the final result set. Notice: a hierarchical query may not necessarily generate items, yet grab the resulting items from one of its children", // Column_ItemCounts
	"Elapsed time from start to finish of the query. Notice: don't confuse with *consumed* time."                                                                                                       // Column_ElapsedTime,
};

//===================================================================================
//
// CHistoricQueryTreeModel
//
//===================================================================================

class CHistoricQueryTreeModel : public QAbstractItemModel
{
public:
	explicit CHistoricQueryTreeModel(QObject* pParent)
		: QAbstractItemModel(pParent)
	{}

	Q_INVOKABLE virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
	{
		assert(row >= 0 && column >= 0);

		const SQuery* pParentQuery;

		if (parent.isValid())
		{
			pParentQuery = static_cast<const SQuery*>(parent.internalPointer());
		}
		else
		{
			pParentQuery = &m_root;
		}

		if (row < (int)pParentQuery->children.size())
		{
			const SQuery* pChildQuery = pParentQuery->children[row];
			return createIndex(row, column, reinterpret_cast<quintptr>(pChildQuery));
		}
		else
		{
			return QModelIndex();
		}
	}

	Q_INVOKABLE virtual QModelIndex parent(const QModelIndex& child) const override
	{
		if (!child.isValid())
		{
			return QModelIndex();
		}

		const SQuery* pChildQuery = static_cast<SQuery*>(child.internalPointer());
		const SQuery* pParentQuery = pChildQuery->pParent;
		assert(pParentQuery);

		if (pParentQuery == &m_root)
		{
			return QModelIndex();
		}

		for (size_t i = 0, n = pParentQuery->children.size(); i < n; ++i)
		{
			if (pParentQuery->children[i] == pChildQuery)
			{
				return createIndex((int)i, 0, reinterpret_cast<quintptr>(pParentQuery));
			}
		}

		assert(0);
		return QModelIndex();
	}

	Q_INVOKABLE virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		if (parent.column() > 0)
		{
			return 0;
		}

		const SQuery* pParentQuery;

		if (parent.isValid())
		{
			pParentQuery = static_cast<const SQuery*>(parent.internalPointer());
		}
		else
		{
			pParentQuery = &m_root;
		}

		return (int)pParentQuery->children.size();
	}

	Q_INVOKABLE virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return SQuery::ColumnCount;
	}

	Q_INVOKABLE virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
	{
		if (role == Qt::DisplayRole)
		{
			if (index.isValid())
			{
				const SQuery* pQuery = static_cast<const SQuery*>(index.internalPointer());
				return pQuery->dataPerColumn[index.column()];
			}
		}
		return QVariant();
	}

	Q_INVOKABLE virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
	{
		switch (role)
		{
		case Qt::DisplayRole:
			assert(section >= 0 && section < SQuery::ColumnCount);
			return SQuery::headers[section];

		case Qt::ToolTipRole:
			assert(section >= 0 && section < SQuery::ColumnCount);
			return SQuery::toolTips[section];

		default:
			return QAbstractItemModel::headerData(section, orientation, role);
		}
	}

public:
	void AddHistoricQuery(const uqs::core::IQueryHistoryConsumer::SHistoricQueryOverview& overview)
	{
		SQuery* pParent;

		if (overview.parentQueryID.IsValid())
		{
			pParent = SQuery::FindQueryByQueryID(&m_root, overview.parentQueryID);
			assert(pParent);
		}
		else
		{
			pParent = &m_root;
		}

		SQuery* pNewQuery = new SQuery(pParent, overview);
		pParent->children.push_back(pNewQuery);

		// TODO: don't call them for every added historic query when the whole query history is being built up
		beginResetModel();
		endResetModel();
	}

	void ClearAllHistoricQueries()
	{
		beginResetModel();
		endResetModel();

		for (SQuery* pChild : m_root.children)
			delete pChild;
		m_root.children.clear();
	}

private:
	SQuery m_root;
};

//===================================================================================
//
// CHistoricQueryTreeView
//
//===================================================================================

class CHistoricQueryTreeView : public QTreeView
{
public:
	explicit CHistoricQueryTreeView(QWidget* pParent)
		: QTreeView(pParent)
	{
	}

protected:
	virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous) override
	{
		QTreeView::currentChanged(current, previous);

		uqs::core::IQueryHistoryManager* pHistoryQueryManager = GetHistoryQueryManager();

		if (!pHistoryQueryManager)
			return;

		if (current.isValid())
		{
			const SQuery* pQueryToSwitchTo = static_cast<const SQuery*>(current.internalPointer());
			const uqs::core::IQueryHistoryManager::EHistoryOrigin whichHistory = pHistoryQueryManager->GetCurrentQueryHistory();
			pHistoryQueryManager->MakeHistoricQueryCurrentForInWorldRendering(whichHistory, pQueryToSwitchTo->queryID);
		}
	}

private:
	uqs::core::IQueryHistoryManager* GetHistoryQueryManager()
	{
		if (uqs::core::IHub* pHub = uqs::core::IHubPlugin::GetHubPtr())
		{
			return &pHub->GetQueryHistoryManager();
		}
		else
		{
			return nullptr;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// CMainEditorWindow
//////////////////////////////////////////////////////////////////////////

CMainEditorWindow::CMainEditorWindow()
	: m_pQueryHistoryManager(nullptr)
{
	GetIEditor()->RegisterNotifyListener(this);

	// "file" menu
	{
		QMenu* pFileMenu = menuBar()->addMenu("&File");

		QAction* pSaveLiveHistory = pFileMenu->addAction("&Save 'live' history");
		connect(pSaveLiveHistory, &QAction::triggered, this, &CMainEditorWindow::OnSaveLiveHistoryToFile);

		QAction* pLoadHistory = pFileMenu->addAction("&Load history");
		connect(pLoadHistory, &QAction::triggered, this, &CMainEditorWindow::OnLoadHistoryFromFile);
	}

	m_pComboBoxHistoryOrigin = new QComboBox(this);
	m_pComboBoxHistoryOrigin->addItem("Live history", QVariant(uqs::core::IQueryHistoryManager::EHistoryOrigin::Live));
	m_pComboBoxHistoryOrigin->addItem("Deserialized history", QVariant(uqs::core::IQueryHistoryManager::EHistoryOrigin::Deserialized));

	m_pButtonClearCurrentHistory = new QPushButton(this);
	m_pButtonClearCurrentHistory->setText("Clear history");

	m_pTreeView = new CHistoricQueryTreeView(this);
	m_pTreeModel = new CHistoricQueryTreeModel(this);
	m_pTreeView->setModel(m_pTreeModel);

	m_pTextQueryDetails = new QTextEdit(this);
	m_pTextQueryDetails->setText("(Query details will go here)");

	m_pTextItemDetails = new QTextEdit(this);
	m_pTextItemDetails->setText("(Item details will go here)");

	QSplitter* pDetailsSplitter = new QSplitter(this);
	pDetailsSplitter->setOrientation(Qt::Vertical);
	pDetailsSplitter->addWidget(m_pTextQueryDetails);
	pDetailsSplitter->addWidget(m_pTextItemDetails);

	QHBoxLayout* pHBoxLayout = new QHBoxLayout(this);
	pHBoxLayout->addWidget(m_pComboBoxHistoryOrigin);
	pHBoxLayout->addWidget(m_pButtonClearCurrentHistory);

	QWidget* pHorzContainer = new QWidget(this);
	pHorzContainer->setLayout(pHBoxLayout);

	QVBoxLayout* pVBoxLayout = new QVBoxLayout(this);
	pVBoxLayout->addWidget(pHorzContainer);
	pVBoxLayout->addWidget(m_pTreeView);

	QWidget* pLeftContainer = new QWidget(this);
	pLeftContainer->setLayout(pVBoxLayout);

	QSplitter* pMainSplitter = new QSplitter(this);
	pMainSplitter->addWidget(pLeftContainer);
	pMainSplitter->addWidget(pDetailsSplitter);

	setCentralWidget(pMainSplitter);

	QObject::connect(m_pComboBoxHistoryOrigin, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CMainEditorWindow::OnHistoryOriginComboBoxSelectionChanged);	// the static_cast<> is only for disambiguation of the overloaded currentIndexChanged() method
	QObject::connect(m_pButtonClearCurrentHistory, &QPushButton::clicked, this, &CMainEditorWindow::OnClearHistoryButtonClicked);

	if (uqs::core::IHub* pHub = uqs::core::IHubPlugin::GetHubPtr())
	{
		m_pQueryHistoryManager = &pHub->GetQueryHistoryManager();
		m_pQueryHistoryManager->RegisterQueryHistoryListener(this);
		m_pQueryHistoryManager->EnumerateHistoricQueries(m_pQueryHistoryManager->GetCurrentQueryHistory(), *this);
	}
	else
	{
		// disable everything
		QMessageBox::warning(this, "UQS engine-plugin not loaded", "The UQS History Inspector will be disabled as the UQS core engine-plugin hasn't been loaded.");
		setEnabled(false);
	}
}

CMainEditorWindow::~CMainEditorWindow()
{
	if (m_pQueryHistoryManager)
	{
		m_pQueryHistoryManager->UnregisterQueryHistoryListener(this);
	}

	GetIEditor()->UnregisterNotifyListener(this);
}

const char* CMainEditorWindow::GetPaneTitle() const
{
	// check for whether the UQS engine plugin has been loaded
	if (uqs::core::IHubPlugin::GetHubPtr())
	{
		return "UQS Query History Inspector";
	}
	else
	{
		return "UQS Query History Inspector (disabled due to the UQS engine-plugin not being loaded)";
	}
}

void CMainEditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
	switch (ev)
	{
	case eNotify_OnIdleUpdate:
		if (m_pQueryHistoryManager)
		{
			if (IDisplayViewport* pActiveDisplayViewport = GetIEditor()->GetActiveDisplayViewport())
			{
				const Matrix34& viewTM = pActiveDisplayViewport->GetViewTM();
				Matrix33 orientation;
				viewTM.GetRotation33(orientation);
				uqs::core::SDebugCameraView uqsCameraView;
				uqsCameraView.pos = viewTM.GetTranslation();
				uqsCameraView.dir = orientation * Vec3(0, 1, 0);
				m_pQueryHistoryManager->UpdateDebugRendering3D(uqsCameraView);
			}
		}
		break;
	}
}

void CMainEditorWindow::OnQueryHistoryEvent(EEvent ev)
{
	switch (ev)
	{
	case EEvent::HistoricQueryJustFinishedInLiveQueryHistory:
		if (m_pQueryHistoryManager->GetCurrentQueryHistory() == uqs::core::IQueryHistoryManager::EHistoryOrigin::Live)
		{
			m_pTreeModel->ClearAllHistoricQueries();
			m_pQueryHistoryManager->EnumerateHistoricQueries(m_pQueryHistoryManager->GetCurrentQueryHistory(), *this);
		}
		break;

	case EEvent::CurrentQueryHistorySwitched:
		m_pTreeModel->ClearAllHistoricQueries();
		m_pQueryHistoryManager->EnumerateHistoricQueries(m_pQueryHistoryManager->GetCurrentQueryHistory(), *this);
		break;

	case EEvent::DeserializedQueryHistoryCleared:
		if (m_pQueryHistoryManager->GetCurrentQueryHistory() == uqs::core::IQueryHistoryManager::EHistoryOrigin::Deserialized)
		{
			m_pTreeModel->ClearAllHistoricQueries();
		}
		break;

	case EEvent::LiveQueryHistoryCleared:
		if (m_pQueryHistoryManager->GetCurrentQueryHistory() == uqs::core::IQueryHistoryManager::EHistoryOrigin::Live)
		{
			m_pTreeModel->ClearAllHistoricQueries();
		}
		break;

	case EEvent::DifferentHistoricQuerySelected:
		{
			const uqs::core::IQueryHistoryManager::EHistoryOrigin historyOrigin = m_pQueryHistoryManager->GetCurrentQueryHistory();
			const uqs::core::CQueryID queryID = m_pQueryHistoryManager->GetCurrentHistoricQueryForInWorldRendering(historyOrigin);
			m_pTextQueryDetails->clear();
			m_pTextItemDetails->clear();   // get rid of some leftovers in case no item will be focused anymore
			m_pQueryHistoryManager->GetDetailsOfHistoricQuery(historyOrigin, queryID, *this);
		}
		break;

	case EEvent::FocusedItemChanged:
		{
			m_pTextItemDetails->clear();
			m_pQueryHistoryManager->GetDetailsOfFocusedItem(*this);
		}
		break;

	case EEvent::QueryHistoryDeserialized:
		if (m_pQueryHistoryManager->GetCurrentQueryHistory() == uqs::core::IQueryHistoryManager::EHistoryOrigin::Deserialized)
		{
			m_pTreeModel->ClearAllHistoricQueries();
			m_pQueryHistoryManager->EnumerateHistoricQueries(m_pQueryHistoryManager->GetCurrentQueryHistory(), *this);
		}
		break;
	}
}

void CMainEditorWindow::AddHistoricQuery(const SHistoricQueryOverview& overview)
{
	m_pTreeModel->AddHistoricQuery(overview);
}

void CMainEditorWindow::AddTextLineToCurrentHistoricQuery(const ColorF& color, const char* fmt, ...)
{
	stack_string tmpText;
	va_list ap;
	va_start(ap, fmt);
	tmpText.FormatV(fmt, ap);
	va_end(ap);

	const unsigned int rgb888 = color.pack_rgb888();
	stack_string html;
	html.Format("<font color='#%06x'>%s</font><br>", rgb888, tmpText.c_str());

	const QString qHtml = QtUtil::ToQString(html.c_str());
	m_pTextQueryDetails->insertHtml(qHtml);
}

void CMainEditorWindow::AddTextLineToFocusedItem(const ColorF& color, const char* fmt, ...)
{
	stack_string tmpText;
	va_list ap;
	va_start(ap, fmt);
	tmpText.FormatV(fmt, ap);
	va_end(ap);

	const unsigned int rgb888 = color.pack_rgb888();
	stack_string html;
	html.Format("<font color='#%06x'>%s</font><br>", rgb888, tmpText.c_str());

	const QString qHtml = QtUtil::ToQString(html.c_str());
	m_pTextItemDetails->insertHtml(qHtml);
}

void CMainEditorWindow::OnHistoryOriginComboBoxSelectionChanged(int index)
{
	if (m_pQueryHistoryManager)
	{
		const QVariant data = m_pComboBoxHistoryOrigin->itemData(index);
		assert(data.type() == QVariant::Int);
		const uqs::core::IQueryHistoryManager::EHistoryOrigin origin = (uqs::core::IQueryHistoryManager::EHistoryOrigin)data.toInt();
		m_pQueryHistoryManager->MakeQueryHistoryCurrent(origin);
	}
}

void CMainEditorWindow::OnClearHistoryButtonClicked(bool checked)
{
	if (m_pQueryHistoryManager)
	{
		m_pQueryHistoryManager->ClearQueryHistory(m_pQueryHistoryManager->GetCurrentQueryHistory());
	}
}

void CMainEditorWindow::OnSaveLiveHistoryToFile()
{
	QString qFilePath = QFileDialog::getSaveFileName(this, "Save File", "uqs_history.xml", "XML file (*.xml)");
	string sFilePath = QtUtil::ToString(qFilePath);
	if (!sFilePath.empty())  // would be empty if pressing the "cancel" button
	{
		uqs::shared::CUqsString uqsErrorMessage;
		if (!m_pQueryHistoryManager->SerializeLiveQueryHistory(sFilePath.c_str(), uqsErrorMessage))
		{
			// show the error message
			stack_string error;
			error.Format("Serializing the live query history to '%s' failed: %s", sFilePath.c_str(), uqsErrorMessage.c_str());
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "UQS Query History Inspector: %s", error.c_str());
			QMessageBox::warning(this, "Error saving the live query history", error.c_str());
		}
	}
}

void CMainEditorWindow::OnLoadHistoryFromFile()
{
	QString qFilePath = QFileDialog::getOpenFileName(this, "Load File", "*.xml", "XML file (*.xml)");
	string sFilePath = QtUtil::ToString(qFilePath);
	uqs::shared::CUqsString uqsErrorMessage;
	if (!m_pQueryHistoryManager->DeserializeQueryHistory(sFilePath.c_str(), uqsErrorMessage))
	{
		// show the error message
		stack_string error;
		error.Format("Deserializing the query history from '%s' failed: %s", sFilePath.c_str(), uqsErrorMessage.c_str());
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "UQS Query History Inspector: %s", error.c_str());
		QMessageBox::warning(this, "Error loading the query history", error.c_str());
	}
}
