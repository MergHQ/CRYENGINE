// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "QTimeOfDayWidget.h"

#include "EditorEnvironmentWindow.h"

#include <Cry3DEngine/I3DEngine.h>
#include <CryEditDoc.h>
#include <CryIcon.h>
#include <CrySerialization/yasli/JSONIArchive.h>
#include <CrySerialization/yasli/JSONOArchive.h>
#include <CurveEditor.h>
#include <EditorStyleHelper.h>
#include <IEditor.h>
#include <IUndoObject.h>
#include <QPropertyTree/ContextList.h>
#include <Serialization/QPropertyTree/QPropertyTree.h>
#include <TimeEditControl.h>

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPainter>
#include <QSplitter>
#include <QTimeEdit>
#include <QToolBar>
#include <QToolButton>

#include <map>

SERIALIZATION_ENUM_BEGIN(ETODParamType, "ETodParamType")
SERIALIZATION_ENUM(eFloatType, "eFloatType", "eFloatType")
SERIALIZATION_ENUM(eColorType, "eColorType", "eColorType")
SERIALIZATION_ENUM_END()

namespace
{

const float nTimeLineMinutesScale = 60.0f;
const int nRulerHeight = 30;
const int nRulerGradientHeight = 14;
const float fToDParameterComareEpsilon = 0.0001f;
const char* sClipboardCurveContentMimeTypeFloat = "binary/curveeditorcontentfloat";
const char* sClipboardCurveContentMimeTypeColor = "binary/curveeditorcontentcolor";

ITimeOfDay* GetTimeOfDay()
{
	ITimeOfDay* pTimeOfDay = gEnv->p3DEngine->GetTimeOfDay();
	assert(pTimeOfDay); //pTimeOfDay is always exists
	return pTimeOfDay;
}

float QTimeToFloat(const QTime& time)
{
	const int nSeconds = QTime(0, 0, 0).secsTo(time);
	const float fTime = nSeconds / 3600.0f;
	return fTime;
}

QTime FloatToQTime(float fTime)
{
	unsigned int hour = floor(fTime);
	unsigned int minute = floor((fTime - floor(fTime)) * nTimeLineMinutesScale + 0.5f);
	if (hour > 23)
	{
		hour = 23;
		minute = 59;
	}
	return QTime(hour, minute);
}

void SetQTimeEditTimeBlocking(QTimeEdit* pTimeEdit, QTime time)
{
	pTimeEdit->blockSignals(true);
	pTimeEdit->setTime(time);
	pTimeEdit->blockSignals(false);
}

void SetQTimeEditTimeBlocking(CTimeEditControl* pTimeEdit, QTime time)
{
	pTimeEdit->blockSignals(true);
	pTimeEdit->setTime(time);
	pTimeEdit->blockSignals(false);
}

void UpdateToDAdvancedInfo(ITimeOfDay::SAdvancedInfo& sAdvInfo, std::function<void (ITimeOfDay::SAdvancedInfo& sAdvInfo)> updateFunc)
{
	ITimeOfDay* const pTimeOfDay = GetTimeOfDay();

	pTimeOfDay->GetAdvancedInfo(sAdvInfo);
	updateFunc(sAdvInfo);
	pTimeOfDay->SetAdvancedInfo(sAdvInfo);

	pTimeOfDay->Update(false, true);
	GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
}

void UpdateCurveContentFromToDVar(ITimeOfDay::IPreset* pPreset, int nVarId, int nSplineId, SCurveEditorContent* pContent)
{
	if (nSplineId >= 0 && nSplineId < pContent->m_curves.size())
	{
		std::vector<SCurveEditorKey>& splineKeys = pContent->m_curves[nSplineId].m_keys;
		splineKeys.clear();

		const ITimeOfDay::IVariables& variables = pPreset->GetVariables();

		const uint nKeysNum = variables.GetSplineKeysCount(nVarId, nSplineId);
		if (nKeysNum)
		{
			std::vector<SBezierKey> keys;
			keys.resize(nKeysNum);
			variables.GetSplineKeysForVar(nVarId, nSplineId, &keys[0], nKeysNum);

			splineKeys.reserve(nKeysNum);
			for (auto& key : keys)
			{
				SCurveEditorKey newKey;
				newKey.m_time = key.m_time;
				newKey.m_controlPoint = key.m_controlPoint;
				splineKeys.push_back(newKey);
			}
		}
	}
}

void UpdateToDVarFromCurveContent(ITimeOfDay::IPreset* pPreset, int nVarId, int nSplineId, const SCurveEditorContent* pContent)
{
	if (nSplineId >= 0 && nSplineId < pContent->m_curves.size())
	{
		const SCurveEditorCurve& curve = pContent->m_curves[nSplineId];
		std::vector<SBezierKey> keys;
		keys.reserve(curve.m_keys.size());
		for (auto& key : curve.m_keys)
		{
			if (!key.m_bDeleted)
			{
				SBezierKey newKey;
				newKey.m_time = key.m_time;
				newKey.m_controlPoint = key.m_controlPoint;
				keys.push_back(newKey);
			}
		}

		pPreset->GetVariables().SetSplineKeysForVar(nVarId, nSplineId, &keys[0], keys.size());
	}
}

QRgb ColorLinearToGamma(const Vec3& col)
{
	float r = clamp_tpl(col.x, 0.0f, 1.0f);
	float g = clamp_tpl(col.y, 0.0f, 1.0f);
	float b = clamp_tpl(col.z, 0.0f, 1.0f);

	r = (float)(r <= 0.0031308 ? (12.92 * r) : (1.055 * pow((double)r, 1.0 / 2.4) - 0.055));
	g = (float)(g <= 0.0031308 ? (12.92 * g) : (1.055 * pow((double)g, 1.0 / 2.4) - 0.055));
	b = (float)(b <= 0.0031308 ? (12.92 * b) : (1.055 * pow((double)b, 1.0 / 2.4) - 0.055));

	r *= 255.0f;
	g *= 255.0f;
	b *= 255.0f;

	return qRgb(int(r), int(g), int(b));
}

void DrawGradient(ITimeOfDay::IPreset* pPreset, int selectedParam, QPainter& painter, const QRect& rect, const Range& visibleRange)
{
	if (!pPreset)
	{
		return;
	}

	ITimeOfDay::IVariables& variables = pPreset->GetVariables();
	if (selectedParam >= 0 && selectedParam < variables.GetVariableCount())
	{
		return;
	}

	ITimeOfDay::SVariableInfo sVarInfo;
	if (variables.GetVariableInfo(selectedParam, sVarInfo))
	{
		const int xMin = rect.left();
		const int xMax = rect.right();
		const int nRange = xMax - xMin;
		const float fRange = float(nRange);

		QPoint from = QPoint(xMin, rect.bottom() - nRulerGradientHeight + 1);
		QPoint to = QPoint(xMin, rect.bottom() + 1);

		std::vector<Vec3> gradient;
		gradient.resize(nRange);
		variables.InterpolateVarInRange(selectedParam, visibleRange.start, visibleRange.end, nRange, &gradient[0]);

		if (ITimeOfDay::TYPE_COLOR == sVarInfo.type)
		{
			for (size_t i = 0; i < nRange; ++i)
			{
				const Vec3& val = gradient[i];
				QColor color(ColorLinearToGamma(val));
				painter.setPen(color);
				from.setX(i + xMin);
				to.setX(i + xMin);
				painter.drawLine(from, to);
			}
		}
		else
		{
			for (size_t i = 0; i < nRange; ++i)
			{
				const Vec3& val = gradient[i];
				const float value = clamp_tpl(val.x, sVarInfo.fValue[1], sVarInfo.fValue[2]);
				const float normVal = (value - sVarInfo.fValue[1]) / (sVarInfo.fValue[2] - sVarInfo.fValue[1]);
				const int grayScaled = int(normVal * 255.0f);
				QColor color(grayScaled, grayScaled, grayScaled);
				painter.setPen(color);
				from.setX(i + xMin);
				to.setX(i + xMin);
				painter.drawLine(from, to);
			}
		}
	}
}
} // unnamed namespace

