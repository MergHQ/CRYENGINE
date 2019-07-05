// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

#include <QAbstractSpinBox>

// Int64 spin box
class EDITOR_COMMON_API CLongLongSpinBox : public QAbstractSpinBox
{
	Q_OBJECT
public:
	explicit CLongLongSpinBox(QWidget* pParent = nullptr);

	virtual void StepBy(int steps);

	qlonglong    Value() const;
	qlonglong    Minimum() const;
	qlonglong    Maximum() const;
	void         SetMinimum(qlonglong min);
	void         SetMaximum(qlonglong max);

public Q_SLOTS:
	void SetValue(qlonglong val);
	void OnEditFinished();

Q_SIGNALS:
	void ValueChanged(qlonglong v);

private:
	qlonglong ValueFromText(const QString& text) const;
	QString   TextFromValue(qlonglong val) const;

private:
	qlonglong m_minimum;
	qlonglong m_maximum;
	qlonglong m_value;
};
