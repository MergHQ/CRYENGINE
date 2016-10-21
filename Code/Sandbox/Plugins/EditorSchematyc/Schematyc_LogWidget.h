// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Store stream ids rather than names in order to optimize filtering.
// #SchematycTODO : Display separate report when opening editor rather than copying recorded messages to log?
// #SchematycTODO : Should we allow log widgets to output messages when tabbed (i.e. not visible but not closed)?

#pragma once

#include <QTextBrowser>
#include <QWidget>
#include <Schematyc/Services/Schematyc_LogStreamName.h>
#include <Schematyc/SerializationUtils/Schematyc_SerializationUtils.h>
#include <Schematyc/Utils/Schematyc_ScopedConnection.h>

class QBoxLayout;
class QLineEdit;
class QParentWndWidget;
class QPlainTextEdit;
class QPropertyTree;
class QPushButton;
class QSplitter;

namespace Schematyc
{
typedef std::vector<SLogStreamName> LogStreams;

struct SLogSettings
{
	SLogSettings();

	void Serialize(Serialization::IArchive& archive);

	string     entity;
	LogStreams streams;
	bool       bShowComments;
	bool       bShowWarnings;
	bool       bShowErrors;
	bool       bShowEntity;
	bool       bShowOrigin;
};

class CLogSettingsWidget : public QWidget
{
	Q_OBJECT

public:

	CLogSettingsWidget(QWidget* pParent);

	void                InitLayout();
	void                ApplySettings(const SLogSettings& settings);
	const SLogSettings& GetSettings() const;
	void                Serialize(Serialization::IArchive& archive);

public slots:

	void OnAttachToSelectedEntityButtonClicked();

private:

	SLogSettings   m_settings;
	QBoxLayout*    m_pLayout;
	QPushButton*   m_pAttachToSelectedEntityButton;
	QPropertyTree* m_pPropertyTree;
};

class CLogWidget : public QWidget
{
	Q_OBJECT

private:

	typedef std::vector<QString> PendingMessages;

public:

	CLogWidget(QWidget* pParent);

	void       InitLayout();
	void       ApplySettings(const SLogSettings& settings);
	void       LoadSettings(const XmlNodeRef& xml);
	XmlNodeRef SaveSettings(const char* szName);
	void       ShowSettings(bool bShowSettings);

public slots:

	void OnShowHideSettingsButtonClicked();
	void OnClearOutputButtonClicked();
	void OnLinkClicked(const QUrl& link);

protected:

	void timerEvent(QTimerEvent* pTimerEvent);

private:

	EVisitStatus VisitRecordedLogMessage(const SLogMessageData& logMessageData);
	void         OnLogMessage(const SLogMessageData& logMessageData);
	void         WriteLogMessageToOutput(const SLogMessageData& logMessageData);

private:

	QBoxLayout*         m_pMainLayout;
	QSplitter*          m_pSplitter;

	CLogSettingsWidget* m_pSettings;
	QTextBrowser*       m_pOutput;
	QBoxLayout*         m_pControlLayout;
	QPushButton*        m_pShowHideSettingsButton;
	QPushButton*        m_pClearOutputButton;

	PendingMessages     m_pendingMessages;
	CConnectionScope    m_connectionScope;
};
} // Schematyc