//////////////////////////////////////////////////////////////////////////
STODParameter::STODParameter() : m_value(ZERO), m_ID(0), m_Type(eFloatType), m_pGroup(NULL), m_IDWithinGroup(-1)
{
	m_ParamName = "DefaultName";
	m_LabelName = "DefaultLabel";
	m_GroupName = "DefaultGroup";
}

bool Serialize(Serialization::IArchive& ar, STODParameter& param, const char* name, const char* label)
{
	Vec3 value = param.GetValue();

	if (param.GetTODParamType() == ITimeOfDay::TYPE_COLOR)
	{
		if (!ar(Serialization::Vec3AsColor(value), name, label))
			return false;
	}
	else if (param.GetTODParamType() == ITimeOfDay::TYPE_FLOAT)
	{
		if (value.y < value.z)
		{
			if (!ar(Serialization::Range(value.x, value.y, value.z), name, label))
				return false;
		}
		else
		{
			if (!ar(value.x, name, label))
				return false;
		}
	}

	if (ar.isInput())
	{
		if (QTimeOfDayWidget* window = ar.context<QTimeOfDayWidget>())
		{
			window->CheckParameterChanged(param, value);
		}
	}
	return true;
}

void STODParameterGroup::Serialize(IArchive& ar)
{
	for (size_t i = 0, nParamCount = m_Params.size(); i < nParamCount; ++i)
	{
		ar(m_Params[i], m_Params[i].GetName(), m_Params[i].GetLabel());
	}
}

void STODParameterGroupSet::Serialize(IArchive& ar)
{
	for (TPropertyGroupMap::iterator it = m_propertyGroups.begin(); it != m_propertyGroups.end(); ++it)
	{
		STODParameterGroup& group = *it;
		const char* name = group.m_name.c_str();
		ar(group, name, name);
	}
}

//////////////////////////////////////////////////////////////////////////
class QTimeOfDayWidget::CContentChangedUndoCommand : public IUndoObject
{
public:
	explicit CContentChangedUndoCommand(QTimeOfDayWidget* pWidget)
	{
		m_pWidget = pWidget;
		m_paramId = pWidget->m_selectedParamId;
		SCurveEditorContent* pContent = pWidget->m_pCurveContent.get();
		Serialization::SaveBinaryBuffer(m_oldState, *pContent);
	}

	void SaveNewState(QTimeOfDayWidget* pWidget)
	{
		SCurveEditorContent* pContent = pWidget->m_pCurveContent.get();
		Serialization::SaveBinaryBuffer(m_newState, *pContent);
	}

	int GetParamId() const { return m_paramId; }

private:
	virtual const char* GetDescription() override { return "QTimeOfDayWidget content changed"; }

	virtual void        Undo(bool bUndo) override
	{
		SaveNewState(m_pWidget);
		Update(m_oldState);
	}

	virtual void Redo() override
	{
		Update(m_newState);
	}

	void Update(DynArray<char>& array)
	{
		SCurveEditorContent tempContent;
		Serialization::LoadBinaryBuffer(tempContent, array.begin(), array.size());

		const SCurveEditorContent* pContent = &tempContent;
		const int paramID = m_paramId;
		if (paramID >= 0)
		{
			ITimeOfDay::IPreset* pPreset = m_pWidget->GetPreset();

			UpdateToDVarFromCurveContent(pPreset, paramID, 0, pContent);
			UpdateToDVarFromCurveContent(pPreset, paramID, 1, pContent);
			UpdateToDVarFromCurveContent(pPreset, paramID, 2, pContent);

			GetTimeOfDay()->Update(true, true);
			GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
			m_pWidget->OnChanged();
		}
	}

	DynArray<char> m_oldState;
	DynArray<char> m_newState;
	int            m_paramId;
	QTimeOfDayWidget* m_pWidget;
};

//////////////////////////////////////////////////////////////////////////
class QTimeOfDayWidget::CUndoConstPropTreeCommand : public IUndoObject
{
public:
	explicit CUndoConstPropTreeCommand(QTimeOfDayWidget* pTodWidget)
		: m_pTodWidget(pTodWidget)
	{
		ITimeOfDay::IPreset* const pPreset = m_pTodWidget->GetPreset();
		gEnv->pSystem->GetArchiveHost()->SaveBinaryBuffer(m_undoState, Serialization::SStruct(pPreset->GetConstants()));
	}

private:
	virtual const char* GetDescription() override { return "Set Environment constant properties"; }

