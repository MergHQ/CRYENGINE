// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <QPropertyTree/ContextList.h>
#include <QWidget>

class CController;
class QPropertyTree;

enum class PlaybackMode;

class CVariablesWidget : public QWidget
{
public:
	CVariablesWidget(CController& controller);
	~CVariablesWidget();

private:
	void OnTreeSelectionChanged();
	void OnSelectedVariableStartChange();
	void OnSelectedVariableEndChange();

	void OnAssetOpened();
	void OnAssetClosed();
	void OnSelectedVariableChanged(QWidget* pSender);
	void ResetTree();
	void OnPlaybackModeChanged(PlaybackMode newMode);
	void ProcessUserEventsFromCurveEditor(QEvent* pEvent);

	CController&                m_controller;
	QPropertyTree*              m_pPropertyTree;
	Serialization::CContextList m_treeContext;
};
