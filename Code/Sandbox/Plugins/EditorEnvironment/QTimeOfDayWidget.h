// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IEditor.h>
#include <EditorFramework/StateSerializable.h>

#include <Cry3DEngine/ITimeOfDay.h>

#include <QPropertyTree/ContextList.h>

#include <QWidget>

struct SCurveEditorContent;
struct STODParameterGroup;

class CEditorEnvironmentWindow;

enum ETODParamType
{
	eFloatType,
	eColorType
};

struct STODParameter
{
public:
	STODParameter();

	void          SetName(const string& name)           { m_ParamName = name; }
	const char*   GetName() const                       { return m_ParamName.c_str(); }

	void          SetLabel(string labelName)            { m_LabelName = labelName; }
	const char*   GetLabel() const                      { return m_LabelName.c_str(); }

	void          SetGroupName(const string& groupName) { m_GroupName = groupName; }
	const char*   GetGroupName() const                  { return m_GroupName.c_str(); }

	void          SetTODParamType(ETODParamType type)   { m_Type = type; }
	ETODParamType GetTODParamType() const               { return m_Type; }

	void          SetParamID(int id)                    { m_ID = id; }
	int           GetParamID() const                    { return m_ID; }

	void          SetValue(const Vec3& paramValue)      { m_value = paramValue; }
	Vec3          GetValue() const                      { return m_value; }

private:
	Vec3          m_value;
	int           m_ID;

	ETODParamType m_Type;

	string        m_ParamName;
	string        m_LabelName;
	string        m_GroupName;

public:
	STODParameterGroup* m_pGroup;
	int                 m_IDWithinGroup;
};

bool Serialize(Serialization::IArchive& ar, STODParameter& param, const char* name, const char* label);

struct STODParameterGroup
{
	void Serialize(Serialization::IArchive& ar);

	int                        m_id;
	std::string                m_name;
	std::vector<STODParameter> m_Params;
};

struct STODParameterGroupSet
{
	typedef std::vector<STODParameterGroup> TPropertyGroupMap;
	TPropertyGroupMap           m_propertyGroups;

	std::vector<STODParameter*> m_params; // paramID to pointer

	void                        Serialize(Serialization::IArchive& ar);
};

class CCurveEditor;
class QLineEdit;
class QPropertyTree;
class QSplitter;
class CTimeEditControl;

class QTimeOfDayWidget : public QWidget, public IEditorNotifyListener, public IStateSerializable
{
	Q_OBJECT
	Q_INTERFACES(IStateSerializable)

	friend struct STODParameter;

public:
	QTimeOfDayWidget(CEditorEnvironmentWindow* pEditor);
	~QTimeOfDayWidget();

	void                 CheckParameterChanged(STODParameter& param, const Vec3& newValue);

	virtual void         SetState(const QVariantMap& state) override;
	virtual QVariantMap  GetState() const override;

	void                 Refresh();
	void                 OnIdleUpdate();
	void                 UpdateCurveContent();
	void                 OnOpenAsset();
	void                 OnCloseAsset(CAsset* pAsset);

	ITimeOfDay::IPreset* GetPreset();

signals:
	void SignalContentChanged();

private slots:
	void CurveEditTimeChanged();
	void OnPropertySelected();
	void OnCopyCurveContent();
	void OnPasteCurveContent();

private:
	class CContentChangedUndoCommand;
	class CUndoConstPropTreeCommand;

	void         OnBeginUndo();
	void         OnChanged();
	void         OnEndUndo(bool acceptUndo);
	void         CurrentTimeEdited(); // called when user enters new value in m_currentTimeEdit
	void         OnSplineEditing();
	void         OnEditorNotifyEvent(EEditorNotifyEvent event) override;
	void         SetTODTime(const float fTime);

	void         UndoConstantProperties();
	void         OnEndActionUndoConstantProperties(bool acceptUndo);
	void         CreateUi();
	void         CreatePropertyTrees(QSplitter* pParent);
	void         CreateCurveEditor(QSplitter* pParent);
	void         LoadPropertiesTrees();

	void         UpdateValues();
	void         UpdateSelectedParamId();
	void         UpdateVarPropTree();
	void         UpdateCurrentTimeEdit();
	void         UpdateCurveTime();
	void         UpdateConstPropTree();

	virtual bool eventFilter(QObject* obj, QEvent* event) override;
	virtual bool event(QEvent* event) override;

	bool                                         m_bIsPlaying;
	bool                                         m_bIsEditing;
	STODParameterGroupSet                        m_groups;
	std::unique_ptr<Serialization::CContextList> m_pContextList;
	QPropertyTree*                               m_pPropertyTree;

	CTimeEditControl*                            m_pCurrentTimeEdit;
	CTimeEditControl*                            m_pStartTimeEdit;
	CTimeEditControl*                            m_pEndTimeEdit;
	QLineEdit*                                   m_pPlaySpeedEdit;
	CCurveEditor*                                m_pCurveEdit;
	std::unique_ptr<SCurveEditorContent>         m_pCurveContent;

	int                         m_selectedParamId;
	int                         m_previousSelectedParamId;
	CContentChangedUndoCommand* m_pUndoCommand;

	float                       m_fAnimTimeSecondsIn24h;

	QSplitter*                  m_pSplitterBetweenTrees;
	QPropertyTree*              m_pPropertyTreeConst;

	CEditorEnvironmentWindow*   m_pEditor;
	string                      m_preset;
	ITimeOfDay::SAdvancedInfo   m_advancedInfo;
};
