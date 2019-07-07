// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ICharacterInstance;

class QString;

QString CreateCDF(
  const QString& skeletonFilePath,
  const QString& skinFilePath,
  const QString& materialFilePath);

QString CreateCDF(
	const QString& skeletonFilePath,
	const QStringList& skinsFilePaths,
	const QStringList& materialsFilePaths);

ICharacterInstance* CreateTemporaryCharacter(
  const QString& skeletonFilePath,
  const QString& skinFilePath,
  const QString& materialFilePath);