	virtual void        Undo(bool bUndo = true) override
	{
		if (bUndo)
		{
			ITimeOfDay::IPreset* const pPreset = m_pTodWidget->GetPreset();
			gEnv->pSystem->GetArchiveHost()->SaveBinaryBuffer(m_redoState, Serialization::SStruct(pPreset->GetConstants()));
		}

		ApplyState(m_undoState);
	}

	virtual void Redo() override
	{
		ApplyState(m_redoState);
	}

	void ApplyState(const DynArray<char>& state)
	{
		ITimeOfDay::IPreset* const pPreset = m_pTodWidget->GetPreset();
		gEnv->pSystem->GetArchiveHost()->LoadBinaryBuffer(Serialization::SStruct(pPreset->GetConstants()), state.data(), state.size());
		m_pTodWidget->UpdateConstPropTree();
		m_pTodWidget->OnChanged();
	}

	QTimeOfDayWidget* m_pTodWidget;
	DynArray<char>    m_undoState;
	DynArray<char>    m_redoState;
};

//////////////////////////////////////////////////////////////////////////
QTimeOfDayWidget::QTimeOfDayWidget(CEditorEnvironmentWindow* pEditor)
	: m_pContextList(new Serialization::CContextList())
	, m_pPropertyTree(nullptr)
	, m_pCurrentTimeEdit(nullptr)
	, m_pStartTimeEdit(nullptr)
	, m_pEndTimeEdit(nullptr)
	, m_pPlaySpeedEdit(nullptr)
	, m_pCurveEdit(nullptr)
	, m_pCurveContent(new SCurveEditorContent)
	, m_selectedParamId(-1)
	, m_previousSelectedParamId(-1)
	, m_pUndoCommand(nullptr)
	, m_fAnimTimeSecondsIn24h(0.0f)
	, m_bIsPlaying(false)
	, m_bIsEditing(false)
	, m_pSplitterBetweenTrees(nullptr)
	, m_pPropertyTreeConst(nullptr)
	, m_pEditor(pEditor)
{
	m_advancedInfo.fAnimSpeed = 0;
	m_advancedInfo.fStartTime = 0;
	m_advancedInfo.fEndTime = 24;

	m_pContextList->Update<QTimeOfDayWidget>(this);

	m_fAnimTimeSecondsIn24h = GetTimeOfDay()->GetAnimTimeSecondsIn24h();

	CreateUi();
	GetIEditor()->RegisterNotifyListener(this);
	m_pEditor->signalAssetOpened.Connect(this, &QTimeOfDayWidget::OnOpenAsset);
	m_pEditor->signalAssetClosed.Connect(this, &QTimeOfDayWidget::OnCloseAsset);
	setEnabled(false);
}

QTimeOfDayWidget::~QTimeOfDayWidget()
{
	GetIEditor()->UnregisterNotifyListener(this);
	m_pEditor->signalAssetOpened.DisconnectObject(this);
}

void QTimeOfDayWidget::Refresh()
{
	setEnabled(m_pEditor->GetAssetBeingEdited());
	if (!GetPreset())
	{
		return;
	}

	LoadPropertiesTrees();

	UpdateValues();
	UpdateCurveContent();

	if (m_selectedParamId < 0)
	{
		m_pCurveEdit->SetFitMargin(0.0f);
		m_pCurveEdit->ZoomToTimeRange(SAnimTime(0.0f), SAnimTime(m_fAnimTimeSecondsIn24h));
	}
	else
	{
		m_pCurveEdit->OnFitCurvesVertically();
		m_pCurveEdit->OnFitCurvesHorizontally();
	}
}

void QTimeOfDayWidget::OnIdleUpdate()
{
	if (!m_bIsPlaying)
	{
		return;
	}

	ITimeOfDay* const pTimeOfDay = GetTimeOfDay();

	// Animate the preset parameters for the active editor only.
	const ITimeOfDay::IPreset* const pPreset = &pTimeOfDay->GetCurrentPreset();
	if (pPreset != GetPreset())
	{
		return;
	}

	const float fHour = pTimeOfDay->GetTime();

	ITimeOfDay::SAdvancedInfo advInfo;
	pTimeOfDay->GetAdvancedInfo(advInfo);
	float fTime = fHour + gEnv->pTimer->GetFrameTime() * advInfo.fAnimSpeed;
	if (fTime > advInfo.fEndTime)
		fTime = advInfo.fStartTime;
	if (fTime < advInfo.fStartTime)
		fTime = advInfo.fEndTime;

	SetTODTime(fTime);
	UpdateValues();
}

void QTimeOfDayWidget::UpdateCurveContent()
{
	const QColor& textColor = GetStyleHelper()->textColor();

	if (m_selectedParamId >= 0)
	{
		ITimeOfDay::IPreset* const pPreset = GetPreset();
		const int paramID = m_selectedParamId;
		ITimeOfDay::SVariableInfo sVarInfo;
		pPreset->GetVariables().GetVariableInfo(m_selectedParamId, sVarInfo);

		if (ITimeOfDay::TYPE_FLOAT == sVarInfo.type)
		{
			m_pCurveContent->m_curves.resize(1);
			m_pCurveContent->m_curves[0].m_color.set(textColor.red(), textColor.green(), textColor.blue(), 255);
			m_pCurveEdit->SetValueRange(sVarInfo.fValue[1], sVarInfo.fValue[2]);
		}
		else if (ITimeOfDay::TYPE_COLOR == sVarInfo.type)
		{
			m_pCurveContent->m_curves.resize(3);
			m_pCurveContent->m_curves[0].m_color.set(190, 20, 20, 255); // r
			m_pCurveContent->m_curves[1].m_color.set(20, 190, 20, 255); // g
			m_pCurveContent->m_curves[2].m_color.set(20, 20, 190, 255); // b
			m_pCurveEdit->SetValueRange(0.0f, 1.0f);
		}

		UpdateCurveContentFromToDVar(pPreset, paramID, 0, m_pCurveContent.get());
		UpdateCurveContentFromToDVar(pPreset, paramID, 1, m_pCurveContent.get());
		UpdateCurveContentFromToDVar(pPreset, paramID, 2, m_pCurveContent.get());
	}
	else
	{
		m_pCurveEdit->SetValueRange(0.0f, 1.0f);
		m_pCurveContent->m_curves.resize(0);
	}
}

