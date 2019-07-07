// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>
#include <QMenu>

class CLevelEditorViewport;
class CPopupMenuItem;
class QLabel;
class QLineEdit;
class QToolButton;
class QViewportHeader;
struct IViewPaneClass;

//////////////////////////////////////////////////////////////////////////
class QTerrainSnappingMenu : public QMenu
{
	Q_OBJECT
public:
	QTerrainSnappingMenu(QToolButton* pToolButton, QViewportHeader* pViewportHeader);

protected:
	void     mouseReleaseEvent(QMouseEvent* pEvent);
	QAction* AddCommandAction(const char* szCommand, QViewportHeader* pViewportHeader);

protected Q_SLOTS:
	void RefreshParentToolButton();

private:
	QToolButton* m_pParentToolButton;
};

//////////////////////////////////////////////////////////////////////////
class QViewportHeader : public QFrame, public IEditorNotifyListener
{
	Q_OBJECT

public:
	QViewportHeader(CLevelEditorViewport* pViewport);
	virtual ~QViewportHeader();

	void UpdateCameraName();

	void                       AddFOVMenus(CPopupMenuItem& menu, Functor1<float> setFOVFunc, const std::vector<string>& customPresets);
	void                       AddAspectRatioMenus(CPopupMenuItem& menu, Functor2<unsigned int, unsigned int> setAspectRatioFunc, const std::vector<string>& customPresets);
	void                       AddResolutionMenus(CPopupMenuItem& menu, Functor2<int, int> resizeFunc, const std::vector<string>& customPresets);
	static void                UpdateCustomPresets(const string& text, std::vector<string>& custompresets);

	static void                LoadCustomPresets(const char* keyName, std::vector<string>& outCustompresets);
	static void                SaveCustomPresets(const char* keyName, const std::vector<string>& custompresets);

	const std::vector<string>& GetCustomResolutionPresets() const  { return m_customResPresets; }
	const std::vector<string>& GetCustomFOVPresets() const         { return m_customFOVPresets; }
	const std::vector<string>& GetCustomAspectRatioPresets() const { return m_customAspectRatioPresets; }

	void                       OnMenuFOVCustom();
	void                       OnMenuResolutionCustom();

	void                       OnMenuResolutionCustomClear();
	void                       SavePersonalization();

Q_SIGNALS:
	void cameraSpeedChanged(float speed, int increments);

protected:
	void OnEditToolChanged();
	virtual void  OnEditorNotifyEvent(EEditorNotifyEvent event);
	virtual QSize sizeHint() const;

private:
	void         LoadPersonalization();
	QToolButton* AddToolButtonForCommand(const char* szCommand, QLayout* pLayout);

	void         SetViewportFOV(float fov);
	float        GetViewportFOV();

	void         OnMenuViewSelected(IViewPaneClass* viewClass);

	friend class QViewportTitleMenu;
private:
	QToolButton* m_titleBtn;
	QToolButton* m_displayOptions;
	QToolButton* m_pHelpersOptionsBtn;
	QToolButton* m_pDisplayHelpersBtn;

	// Snapping
	QToolButton*          m_pivotSnapping;
	QToolButton*          m_vertexSnapping;
	QString               m_moduleName;

	CLevelEditorViewport* m_viewport;

	static const int      MAX_NUM_CUSTOM_PRESETS = 10;
	std::vector<string>   m_customResPresets;
	std::vector<string>   m_customFOVPresets;
	std::vector<string>   m_customAspectRatioPresets;

	// current tool info and exit button
	QLabel*      m_currentToolText;
	QToolButton* m_currentToolExit;
};
