// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_LogWidget.h"

#include <QBoxLayout>
#include <QLineEdit>
#include <QParentWndWidget.h>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <CryEntitySystem/IEntitySystem.h>
#include <CryLink/CryLink.h>
#include <CrySerialization/IArchiveHost.h>
#include <QPropertyTree/QPropertyTree.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>
#include <Schematyc/Services/Schematyc_ILog.h>
#include <Schematyc/Services/Schematyc_ILogRecorder.h>
#include <Schematyc/Services/Schematyc_LogStreamName.h>
#include <Schematyc/Utils/Schematyc_StackString.h>

#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
namespace LogWidgetUtils
{
inline bool FilterMessage(const SLogSettings& settings, const LogStreamId& streamId, const EntityId entityId, const ELogMessageType messageType)
{
	switch (messageType)
	{
	case ELogMessageType::Comment:
		{
			if (!settings.bShowComments)
			{
				return false;
			}
			break;
		}
	case ELogMessageType::Warning:
		{
			if (!settings.bShowWarnings)
			{
				return false;
			}
			break;
		}
	case ELogMessageType::Error:
	case ELogMessageType::CriticalError:
	case ELogMessageType::FatalError:
		{
			if (!settings.bShowErrors)
			{
				return false;
			}
			break;
		}
	}

	if (!settings.entity.empty())
	{
		const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
		if (!pEntity || (settings.entity != pEntity->GetName()))
		{
			return false;
		}
	}

	const SLogStreamName streamName(GetSchematycFramework().GetLog().GetStreamName(streamId));
	auto compareStreamName = [&streamName](const SLogStreamName& rhs)
	{
		return streamName.value == rhs.value;
	};
	return std::find_if(settings.streams.begin(), settings.streams.end(), compareStreamName) != settings.streams.end();
}

void FormatMessage(const SLogSettings& settings, QString& output, const CLogMetaData* pMetaData,  ELogMessageType messageType, const char* szMessage)
{
	CStackString color = "lightgray";
	switch (messageType)
	{
	case ELogMessageType::Warning:
		{
			color = "yellow";
			break;
		}
	case ELogMessageType::Error:
	case ELogMessageType::CriticalError:
	case ELogMessageType::FatalError:
		{
			color = "red";
			break;
		}
	}

	CStackString text;

	if (settings.bShowOrigin && pMetaData)
	{
		CStackString function;
		pMetaData->Get(ELogMetaField::Function, function);
		if (!function.empty())
		{
			text.append("[");
			text.append(function.c_str());
			text.append("] ");
		}
	}

	if (settings.bShowEntity && pMetaData)
	{
		EntityId entityId = INVALID_ENTITYID;
		pMetaData->Get(ELogMetaField::EntityId, entityId);
		if (entityId != INVALID_ENTITYID)
		{
			const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
			if (pEntity)
			{
				text.append(pEntity->GetName());
				text.append(" : ");
			}
		}
	}

	text.append(szMessage);
	
	text.replace("\n", "<br>");

	CStackString uri;
	if (pMetaData)
	{
		pMetaData->CreateUri(uri);
	}
	if (!uri.empty())
	{
		output.append("<a href=\"");
		output.append(uri.c_str());
		output.append("\" style=\"color:");
		output.append(color.c_str());
		output.append("\">");
		output.append(text.c_str());
		output.append("</a>");
	}
	else
	{
		output.append("<font color=\"");
		output.append(color.c_str());
		output.append("\">");
		output.append(text.c_str());
	}
}
} // LogWidgetUtils

SLogSettings::SLogSettings()
	: bShowComments(true)
	, bShowWarnings(true)
	, bShowErrors(true)
	, bShowEntity(false)
	, bShowOrigin(false)
{}

void SLogSettings::Serialize(Serialization::IArchive& archive)
{
	archive(entity, "entity", "Entity");
	archive(streams, "streams", "Streams");
	archive(bShowComments, "bShowComments", "Show Comments");
	archive(bShowWarnings, "bShowWarnings", "Show Warnings");
	archive(bShowErrors, "bShowErrors", "Show Errors");
	archive(bShowEntity, "bShowEntity", "Show Entity");
	archive(bShowOrigin, "bShowOrigin", "Show Origin");
}

CLogSettingsWidget::CLogSettingsWidget(QWidget* pParent)
	: QWidget(pParent)
{
	m_pLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pAttachToSelectedEntityButton = new QPushButton("Attach To Selected Entity", this);
	m_pPropertyTree = new QPropertyTree(this);

	connect(m_pAttachToSelectedEntityButton, SIGNAL(clicked()), this, SLOT(OnAttachToSelectedEntityButtonClicked()));
}

void CLogSettingsWidget::InitLayout()
{
	QWidget::setLayout(m_pLayout);
	m_pLayout->setSpacing(2);
	m_pLayout->setMargin(4);
	m_pLayout->addWidget(m_pAttachToSelectedEntityButton, 1);
	m_pLayout->addWidget(m_pPropertyTree, 1);

	m_pPropertyTree->setSizeHint(QSize(250, 250));
	m_pPropertyTree->setExpandLevels(1);
	m_pPropertyTree->setSliderUpdateDelay(5);
	m_pPropertyTree->setValueColumnWidth(0.6f);
	m_pPropertyTree->attach(Serialization::SStruct(*this));
}

void CLogSettingsWidget::ApplySettings(const SLogSettings& settings)
{
	m_settings = settings;
}

const SLogSettings& CLogSettingsWidget::GetSettings() const
{
	return m_settings;
}

void CLogSettingsWidget::Serialize(Serialization::IArchive& archive)
{
	m_settings.Serialize(archive);
}