void QTimeOfDayWidget::OnOpenAsset()
{
	m_preset = m_pEditor->GetAssetBeingEdited()->GetFile(0);
	Refresh();
}

void QTimeOfDayWidget::OnCloseAsset(CAsset* pAsset)
{
	m_preset.clear();
	Refresh();
}

ITimeOfDay::IPreset* QTimeOfDayWidget::GetPreset()
{
	return m_pEditor->GetPreset();
}

void QTimeOfDayWidget::CurveEditTimeChanged()
{
	float time = m_pCurveEdit->Time().ToFloat();
	time /= m_fAnimTimeSecondsIn24h;
	float fTODTime = time * 24.0f;
	SetTODTime(fTODTime);
	UpdateCurrentTimeEdit();
	UpdateVarPropTree();
}

void QTimeOfDayWidget::CurrentTimeEdited()
{
	const float fTODTime = QTimeToFloat(m_pCurrentTimeEdit->time());
	SetTODTime(fTODTime);
	UpdateCurveTime();
	UpdateVarPropTree();
}

void QTimeOfDayWidget::OnPropertySelected()
{
	const int oldSelectedParam = m_selectedParamId;

	UpdateSelectedParamId();

	if (m_selectedParamId < 0)
	{
		UpdateCurveContent();
		m_pCurveEdit->ZoomToTimeRange(SAnimTime(0.0f), SAnimTime(m_fAnimTimeSecondsIn24h));
		return;
	}

	if (oldSelectedParam != m_selectedParamId)
	{
		UpdateCurveContent();
		m_pCurveEdit->OnFitCurvesVertically();
		m_pCurveEdit->OnFitCurvesHorizontally();
	}
}

void QTimeOfDayWidget::OnBeginUndo()
{
	if (m_pCurveContent && !CUndo::IsRecording() && !m_pUndoCommand)
	{
		GetIEditor()->GetIUndoManager()->Begin();
		m_pUndoCommand = new CContentChangedUndoCommand(this);
		const int paramId = m_pUndoCommand->GetParamId();
		if (paramId >= 0)
		{
			GetIEditor()->GetIUndoManager()->RecordUndo(m_pUndoCommand);
		}
	}
}

void QTimeOfDayWidget::OnChanged()
{
	SignalContentChanged();
	m_pEditor->GetAssetBeingEdited()->SetModified(true);
}

void QTimeOfDayWidget::OnEndUndo(bool acceptUndo)
{
	if (!m_pUndoCommand)
	{
		return;
	}

	const int paramId = m_pUndoCommand->GetParamId();
	if (paramId >= 0)
	{
		if (acceptUndo)
		{
			const STODParameter* pParam = m_groups.m_params[paramId];
			const char* paramName = pParam->GetName();

			string undoDesc;
			undoDesc.Format("Environment Editor: Curve content changed for '%s'", paramName);
			GetIEditor()->GetIUndoManager()->Accept(undoDesc);
		}
		else
		{
			GetIEditor()->GetIUndoManager()->Cancel();
		}

		SignalContentChanged();
		m_pUndoCommand = nullptr;
	}
}

void QTimeOfDayWidget::UndoConstantProperties()
{
	GetIEditor()->GetIUndoManager()->Begin();
	GetIEditor()->GetIUndoManager()->RecordUndo(new CUndoConstPropTreeCommand(this));

}

void QTimeOfDayWidget::OnEndActionUndoConstantProperties(bool acceptUndo)
{
	if (acceptUndo)
	{
		GetIEditor()->GetIUndoManager()->Accept("Update Environment Constants");
	}
	else
	{
		GetIEditor()->GetIUndoManager()->Cancel();
	}
}

void QTimeOfDayWidget::CheckParameterChanged(STODParameter& param, const Vec3& newValue)
{
	CRY_ASSERT(GetPreset());

	ITimeOfDay::IPreset* const pPreset = GetPreset();
	ITimeOfDay::IVariables& variables = pPreset->GetVariables();

	const int paramID = param.GetParamID();
	const float fTime = GetTimeOfDay()->GetTime();
	const float fSplineTime = (fTime / 24.0f) * m_fAnimTimeSecondsIn24h;

	bool bChanged = false;
	const Vec3 oldValue = param.GetValue();
	if (fabs(oldValue.x - newValue.x) > fToDParameterComareEpsilon)
	{
		bChanged = true;
		variables.UpdateSplineKeyForVar(paramID, 0, fSplineTime, newValue.x);
	}
	if (fabs(oldValue.y - newValue.y) > fToDParameterComareEpsilon)
	{
		bChanged = true;
		variables.UpdateSplineKeyForVar(paramID, 1, fSplineTime, newValue.y);
	}
	if (fabs(oldValue.z - newValue.z) > fToDParameterComareEpsilon)
	{
		bChanged = true;
		variables.UpdateSplineKeyForVar(paramID, 2, fSplineTime, newValue.z);
	}

	if (bChanged)
	{
		param.SetValue(newValue);
		GetTimeOfDay()->Update(true, true);
		m_bIsEditing = true;
		GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
		m_bIsEditing = false;
		UpdateCurveContent();
	}
}

namespace {

void RestoreTreeState(const QVariantMap& state, const char* name, QPropertyTree* pTree)
{
	QVariant var = state.value(name);
	if (!var.isValid())
	{
		return;
	}

	auto arr = var.toByteArray();
	yasli::JSONIArchive ia;
	if (!ia.open(arr, arr.size()))
	{
		return;
	}

	ia(*pTree);
}

void SaveTreeState(const QPropertyTree* pTree, const char* name, QVariantMap& state)
{
	yasli::JSONOArchive oa;
	oa(*pTree, name);
	QVariant variant = QString(oa.buffer());
	state.insert(name, variant);
}

} //~unnamed namespace

