// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QToolButton>

//////////////////////////////////////////////////////////////////////////
//! This is meant to be used for a button that will activate a CEditTool
//! The button will deactivate when the tool gets deactivated
class QEditToolButton : public QToolButton
{
	Q_OBJECT
public:
	QEditToolButton(QWidget* parent);
	~QEditToolButton();

	void SetToolClass(CRuntimeClass* toolClass);
	void SetToolName(const string& sEditToolName);
	// Set if this tool button relies on a loaded level / ready document. By default every tool button only works if a level is loaded.
	// However some tools are also used without a loaded level (e.g. UI Emulator)
	void SetNeedDocument(bool bNeedDocument) { m_bNeedDocument = bNeedDocument; }

protected Q_SLOTS:
	void OnClicked(bool bChecked);

private:
	void DetermineCheckedState();
	//! Tool associated with this button.
	CRuntimeClass* m_toolClass;
	string         m_userDataKey;
	bool           m_bNeedDocument;
	bool           m_ignoreClick;
	void*          m_userData;
};

//////////////////////////////////////////////////////////////////////////
class QEditToolButtonPanel : public QWidget
{
public:
	struct SButtonInfo
	{
		string         name;
		string         toolClassName;
		string         toolUserDataKey;
		string         toolUserData;
		string         toolTip;
		string         icon;
		bool           bNeedDocument;
		CRuntimeClass* pToolClass;

		SButtonInfo() : pToolClass(0), bNeedDocument(true) {}
	};

	enum class LayoutType
	{
		Grid,
		Vertical,
		Horizontal
	};

	QEditToolButtonPanel(LayoutType layoutType = LayoutType::Grid, QWidget* pParent = nullptr);
	virtual ~QEditToolButtonPanel();

	void AddButton(const SButtonInfo& button);
	void AddButton(string name, string toolClass);
	void AddButton(string name, CRuntimeClass* pToolClass);
	void EnableButton(string buttonName, bool disable);
	void ClearButtons();

	void UncheckAll();

protected:
	//////////////////////////////////////////////////////////////////////////
	struct SButton
	{
		SButtonInfo      info;
		QEditToolButton* pButton;
		SButton() : pButton(0) {}
	};

	std::vector<SButton> m_buttons;
	LayoutType           m_layoutType;
};
