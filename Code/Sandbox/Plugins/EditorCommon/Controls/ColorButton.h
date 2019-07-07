// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <CrySandbox/CrySignal.h>
#include <CryMath/Cry_Math.h>
#include <CryMath/Cry_Color.h>
#include <QPushButton>

class QColor;

class EDITOR_COMMON_API CColorButton : public QPushButton
{
	Q_OBJECT
public:
	CColorButton();

	void SetColor(const QColor& color);
	void SetColor(const ColorB& color);
	void SetColor(const ColorF& color);

	QColor GetQColor() const { return m_color; }
	ColorB GetColorB() const;
	ColorF GetColorF() const;

	void SetHasAlpha(bool hasAlpha) { m_hasAlpha = hasAlpha; }

	//!Sent when the color has changed and editing is finished
	CCrySignal<void(const QColor&)> signalColorChanged;

	//!Sent when the user is still editing the color
	CCrySignal<void(const QColor&)> signalColorContinuousChanged;

protected:
	void paintEvent(QPaintEvent* paintEvent) override;

private:
	void OnClick();

	QColor m_color;
	bool m_hasAlpha;
};