void QTimeOfDayWidget::SetState(const QVariantMap& state)
{
	QVariant var = state.value("TreeSplitter");
	if (var.isValid())
	{
		m_pSplitterBetweenTrees->restoreState(QByteArray::fromBase64(var.toByteArray()));
	}

	RestoreTreeState(state, "TreeVar", m_pPropertyTree);
	RestoreTreeState(state, "TreeConst", m_pPropertyTreeConst);
}

QVariantMap QTimeOfDayWidget::GetState() const
{
	QVariantMap state;
	state.insert("TreeSplitter", m_pSplitterBetweenTrees->saveState().toBase64());

	SaveTreeState(m_pPropertyTree, "TreeVar", state);
	SaveTreeState(m_pPropertyTreeConst, "TreeConst", state);

	return state;
}

void QTimeOfDayWidget::OnSplineEditing()
{
	if (m_selectedParamId < 0)
		return;

	SCurveEditorContent* pContent = m_pCurveContent.get();

	if (!m_bIsPlaying)
	{
		// update current time if something is selected
		float fMinSelectedTime = std::numeric_limits<float>::max();
		for (auto& curve : pContent->m_curves)
		{
			for (auto& key : curve.m_keys)
			{
				if (key.m_bSelected)
					fMinSelectedTime = std::min(fMinSelectedTime, key.m_time.ToFloat());
			}
		}

		fMinSelectedTime /= m_fAnimTimeSecondsIn24h;
		if (fMinSelectedTime >= 0.0f && fMinSelectedTime <= 1.0f)
		{
			SetTODTime(fMinSelectedTime * 24.0f);
			UpdateCurrentTimeEdit();
			UpdateCurveTime();
		}
	}

	ITimeOfDay::IPreset* const pPreset = GetPreset();
	UpdateToDVarFromCurveContent(pPreset, m_selectedParamId, 0, pContent);
	UpdateToDVarFromCurveContent(pPreset, m_selectedParamId, 1, pContent);
	UpdateToDVarFromCurveContent(pPreset, m_selectedParamId, 2, pContent);

	GetTimeOfDay()->Update(true, true);
	m_bIsEditing = true;
	GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
	m_bIsEditing = false;
	UpdateVarPropTree();
}

void QTimeOfDayWidget::OnCopyCurveContent()
{
	if (m_selectedParamId < 0 || m_selectedParamId >= m_groups.m_params.size())
		return;

	SCurveEditorContent* pContent = m_pCurveContent.get();
	if (pContent)
	{
		DynArray<char> buffer;
		Serialization::SaveBinaryBuffer(buffer, *pContent);

		const STODParameter* pParam = m_groups.m_params[m_selectedParamId];
		const char* mimeType = (pParam->GetTODParamType() == eFloatType) ? sClipboardCurveContentMimeTypeFloat : sClipboardCurveContentMimeTypeColor;

		QByteArray byteArray(buffer.begin(), buffer.size());
		QMimeData* mime = new QMimeData;
		mime->setData(mimeType, byteArray);

		QApplication::clipboard()->setMimeData(mime);
	}
}

void QTimeOfDayWidget::OnPasteCurveContent()
{
	if (m_selectedParamId < 0 || m_selectedParamId >= m_groups.m_params.size())
		return;

	const QMimeData* mime = QApplication::clipboard()->mimeData();
	if (mime)
	{
		STODParameter* pParam = m_groups.m_params[m_selectedParamId];
		const char* mimeType = (pParam->GetTODParamType() == eFloatType) ? sClipboardCurveContentMimeTypeFloat : sClipboardCurveContentMimeTypeColor;

		QByteArray array = mime->data(mimeType);
		if (array.isEmpty())
			return;

		SCurveEditorContent* pContent = m_pCurveContent.get();
		if (pContent)
		{
			OnBeginUndo();
			DynArray<char> buffer;
			buffer.resize(array.size());
			memcpy(buffer.begin(), array.data(), array.size());
			Serialization::LoadBinaryBuffer(*pContent, buffer.begin(), buffer.size());
			OnChanged();
			OnSplineEditing();
		}
	}
}

void QTimeOfDayWidget::SetTODTime(const float fTime)
{
	ITimeOfDay* const pTimeOfDay = GetTimeOfDay();
	pTimeOfDay->SetTime(fTime, true);

	if (GetIEditor()->GetDocument()->GetMissionCount())
	{
		GetIEditor()->SetCurrentMissionTime(fTime);
	}

	m_bIsEditing = true;
	GetIEditor()->Notify(eNotify_OnTimeOfDayChange);
	m_bIsEditing = false;
}

void QTimeOfDayWidget::CreateUi()
{
	QBoxLayout* rootLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	rootLayout->setContentsMargins(1, 1, 1, 1);
	setLayout(rootLayout);

	QSplitter* splitter = new QSplitter(Qt::Orientation::Horizontal);
	splitter->setChildrenCollapsible(false);
	rootLayout->addWidget(splitter, Qt::AlignCenter);

	CreatePropertyTrees(splitter);
	CreateCurveEditor(splitter);

	setMinimumSize(size());
}

