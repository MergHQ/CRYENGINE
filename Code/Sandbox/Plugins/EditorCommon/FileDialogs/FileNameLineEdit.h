// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once
#include "EditorCommonAPI.h"

#include <QLineEdit>

class EDITOR_COMMON_API CFileNameLineEdit
	: public QLineEdit
{
public:
	CFileNameLineEdit(QWidget* parent = nullptr);

	QStringList GetFileNames() const;
	
	void        SetFileNames(const QStringList& fileNames);
	void		SetExtensionForFiles(const QString& extension);
	void		Clear();
};