void CLogSettingsWidget::OnAttachToSelectedEntityButtonClicked()
{
	const IEntity* pEntity = gEnv->pEntitySystem->GetEntity(PluginUtils::GetSelectedEntityId());
	m_settings.entity = pEntity ? pEntity->GetName() : "";
	m_pPropertyTree->revert();
}

CLogWidget::CLogWidget(QWidget* pParent)
	: QWidget(pParent)
{
	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pSplitter = new QSplitter(Qt::Horizontal, this);
	m_pSettings = new CLogSettingsWidget(this);
	m_pOutput = new QTextBrowser(this);
	m_pControlLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_pShowHideSettingsButton = new QPushButton(">> Show Settings", this);
	m_pClearOutputButton = new QPushButton("Clear Output", this);

	ShowSettings(false);

	m_pOutput->setContextMenuPolicy(Qt::NoContextMenu);
	m_pOutput->setUndoRedoEnabled(false);
	m_pOutput->setOpenLinks(false);

	connect(m_pShowHideSettingsButton, SIGNAL(clicked()), this, SLOT(OnShowHideSettingsButtonClicked()));
	connect(m_pClearOutputButton, SIGNAL(clicked()), this, SLOT(OnClearOutputButtonClicked()));
	connect(m_pOutput, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(OnLinkClicked(const QUrl &)));

	GetSchematycFramework().GetLogRecorder().VisitMessages(Schematyc::Delegate::Make(*this, &CLogWidget::VisitRecordedLogMessage));

	GetSchematycFramework().GetLog().GetMessageSignalSlots().Connect(Schematyc::Delegate::Make(*this, &CLogWidget::OnLogMessage), m_connectionScope);

	QWidget::startTimer(80);
}

void CLogWidget::InitLayout()
{
	QWidget::setLayout(m_pMainLayout);
	m_pMainLayout->setSpacing(1);
	m_pMainLayout->setMargin(0);
	m_pMainLayout->addWidget(m_pSplitter, 1);
	m_pMainLayout->addLayout(m_pControlLayout, 0);

	m_pSplitter->setStretchFactor(0, 1);
	m_pSplitter->setStretchFactor(1, 0);
	m_pSplitter->addWidget(m_pSettings);
	m_pSplitter->addWidget(m_pOutput);

	QList<int> splitterSizes;
	splitterSizes.push_back(60);
	splitterSizes.push_back(200);
	m_pSplitter->setSizes(splitterSizes);

	m_pSettings->InitLayout();

	m_pControlLayout->setSpacing(2);
	m_pControlLayout->setMargin(4);
	m_pControlLayout->addWidget(m_pShowHideSettingsButton, 1);
	m_pControlLayout->addWidget(m_pClearOutputButton, 1);
}

void CLogWidget::ApplySettings(const SLogSettings& settings)
{
	m_pSettings->ApplySettings(settings);
}

void CLogWidget::LoadSettings(const XmlNodeRef& xml)
{
	Serialization::LoadXmlNode(*m_pSettings, xml);
}

XmlNodeRef CLogWidget::SaveSettings(const char* szName)
{
	return Serialization::SaveXmlNode(*m_pSettings, szName);
}

void CLogWidget::ShowSettings(bool bShowSettings)
{
	if(bShowSettings)
	{
		m_pShowHideSettingsButton->setVisible(true);
	}
	else
	{
		m_pSettings->setVisible(false);
		m_pShowHideSettingsButton->setVisible(false);
	}
}

void CLogWidget::OnShowHideSettingsButtonClicked()
{
	const bool bShowSettings = !m_pSettings->isVisible();
	m_pShowHideSettingsButton->setText(bShowSettings ? "<< Hide Settings" : ">> Show Settings");
	m_pSettings->setVisible(bShowSettings);
}

void CLogWidget::OnClearOutputButtonClicked()
{
	m_pOutput->clear();
}

void CLogWidget::OnLinkClicked(const QUrl& link)
{
	QString qString = link.toDisplayString();
	QByteArray qBytes = qString.toLocal8Bit();
	CryLinkUtils::ExecuteUri(qBytes.constData());
}

void CLogWidget::timerEvent(QTimerEvent* pTimerEvent)
{
	m_pOutput->setUpdatesEnabled(false);
	for (const QString& pendingMessage : m_pendingMessages)
	{
		m_pOutput->append(pendingMessage);
	}
	m_pendingMessages.clear();
	m_pOutput->setUpdatesEnabled(true);
}

EVisitStatus CLogWidget::VisitRecordedLogMessage(const SLogMessageData& logMessageData)
{
	WriteLogMessageToOutput(logMessageData);
	return EVisitStatus::Continue;
}

void CLogWidget::OnLogMessage(const SLogMessageData& logMessageData)
{
	if (QWidget::isVisible())
	{
		EntityId entityId = INVALID_ENTITYID;
		if (logMessageData.pMetaData)
		{
			logMessageData.pMetaData->Get(ELogMetaField::EntityId, entityId);
		}
		if (LogWidgetUtils::FilterMessage(m_pSettings->GetSettings(), logMessageData.streamId, entityId, logMessageData.messageType))
		{
			WriteLogMessageToOutput(logMessageData);
		}
	}
}

void CLogWidget::WriteLogMessageToOutput(const SLogMessageData& logMessageData)
{
	QString message;
	LogWidgetUtils::FormatMessage(m_pSettings->GetSettings(), message, logMessageData.pMetaData, logMessageData.messageType, logMessageData.szMessage);
	m_pendingMessages.push_back(message);
}
} // Schematyc