void QTimeOfDayWidget::CreatePropertyTrees(QSplitter* pParent)
{
	QWidget* const pVarWidget = new QWidget;
	{
		QLabel* pLabel = new QLabel("Variables");
		pLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pPropertyTree = new QPropertyTree(this);
		m_pPropertyTree->setHideSelection(false);
		m_pPropertyTree->setUndoEnabled(false, false);
		connect(m_pPropertyTree, &QPropertyTree::signalSelected, this, &QTimeOfDayWidget::OnPropertySelected);
		connect(m_pPropertyTree, &QPropertyTree::signalBeginUndo, this, &QTimeOfDayWidget::OnBeginUndo);
		connect(m_pPropertyTree, &QPropertyTree::signalChanged, this, &QTimeOfDayWidget::OnChanged);
		connect(m_pPropertyTree, &QPropertyTree::signalEndUndo, this, &QTimeOfDayWidget::OnEndUndo);

		QVBoxLayout* const pLayout = new QVBoxLayout;
		pLayout->setContentsMargins(QMargins(0, 0, 0, 0));
		pLayout->addWidget(pLabel);
		pLayout->addWidget(m_pPropertyTree);
		pVarWidget->setLayout(pLayout);
	}

	QWidget* const pConstWidget = new QWidget;
	{
		QLabel* const pLabel = new QLabel("Constants");
		pLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pPropertyTreeConst = new QPropertyTree(this);
		m_pPropertyTreeConst->setHideSelection(false);
		m_pPropertyTreeConst->setUndoEnabled(false, false);
		connect(m_pPropertyTreeConst, &QPropertyTree::signalChanged, this, [this]() 
		{ 
			ITimeOfDay* const pTimeOfDay = GetTimeOfDay();
			if (GetPreset() == &pTimeOfDay->GetCurrentPreset())
			{
				pTimeOfDay->ConstantsChanged();
				OnChanged();
			}
		});
		connect(m_pPropertyTreeConst, &QPropertyTree::signalBeginUndo, this, &QTimeOfDayWidget::UndoConstantProperties);
		connect(m_pPropertyTreeConst, &QPropertyTree::signalEndUndo, this, &QTimeOfDayWidget::OnEndActionUndoConstantProperties);

		QVBoxLayout* const pLayout = new QVBoxLayout;
		pLayout->setContentsMargins(QMargins(0, 0, 0, 0));
		pLayout->addWidget(pLabel);
		pLayout->addWidget(m_pPropertyTreeConst);
		pConstWidget->setLayout(pLayout);
	}

	m_pSplitterBetweenTrees = new QSplitter(Qt::Orientation::Vertical);
	m_pSplitterBetweenTrees->setChildrenCollapsible(false);

	m_pSplitterBetweenTrees->addWidget(pVarWidget);
	m_pSplitterBetweenTrees->addWidget(pConstWidget);

	pParent->addWidget(m_pSplitterBetweenTrees);
}

void QTimeOfDayWidget::CreateCurveEditor(QSplitter* pParent)
{
	QBoxLayout* pCentralVerticalLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	QWidget* pWidget = new QWidget();
	pWidget->setLayout(pCentralVerticalLayout);
	pParent->addWidget(pWidget);

	QToolBar* pToolBar = new QToolBar(this);
	pCentralVerticalLayout->addWidget(pToolBar, Qt::AlignTop);

	m_pCurveEdit = new CCurveEditor(this);
	m_pCurveEdit->FillWithCurveToolsAndConnect(pToolBar);
	m_pCurveEdit->SetContent(m_pCurveContent.get());
	m_pCurveEdit->SetTimeRange(SAnimTime(0.0f), SAnimTime(m_fAnimTimeSecondsIn24h));
	m_pCurveEdit->SetRulerVisible(true);
	m_pCurveEdit->SetRulerHeight(nRulerHeight);
	m_pCurveEdit->SetRulerTicksYOffset(nRulerGradientHeight);
	m_pCurveEdit->SetHandlesVisible(false);
	m_pCurveEdit->SetGridVisible(true);
	m_pCurveEdit->OnFitCurvesHorizontally();
	m_pCurveEdit->OnFitCurvesVertically();
	m_pCurveEdit->installEventFilter(this);
	m_pCurveEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	connect(m_pCurveEdit, &CCurveEditor::SignalScrub, this, &QTimeOfDayWidget::CurveEditTimeChanged);
	connect(m_pCurveEdit, &CCurveEditor::SignalDrawRulerBackground, [this](QPainter& painter, const QRect& rulerRect, const Range& visibleRange)
	{
		DrawGradient(GetPreset(), m_selectedParamId, painter, rulerRect, visibleRange);
	});
	connect(m_pCurveEdit, &CCurveEditor::SignalContentAboutToBeChanged, this, &QTimeOfDayWidget::OnBeginUndo);
	connect(m_pCurveEdit, &CCurveEditor::SignalContentChanging, this, &QTimeOfDayWidget::OnSplineEditing);
	connect(m_pCurveEdit, &CCurveEditor::SignalContentChanged, [this]() { OnSplineEditing(); OnChanged(); OnEndUndo(true); });

	pCentralVerticalLayout->addWidget(m_pCurveEdit, Qt::AlignTop);

	pToolBar->addSeparator();
	pToolBar->addAction(CryIcon("icons:CurveEditor/Copy_Curve.ico"), "Copy curve content", this, SLOT(OnCopyCurveContent()));
	pToolBar->addAction(CryIcon("icons:CurveEditor/Paste_Curve.ico"), "Paste curve content", this, SLOT(OnPasteCurveContent()));

	QBoxLayout* pBottomLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	pBottomLayout->setContentsMargins(5, 1, 5, 3);
	pCentralVerticalLayout->addLayout(pBottomLayout, Qt::AlignBottom);

	{
		QBoxLayout* pMediaBarLayout = new QHBoxLayout;
		pMediaBarLayout->setContentsMargins(QMargins(0, 0, 0, 0));

		QBoxLayout* pMediaBarLayoutLeft = new QHBoxLayout;
		QBoxLayout* pMediaBarLayoutCenter = new QHBoxLayout;
		QBoxLayout* pMediaBarLayoutRight = new QHBoxLayout;

		pMediaBarLayout->addLayout(pMediaBarLayoutLeft);
		pMediaBarLayout->addLayout(pMediaBarLayoutCenter);
		pMediaBarLayout->addLayout(pMediaBarLayoutRight);

		QLabel* pStartTimeLabel = new QLabel("Start:");
		pStartTimeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pStartTimeEdit = new CTimeEditControl(this);
		m_pStartTimeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		connect(m_pStartTimeEdit, &CTimeEditControl::timeChanged, [this](const QTime& time)
		{
			const float fTODTime = QTimeToFloat(time);
			UpdateToDAdvancedInfo(m_advancedInfo, [fTODTime](ITimeOfDay::SAdvancedInfo& sAdvInfo) { sAdvInfo.fStartTime = fTODTime; });
		});
		pMediaBarLayoutLeft->addWidget(pStartTimeLabel);
		pMediaBarLayoutLeft->addWidget(m_pStartTimeEdit);
		pMediaBarLayoutLeft->addStretch();

		QLabel* pCurrentTimeLabel = new QLabel("Current:");
		pCurrentTimeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pCurrentTimeEdit = new CTimeEditControl(this);
		m_pCurrentTimeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		connect(m_pCurrentTimeEdit, &CTimeEditControl::timeChanged, this, &QTimeOfDayWidget::CurrentTimeEdited);

		QToolButton* pStopButton = new QToolButton;
		pStopButton->setIcon(CryIcon("icons:Trackview/Stop_Sequence.ico"));
		pStopButton->setCheckable(true);
		pStopButton->setChecked(!m_bIsPlaying);
		pStopButton->setAutoExclusive(true);

		QToolButton* pPlayButton = new QToolButton;
		pPlayButton->setIcon(CryIcon("icons:Trackview/Play_Sequence.ico"));
		pPlayButton->setCheckable(true);
		pPlayButton->setChecked(m_bIsPlaying);
		pPlayButton->setAutoExclusive(true);

		QLabel* pPlaySpeedLabel = new QLabel("Speed: ");
		pPlaySpeedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pPlaySpeedEdit = new QLineEdit;
		m_pPlaySpeedEdit->setFixedWidth(37);
		m_pPlaySpeedEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		QDoubleValidator* validator = new QDoubleValidator(0.0, 99.999, 3, m_pPlaySpeedEdit);
		validator->setNotation(QDoubleValidator::StandardNotation);
		m_pPlaySpeedEdit->setValidator(validator);
		m_pPlaySpeedEdit->setToolTip("TimeOfDay play speed\n Valid range:[0.000 - 99.999]");

		connect(m_pPlaySpeedEdit, &QLineEdit::editingFinished, [this]()
		{
			m_pCurveEdit->setFocus();
			const QString value = m_pPlaySpeedEdit->text();
			const float fFloatValue = value.toFloat();
			UpdateToDAdvancedInfo(m_advancedInfo, [fFloatValue](ITimeOfDay::SAdvancedInfo& sAdvInfo) { sAdvInfo.fAnimSpeed = fFloatValue; });
		});

		connect(pStopButton, &QToolButton::clicked, [this]() { m_bIsPlaying = false; m_pPlaySpeedEdit->clearFocus(); m_pPropertyTree->setEnabled(true); });
		connect(pPlayButton, &QToolButton::clicked, [this]() { m_bIsPlaying = true; m_pPlaySpeedEdit->clearFocus();  m_pPropertyTree->setEnabled(false); });

		pMediaBarLayoutCenter->setSpacing(0);
		pMediaBarLayoutCenter->setMargin(0);

		pMediaBarLayoutCenter->addWidget(pCurrentTimeLabel);
		pMediaBarLayoutCenter->addWidget(m_pCurrentTimeEdit);
		pMediaBarLayoutCenter->addSpacing(5);
		pMediaBarLayoutCenter->addWidget(pStopButton);
		pMediaBarLayoutCenter->addWidget(pPlayButton);
		pMediaBarLayoutCenter->addSpacing(5);
		pMediaBarLayoutCenter->addWidget(pPlaySpeedLabel);
		pMediaBarLayoutCenter->addWidget(m_pPlaySpeedEdit);
		pMediaBarLayoutCenter->addSpacing(15);

		QLabel* pEndTimeLabel = new QLabel("End:");
		pEndTimeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

		m_pEndTimeEdit = new CTimeEditControl(this);
		m_pEndTimeEdit->setTime(QTime(23, 59));
		m_pEndTimeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		connect(m_pEndTimeEdit, &CTimeEditControl::timeChanged, [this](const QTime& time)
		{
			const float fTODTime = QTimeToFloat(time);
			UpdateToDAdvancedInfo(m_advancedInfo, [fTODTime](ITimeOfDay::SAdvancedInfo& sAdvInfo) { sAdvInfo.fEndTime = fTODTime; });
		});

		pMediaBarLayoutRight->addStretch();
		pMediaBarLayoutRight->addWidget(pEndTimeLabel, Qt::AlignRight);
		pMediaBarLayoutRight->addWidget(m_pEndTimeEdit, Qt::AlignRight);

		pBottomLayout->addLayout(pMediaBarLayout, Qt::AlignTop);
	}
}

