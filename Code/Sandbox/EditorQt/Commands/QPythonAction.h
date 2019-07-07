// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SandboxAPI.h"
#include <QAction>

class QWidget;

// Special Qt Action derived class to bind with python calls
class SANDBOX_API QPythonAction : public QAction
{
public:
	QPythonAction(const QString& actionName, const QString& actionText, QObject* parent, const char* pythonCall = 0);
	QPythonAction(const QString& actionName, const char* pythonCall, QObject* parent);
	~QPythonAction() {}

protected:
	void OnTriggered();
};
