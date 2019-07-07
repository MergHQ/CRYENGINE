// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryIcon.h>
#include <vector>

struct STemplateDescription
{
	string  language;

	string  templateName;    // Taken from cryproject file of the template
	string  templatePath;    // Relative path inside SDK directory. Used by Launcher
	string  templateFolder;  // Full path to the project folder (source of copy)
	string  cryprojFileName; // Example: "Game.cryproject"
	CryIcon icon;
};

class CTemplateManager
{
public:
	CTemplateManager();

	const std::vector<STemplateDescription>& GetTemplates() const;

private:
	const std::vector<STemplateDescription> m_templates;
};