void QTimeOfDayWidget::LoadPropertiesTrees()
{
	if (m_groups.m_params.size())
	{
		return;
	}

	m_pPropertyTree->detach();
	m_pPropertyTree->setArchiveContext(m_pContextList->Tail());

	ITimeOfDay::IPreset* pPreset = GetPreset();
	ITimeOfDay::IVariables& variables = pPreset->GetVariables();
	const int nTODParamCount = variables.GetVariableCount();

	std::map<string, size_t> tempMap;
	int nGroupId = 0;
	for (int varID = 0; varID < nTODParamCount; ++varID)
	{
		ITimeOfDay::SVariableInfo sVarInfo;
		if (variables.GetVariableInfo(varID, sVarInfo))
		{
			auto it = tempMap.find(sVarInfo.szGroup);
			if (it == tempMap.end())
			{
				tempMap[sVarInfo.szGroup] = nGroupId++;
			}
		}
	}

	m_groups.m_propertyGroups.resize(nGroupId);
	for (int varID = 0; varID < nTODParamCount; ++varID)
	{
		ITimeOfDay::SVariableInfo sVarInfo;
		if (variables.GetVariableInfo(varID, sVarInfo))
		{
			const int nParamID = sVarInfo.nParamId;
			const char* sGroupName = sVarInfo.szGroup;

			const int groupId = tempMap[sGroupName];

			STODParameterGroup& group = m_groups.m_propertyGroups[groupId];
			group.m_id = groupId;
			group.m_name = sGroupName;

			STODParameter todParam;
			todParam.SetParamID(nParamID);
			todParam.SetName(sVarInfo.szName);
			todParam.SetGroupName(sGroupName);
			todParam.SetLabel(sVarInfo.szDisplayName);
			todParam.SetTODParamType((sVarInfo.type == ITimeOfDay::TYPE_FLOAT) ? eFloatType : eColorType);
			todParam.SetValue(Vec3(sVarInfo.fValue[0], sVarInfo.fValue[1], sVarInfo.fValue[2]));
			todParam.m_pGroup = &group;
			todParam.m_IDWithinGroup = group.m_Params.size();
			group.m_Params.push_back(todParam);
		}
	}

	//fill param map for fast update
	m_groups.m_params.resize(nTODParamCount);
	for (STODParameterGroupSet::TPropertyGroupMap::iterator it = m_groups.m_propertyGroups.begin(); it != m_groups.m_propertyGroups.end(); ++it)
	{
		STODParameterGroup& group = *it;
		const size_t nParamSize = group.m_Params.size();
		for (size_t i = 0; i < nParamSize; ++i)
		{
			STODParameter* pParam = &group.m_Params[i];
			const int nParamId = pParam->GetParamID();
			if (nParamId >= 0 && nParamId < m_groups.m_params.size())
			{
				m_groups.m_params[pParam->GetParamID()] = pParam;
			}
			else
			{
				assert(false);
				CryLogAlways("ERROR: QTimeOfDayWidget::LoadPropertiesTree() Parameter mismatch");
			}
		}
	}

	m_pPropertyTree->attach(Serialization::SStruct(m_groups));
	m_pPropertyTreeConst->attach(Serialization::SStruct(pPreset->GetConstants()));

	//By default, trees are collapsed. If layout is present, it will restore state
	m_pPropertyTree->collapseAll();
	m_pPropertyTreeConst->collapseAll();
}

