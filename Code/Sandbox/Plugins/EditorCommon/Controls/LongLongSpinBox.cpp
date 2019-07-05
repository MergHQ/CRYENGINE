// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "LongLongSpinBox.h"

#include <QLineEdit>

CLongLongSpinBox::CLongLongSpinBox(QWidget* pParent)
	: QAbstractSpinBox(pParent)
	, m_minimum(std::numeric_limits<qlonglong>::min())
	, m_maximum(std::numeric_limits<qlonglong>::max())
{
	connect(lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(OnEditFinished()));
}

qlonglong CLongLongSpinBox::Value() const
{
	return m_value;
}

qlonglong CLongLongSpinBox::Minimum() const
{
	return m_minimum;
}

void CLongLongSpinBox::SetMinimum(qlonglong min)
{
	m_minimum = min;
}

qlonglong CLongLongSpinBox::Maximum() const
{
	return m_maximum;
}

void CLongLongSpinBox::SetMaximum(qlonglong max)
{
	m_maximum = max;
}

void CLongLongSpinBox::SetValue(qlonglong val)
{
	if (m_value != val)
	{
		lineEdit()->setText(TextFromValue(val));
		m_value = val;
	}
}

void CLongLongSpinBox::OnEditFinished()
{
	QString input = lineEdit()->text();
	int pos = 0;
	if (QValidator::Acceptable == validate(input, pos))
		SetValue(ValueFromText(input));
	else
		lineEdit()->setText(TextFromValue(m_value));
}

void CLongLongSpinBox::StepBy(int steps)
{
	auto new_value = m_value;
	if (steps < 0 && new_value + steps > new_value)
	{
		new_value = std::numeric_limits<qlonglong>::min();
	}
	else if (steps > 0 && new_value + steps < new_value)
	{
		new_value = std::numeric_limits<qlonglong>::max();
	}
	else
	{
		new_value += steps;
	}

	lineEdit()->setText(TextFromValue(new_value));
	SetValue(new_value);
}

qlonglong CLongLongSpinBox::ValueFromText(const QString& text) const
{
	return text.toLongLong();
}

QString CLongLongSpinBox::TextFromValue(qlonglong val) const
{
	return QString::number(val);
}
