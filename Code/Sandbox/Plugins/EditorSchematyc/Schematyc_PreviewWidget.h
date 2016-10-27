// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QObject.h>
#include <QViewportSettings.h>
#include <QWidget.h>
#include "Gizmos/GizmoManager.h"
#include "DisplayViewportAdapter.h"

// Forward declare interfaces.
struct SRenderContext;
struct SMouseEvent;
// Forward declare classes.
class QBoxLayout;
class QParentWndWidget;
class QPropertyTree;
class QPushButton;
class QSplitter;
class QViewport;
enum EMouseEvent;

namespace Schematyc
{
// Forward declare interfaces.
struct IScriptClass;

class CPreviewWidget : public QWidget
{
	Q_OBJECT

public:

	CPreviewWidget(QWidget* pParent);
	~CPreviewWidget();

	void           InitLayout();
	void           Update();
	void           SetClass(const IScriptClass* pClass);
	void           Reset();
	void           LoadSettings(const XmlNodeRef& xml);
	XmlNodeRef     SaveSettings(const char* szName);
	void           Serialize(Serialization::IArchive& archive);
	IGizmoManager* GetGizmoManager() { return &m_gizmoManager; }

protected slots:

	void OnRender(const SRenderContext& context);
	void OnMouse(const SMouseEvent&);
	void OnShowHideSettingsButtonClicked();

private:

	void IEditorEventFromQViewportEvent(const SMouseEvent& qEvt, CPoint& p, EMouseEvent& evt, int& flags);
	void ResetCamera(const Vec3& orbitTarget, float orbitRadius) const;

	QBoxLayout*         m_pMainLayout = nullptr;
	QSplitter*          m_pSplitter = nullptr;
	QPropertyTree*      m_pSettings = nullptr;
	QViewport*          m_pViewport = nullptr;
	QBoxLayout*         m_pControlLayout = nullptr;
	QPushButton*        m_pShowHideSettingsButton = nullptr;

	SViewportSettings   m_viewportSettings;
	const IScriptClass* m_pScriptClass = nullptr; // #SchematycTODO : Replace with class guid? Do we even need to store this?
	IObjectPreviewer*   m_pObjectPreviewer = nullptr;
	ObjectId            m_objectId;

	static const Vec3   ms_defaultOrbitTarget;
	static const float  ms_defaultOrbitRadius;

	std::unique_ptr<CDisplayViewportAdapter> m_pViewportAdapter;
	CGizmoManager m_gizmoManager;
};
} // Schematyc
