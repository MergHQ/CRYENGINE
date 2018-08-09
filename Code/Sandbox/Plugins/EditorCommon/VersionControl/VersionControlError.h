// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

enum class EVersionControlError
{
	None = 0,
	AlreadyCheckedOutByOthers,
	SubmissionError,
	LoginFailed,
	InvalidSettings,
	Terminated,
	SessionExpired,
	CorruptedFileState,
	NoConnection,
	InvalidFileName,
	Unknown
};

struct SVersionControlError
{
	SVersionControlError(EVersionControlError type = EVersionControlError::None)
		: SVersionControlError(type, "")
	{}

	SVersionControlError(EVersionControlError type, const string& message)
		: type(type)
		, message(message)
	{
		isCritical = type == E::LoginFailed || type == E::InvalidSettings || type == E::Terminated
			|| type == E::NoConnection || type == E::SessionExpired;
	}

	string               message;
	EVersionControlError type;
	bool                 isCritical;

private:
	using E = EVersionControlError;
};
