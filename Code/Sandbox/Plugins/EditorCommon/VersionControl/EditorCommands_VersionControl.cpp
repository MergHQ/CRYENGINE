// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>

#include "BoostPythonMacros.h"
#include "EditorFramework/Events.h"
#include "IEditor.h"

namespace Private_EditorCommands
{

void Refresh()
{
	CommandEvent("version_control_system.refresh").SendToKeyboardFocus();
}

void GetLatest()
{
	CommandEvent("version_control_system.get_latest").SendToKeyboardFocus();
}

void CheckOut()
{
	CommandEvent("version_control_system.check_out").SendToKeyboardFocus();
}

void Revert()
{
	CommandEvent("version_control_system.revert").SendToKeyboardFocus();
}

void RevertUnchanged()
{
	CommandEvent("version_control_system.revert_unchanged").SendToKeyboardFocus();
}

void Submit()
{
	CommandEvent("version_control_system.submit").SendToKeyboardFocus();
}

void RefreshWorkFiles()
{
	CommandEvent("version_control_system.refresh_work_files").SendToKeyboardFocus();
}

void GetLatestWorkFiles()
{
	CommandEvent("version_control_system.get_latest_work_files").SendToKeyboardFocus();
}

void CheckOutWorkFiles()
{
	CommandEvent("version_control_system.check_out_work_files").SendToKeyboardFocus();
}

void RevertWorkFiles()
{
	CommandEvent("version_control_system.revert_work_files").SendToKeyboardFocus();
}

void RevertUnchangedWorkFiles()
{
	CommandEvent("version_control_system.revert_unchanged_work_files").SendToKeyboardFocus();
}

void SubmitWorkFiles()
{
	CommandEvent("version_control_system.submit_work_files").SendToKeyboardFocus();
}

void RemoveLocalWorkFiles()
{
	CommandEvent("version_control_system.remove_local_work_files").SendToKeyboardFocus();
}

}

DECLARE_PYTHON_MODULE(version_control_system);


REGISTER_PYTHON_COMMAND(Private_EditorCommands::Refresh, version_control_system, refresh, "Refresh status of assets/layers");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::GetLatest, version_control_system, get_latest, "Get latest assets/layers");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::CheckOut, version_control_system, check_out, "Check out assets/layers");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::Revert, version_control_system, revert, "Revert assets/layers");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::RevertUnchanged, version_control_system, revert_unchanged, "Revert assets/layers if unchanged");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::Submit, version_control_system, submit, "Open Submit dialog with selected assets/layers");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::RefreshWorkFiles, version_control_system, refresh_work_files, "Refresh status of work files");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::GetLatestWorkFiles, version_control_system, get_latest_work_files, "Get latest work files");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::CheckOutWorkFiles, version_control_system, check_out_work_files, "Check out work files");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::RevertWorkFiles, version_control_system, revert_work_files, "Revert work files");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::RevertUnchangedWorkFiles, version_control_system, revert_unchanged_work_files, "Revert work files if unchanged");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::SubmitWorkFiles, version_control_system, submit_work_files, "Open Submit dialog with selected work files");

REGISTER_PYTHON_COMMAND(Private_EditorCommands::RemoveLocalWorkFiles, version_control_system, remove_local_work_files, "Open Submit dialog with selected work files");
