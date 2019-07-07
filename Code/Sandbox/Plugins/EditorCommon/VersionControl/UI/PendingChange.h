// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <vector>
#include <CrySandbox/CrySignal.h>
#include <QString>

//! Abstract class that represents a single pending change.
class EDITOR_COMMON_API CPendingChange
{
public:
	//! signal that is fired when data is changed indirectly so that the view can be refreshed immediately.
	//! This can happen, for example, when a layer is selected (directly) that causes also the corresponding
	//! level to be selected (indirectly).
	static CCrySignal<void(const std::vector<CPendingChange*>&)> s_signalDataUpdatedIndirectly;

	virtual ~CPendingChange() = default;

	//! The name to be displayed in the pending changes list.
	const QString& GetName() const { return m_name; }

	//! The type to be displayed in the pending changes list.
	const QString& GetTypeName() const { return m_typeName; }

	//! The folder for the pending change to be displayed.
	const QString& GetLocation() const { return m_location; }

	//! Returns a main handle file that represents the pending change.
	const string&  GetMainFile() const { return m_mainFile; }

	//! Returns all the files related to the current pending change.
	const std::vector<string>& GetFiles() const { return m_files; }

	//! This method is called when the pending change is selected/deselected for the submission,
	//! e.g. on user interaction.
	virtual void Check(bool shouldCheck) { SetChecked(shouldCheck); }

	//! Marks the pending change as checked/unchecked.
	virtual void SetChecked(bool value) { m_isChecked = value; }

	//! Says if the pending change is selected to the submission.
	bool IsChecked() const { return m_isChecked; }

	//! Specifies if the pending change is in a valid state, e.g. ready to be handled by version control system.
	//! A simple case when it can be invalid is if a main file (not marked for deletion) is not on the file system.
	virtual bool IsValid() const { return true; }

protected:
	QString m_name;
	QString m_typeName;
	QString m_location;
	string  m_mainFile;
	std::vector<string> m_files;

private:
	bool m_isChecked{ false };
};

//! This class houses all pending changes.
class EDITOR_COMMON_API CPendingChangeList
{
public:
	//! Returns a pending change at the given index.
	static CPendingChange* GetPendingChange(int index);

	//! Returns the number of pending changes.
	static int GetNumPendingChanges();

	//! Finds and returns the pending change corresponding to the given file.
	//! @return nullptr is pending change is not present.
	static CPendingChange* FindPendingChangeFor(const string& file);

	//! Creates a pending change specific for the given file.
	static CPendingChange* CreatePendingChangeFor(const string& file);

	//! Call the provided function for every pending change in the list.
	static void ForEachPendingChange(std::function<void(CPendingChange&)>);

	//! Clears the current pending change list.
	static void Clear();
};
