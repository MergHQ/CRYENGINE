// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlErrorHandler.h"
#include <CrySystem/ISystem.h>

namespace VersionControlErrorHandler
{

void Handle(SVersionControlError error)
{
	if (error.type == EVersionControlError::None)
	{
		return;
	}

	string errorMessage;
	switch (error.type)
	{
	case EVersionControlError::LoginFailed:
		errorMessage = "Version Control System: Login failed";
		break;
	case EVersionControlError::InvalidSettings:
		errorMessage = "Version Control System: Invalid settings";
		break;
	case EVersionControlError::SubmissionError:
		errorMessage = "Version Control System: Submission error";
		break;
	case EVersionControlError::Terminated:
		errorMessage = "Version Control System: Task is terminated";
		break;
	case EVersionControlError::CorruptedFileState:
		errorMessage = "Version Control System: Corrupted state of a file";
		break;
	case EVersionControlError::NoConnection:
		errorMessage = "Version Control System: No connection";
		break;
	case EVersionControlError::InvalidFileName:
		errorMessage = "Version Control System: Invalid file name";
		break;
	case EVersionControlError::OperationFailed:
		errorMessage = "Version Control System: Operation failed";
		break;
	case EVersionControlError::AlreadyCheckedOutByOthers:
		errorMessage = "Version Control System: Exclusive check out";
		break;
	case EVersionControlError::UpdateFileFailed:
		errorMessage = "Version Control System: failed to update file(s)";
		break;
	case EVersionControlError::Unknown:
		errorMessage = "Version Control System: Error";
		break;
	default:
		return;
	}

	if (!error.message.empty())
	{
		errorMessage += ".\n" + error.message;
	}

	CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, errorMessage);
}

}