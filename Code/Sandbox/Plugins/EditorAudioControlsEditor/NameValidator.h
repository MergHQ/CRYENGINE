// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QValidator>

namespace ACE
{
static QRegularExpression const s_regexInvalidFileName(QRegularExpression(R"([^<>:;,?"*|\\/]*)"));
static QRegularExpression const s_regexInvalidFilePath(QRegularExpression(R"([^<>:;,?"*|]*)"));

class CNameValidator final : public QRegularExpressionValidator
{
public:

	CNameValidator() = delete;
	CNameValidator(CNameValidator const&) = delete;
	CNameValidator(CNameValidator&&) = delete;
	CNameValidator& operator=(CNameValidator const&) = delete;
	CNameValidator& operator=(CNameValidator&&) = delete;

	explicit CNameValidator(QRegularExpression const& regex, QWidget* const pParent);
	virtual ~CNameValidator() override = default;

	// QRegularExpressionValidator
	virtual QValidator::State validate(QString& string, int& pos) const override;
	virtual void              fixup(QString& input) const override;
	// ~QRegularExpressionValidator

private:

	QWidget* const m_pParent;
	QString        m_toolTipText;
};
} // namespace ACE
