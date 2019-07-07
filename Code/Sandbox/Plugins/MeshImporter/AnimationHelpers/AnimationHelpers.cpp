// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AnimationHelpers.h"
#include "DialogCommon.h"  // CreateTemporaryDirectory

#include <IEditor.h>
#include <CryAnimation/ICryAnimation.h>

#include <QTemporaryDir>
#include <QTextStream>

#include <memory.h>

struct STemporaryFile
{
	std::unique_ptr<QTemporaryDir> m_pTempDir;
	QString                        m_filePath;
};

extern void LogPrintf(const char* szFormat, ...);

namespace Private_AnimationHelpers
{

void WriteCDF(STemporaryFile& tempFile, const QString& cdf)
{
	if (cdf.isEmpty())
	{
		return;
	}

	tempFile.m_pTempDir = std::move(CreateTemporaryDirectory());

	const QString filePath = tempFile.m_pTempDir->path() + "/override.cdf";

	QFile file(filePath);
	if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))
	{
		return;
	}
	QTextStream out(&file);
	out << cdf;
	file.close();

	tempFile.m_filePath = filePath;
}

ICharacterInstance* LoadCharacter(const QString& cdfFilePath)
{
	ICharacterManager* const pCharManager = GetIEditor()->GetSystem()->GetIAnimationSystem();
	const char* const szFilename = cdfFilePath.toLocal8Bit().constData();
	ICharacterInstance* const pCharInstance = pCharManager->CreateInstance(szFilename, 0);
	if (!pCharInstance)
	{
		LogPrintf("%s: Unable to create character instance from file '%s'.\n", __FUNCTION__, szFilename);
	}
	return pCharInstance;
}

} // namespace Private_AnimationHelpers

QString CreateCDF(
	const QString& skeletonFilePath,
	const QString& skinFilePath,
	const QString& materialFilePath)
{
	return CreateCDF(skeletonFilePath, QStringList{ skinFilePath }, QStringList{ materialFilePath });
}

QString CreateCDF(
  const QString& skeletonFilePath,
  const QStringList& skinsFilePaths,
  const QStringList& materialsFilePaths)
{
	static const char* const szCdfContent =
	  "<CharacterDefinition>\n"
	  " <Model File=\"%1\"/>\n"
	  " %2 \n"
	  "</CharacterDefinition>\n";

	static const char* const szAttachmentListWrapper =
	  " <AttachmentList>\n"
	  "%1 \n"
	  " </AttachmentList>\n";

	static const char* const szAttachmentElement =
		"  <Attachment Type=\"CA_SKIN\" AName=\"skin%1\" Binding=\"%2\" %4 Flags=\"0\"/>\n";

	if (skeletonFilePath.isEmpty())
	{
		return QString();
	}

	QString attachments;
	
	for (int i = 0; i < skinsFilePaths.length(); ++i)
	{
		QString material;
		if (i < materialsFilePaths.length() && !materialsFilePaths[i].isEmpty())
		{
			material = QString("Material=\"%1\"").arg(materialsFilePaths[i]);
		}

		if (!skinsFilePaths[i].isEmpty())
		{
			attachments += QString(szAttachmentElement).arg(i).arg(skinsFilePaths[i]).arg(material);
		}
	}
	
	QString attachmentList;

	if (!attachments.isEmpty())
	{
		attachmentList = QString(szAttachmentListWrapper).arg(attachments);
	}

	return QString(szCdfContent).arg(skeletonFilePath).arg(attachmentList);
}

ICharacterInstance* CreateTemporaryCharacter(
  const QString& skeletonFilePath,
  const QString& skinFilePath,
  const QString& materialFilePath)
{
	using namespace Private_AnimationHelpers;

	static std::unique_ptr<QTemporaryDir> pTempDir;

	if (!pTempDir)
	{
		// Make sure that we have a path that does not conflict with any other CDFs.
		// Note that all instances of the MI can use the same cdf file path, because
		// we do not reference the CDF once the character has been loaded.
		pTempDir = CreateTemporaryDirectory();
	}

	const QString cdfFilePath = pTempDir->path() + "/override.cdf";

	const QString cdfContent = CreateCDF(skeletonFilePath, skinFilePath, materialFilePath);

	ICharacterManager* const pCharManager = GetIEditor()->GetSystem()->GetIAnimationSystem();
	pCharManager->InjectCDF(cdfFilePath.toLocal8Bit().constData(), cdfContent.toLocal8Bit().constData(), cdfContent.length());

	return LoadCharacter(cdfFilePath);
}