void QTimeOfDayWidget::UpdateValues()
{
	UpdateVarPropTree();
	UpdateCurveTime();
	UpdateCurrentTimeEdit();

	GetTimeOfDay()->GetAdvancedInfo(m_advancedInfo);

	const QTime startTime = FloatToQTime(m_advancedInfo.fStartTime);
	SetQTimeEditTimeBlocking(m_pStartTimeEdit, startTime);
	const QTime endTime = FloatToQTime(m_advancedInfo.fEndTime);
	SetQTimeEditTimeBlocking(m_pEndTimeEdit, endTime);

	m_pPlaySpeedEdit->blockSignals(true);
	m_pPlaySpeedEdit->setText(QString::number(m_advancedInfo.fAnimSpeed));
	m_pPlaySpeedEdit->blockSignals(false);

	UpdateConstPropTree();
}

void QTimeOfDayWidget::UpdateSelectedParamId()
{
	PropertyRow* pRow = m_pPropertyTree->selectedRow();
	if (!pRow)
	{
		m_selectedParamId = -1;
		return;
	}

	STODParameter* pParam = NULL;
	while (pRow && pRow->parent())
	{
		if (STODParameterGroup* pGroup = pRow->parent()->serializer().cast<STODParameterGroup>())
		{
			int index = pRow->parent()->childIndex(pRow);
			if (index >= 0 && index < pGroup->m_Params.size())
				pParam = &pGroup->m_Params[index];
			break;
		}
		pRow = pRow->parent();
	}

	m_selectedParamId = pParam ? pParam->GetParamID() : -1;
}

void QTimeOfDayWidget::UpdateVarPropTree()
{
	ITimeOfDay::IVariables& variables = GetPreset()->GetVariables();
	const int nToDVarCount = variables.GetVariableCount();
	for (int varID = 0; varID < nToDVarCount; ++varID)
	{
		ITimeOfDay::SVariableInfo sVarInfo;
		if (variables.GetVariableInfo(varID, sVarInfo))
		{
			if (sVarInfo.nParamId >= 0 && sVarInfo.nParamId < m_groups.m_params.size())
			{
				STODParameter* pParam = m_groups.m_params[sVarInfo.nParamId];
				pParam->SetValue(Vec3(sVarInfo.fValue[0], sVarInfo.fValue[1], sVarInfo.fValue[2]));
			}
		}
	}
	m_pPropertyTree->revert();
}

void QTimeOfDayWidget::UpdateCurrentTimeEdit()
{
	const float fTime = GetTimeOfDay()->GetTime();
	QTime currentTime = FloatToQTime(fTime);

	SetQTimeEditTimeBlocking(m_pCurrentTimeEdit, currentTime);
}

void QTimeOfDayWidget::UpdateCurveTime()
{
	const float fTime = GetTimeOfDay()->GetTime();
	const float fSplineTime = (fTime / 24.0f) * m_fAnimTimeSecondsIn24h;
	m_pCurveEdit->SetTime(SAnimTime(fSplineTime));
}

bool QTimeOfDayWidget::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_pCurveEdit)
	{
		//forward some keys to m_propertyTreeVar, when its not focused
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if ((keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) ||
			    (keyEvent->key() == Qt::Key_Left) || (keyEvent->key() == Qt::Key_Right) ||
			    (keyEvent->key() == Qt::Key_F && keyEvent->modifiers() == Qt::CTRL) ||
			    (keyEvent->key() == Qt::Key_Enter) || (keyEvent->key() == Qt::Key_Return)
			    )
			{
				return QApplication::sendEvent(m_pPropertyTree, event);
			}
		}
	}

	return QObject::eventFilter(obj, event);
}

bool QTimeOfDayWidget::event(QEvent *event)
{
	if (event->type() == QEvent::WindowActivate || isActiveWindow() && event->type() == QEvent::Show)
	{
		// Thus, we ignore a bunch of state change events when the application is activated. 
		// By postponing the processing to the next frame, we make the final decision about 
		// the active state of the editor, when, probably, all windows receive the final state.
		QTimer::singleShot(0, [this]()
		{
			if (isActiveWindow() && !m_preset.empty())
			{
				GetTimeOfDay()->SetAdvancedInfo(m_advancedInfo);
				const float fTODTime = QTimeToFloat(m_pCurrentTimeEdit->time());
				SetTODTime(fTODTime);
				GetTimeOfDay()->PreviewPreset(m_preset);
			}
		});
	}
	return QObject::event(event);
}

void QTimeOfDayWidget::UpdateConstPropTree()
{
	m_pPropertyTree->revert();
	m_pPropertyTreeConst->attach(Serialization::SStruct(GetPreset()->GetConstants()));
}

void QTimeOfDayWidget::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnIdleUpdate)
	{
		OnIdleUpdate();
	}
	else if (event == eNotify_OnStyleChanged)
	{
		// refresh curves color by updating content
		UpdateCurveContent();
	}
	else if (event == eNotify_OnTimeOfDayChange && !m_bIsEditing && isActiveWindow() && GetPreset())
	{
		UpdateCurveContent();
		UpdateVarPropTree();
		UpdateConstPropTree(); //Sun can be changed
	}
}
