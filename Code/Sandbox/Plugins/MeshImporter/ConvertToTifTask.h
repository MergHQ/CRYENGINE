// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "AsyncHelper.h"

#include <QString>
#include <QVector>

class CConvertToTifTask : public CAsyncTaskBase
{
public:
	typedef std::function<void (CConvertToTifTask*)> Callback;

	void SetCallback(const Callback& callback);
	void AddFile(const QString& filePath);

	// CAsyncTaskBase implementation.

	virtual bool InitializeTask() override;
	virtual bool PerformTask() override;
	virtual void FinishTask(bool bTaskSucceeded) override;
private:
	Callback         m_callback;
	QVector<QString> m_filePaths;
};
