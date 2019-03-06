// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BehaviorTreeDocument.h"

#include <QPropertyTree/ContextList.h>

#include <CryAISystem/BehaviorTree/SerializationSupport.h>

#include <QDialog>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

class QPropertyTree;

class MainWindow;

class TreePanel : public QDockWidget
{
	Q_OBJECT

public:
	TreePanel(MainWindow* pMainWindow);

	void Reset();

	void OnWindowEvent_NewFile();
	void OnWindowEvent_OpenFile();
	void OnWindowEvent_OpenRecentFile(const QString& absoluteFilePath);
	void OnWindowEvent_Save();
	void OnWindowEvent_SaveToFile();

	bool HandleCloseEvent();

	// Property tree control:
	void ShowXmlLineNumbers(const bool showFlag);
	void ShowComments(const bool showFlag);
	void EnableCryEngineSignals(const bool enableFlag);
	void EnableGameSDKSignals(const bool enableFlag);
	void EnableDeprecatedSignals(const bool enableFlag);

	void OnWindowEvent_ShowXmlLineNumbers(const bool showFlag);

	void OnWindowEvent_ShowComments(const bool showFlag);
	void OnWindowEvent_EnableCryEngineSignals(const bool enable);
	void OnWindowEvent_EnableDeprecatedSignals(const bool enable);
	void OnWindowEvent_EnableGameSDKSignals(const bool enable);

public slots:
	void OnPropertyTreeDataChanged();

private:
	// Returns true if all data has been saved
	bool CheckForUnsavedDataAndSave();
	bool CheckForBehaviorTreeErrors();

	// File I/O support:
	void TryOpenFileDirectly(const QString& absoluteFilePath);

	// Property tree control:
	void AttachDocumentToPropertyTree();
	void DetachDocumentFromPropertyTree();
	void ForceAttachDocumentToPropertyTree();
	void ForceApplyDocumentToPropertyTree();

	void ReserializeDocumentIntoPropertyTree();

private:

	MainWindow*                          m_pMainWindow;

	QPropertyTree*                       m_propertyTree;

	BehaviorTreeDocument                 m_behaviorTreeDocument;
	bool                                 m_propertiesAttachedToDocument;
	Serialization::CContextList          m_contextList;
	BehaviorTree::NodeSerializationHints m_behaviorTreeNodeSerializationHints;
	QString                              m_lastOpenedFilePath;
};

struct UnsavedChangesDialog : public QDialog
{
	enum Result
	{
		Yes,
		No,
		Cancel
	};

	Result result;

	UnsavedChangesDialog()
		: result(Cancel)
	{
		setWindowTitle("Behavior Tree Editor");

		QLabel* label = new QLabel("There are unsaved changes. Do you want to save them?");

		QPushButton* yesButton = new QPushButton("Yes");
		QPushButton* noButton = new QPushButton("No");
		QPushButton* cancelButton = new QPushButton("Cancel");

		QGridLayout* gridLayout = new QGridLayout();
		gridLayout->addWidget(label, 0, 0);

		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->addWidget(yesButton);
		buttonLayout->addWidget(noButton);
		buttonLayout->addWidget(cancelButton);

		gridLayout->addLayout(buttonLayout, 1, 0, Qt::AlignCenter);

		QVBoxLayout* mainLayout = new QVBoxLayout();
		mainLayout->addLayout(gridLayout);
		setLayout(mainLayout);

		connect(yesButton, SIGNAL(clicked()), this, SLOT(clickedYes()));
		connect(noButton, SIGNAL(clicked()), this, SLOT(clickedNo()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(clickedCancel()));
	}

	Q_OBJECT

public Q_SLOTS:
	void clickedYes()
	{
		result = Yes;
		accept();
	}

	void clickedNo()
	{
		result = No;
		reject();
	}

	void clickedCancel()
	{
		result = Cancel;
		close();
	}
};
