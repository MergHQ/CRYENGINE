// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "DialogLineDatabase.h"
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/XML/IXml.h>
#include <CrySerialization/STL.h>
#include <CrySerialization/CryStrings.h>
#include <CrySerialization/IArchiveHost.h>
#include <CryMath/Random.h>

#if defined(DRS_COLLECT_DEBUG_DATA)
	#include "ResponseSystem.h"
#endif

using namespace DRS;
using namespace CryDRS;

const char* CDialogLineDatabase::s_szFilename = "dialoglines.dialog";

//--------------------------------------------------------------------------------------------------
CDialogLineDatabase::CDialogLineDatabase()
{
	m_drsDialogBinaryFileFormatCVar = 0;
	REGISTER_CVAR2("drs_dialogBinaryFileFormat", &m_drsDialogBinaryFileFormatCVar, 0, VF_NULL, "Toggles if the dialog data should be loaded from JSON or Binary.\n");
}

//--------------------------------------------------------------------------------------------------
CDialogLineDatabase::~CDialogLineDatabase()
{
	gEnv->pConsole->UnregisterVariable("drs_dialogBinaryFileFormat", true);
}

//--------------------------------------------------------------------------------------------------
void CDialogLineDatabase::Serialize(Serialization::IArchive& ar)
{
	ar(m_lineSets, "lineSets", "+[+]LineSets");
}

//--------------------------------------------------------------------------------------------------
bool CDialogLineDatabase::InitFromFiles(const char* szFilePath)
{
	string filepath = PathUtil::AddSlash(szFilePath);
	filepath += s_szFilename;
	if (m_drsDialogBinaryFileFormatCVar != 0)
	{
		if (!Serialization::LoadBinaryFile(*this, filepath.c_str()))
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Could not load dialog file %s", filepath.c_str());
			return false;
		}
	}
	else
	{
		if (!Serialization::LoadJsonFile(*this, filepath.c_str()))
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Could not load dialog file %s", filepath.c_str());
			return false;
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
bool CDialogLineDatabase::Save(const char* szFilePath)
{
	string filepath = PathUtil::AddSlash(szFilePath);
	filepath += s_szFilename;
	if (m_drsDialogBinaryFileFormatCVar != 0)
	{
		if (!Serialization::SaveBinaryFile(filepath.c_str(), *this))
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Could not save dialog file %s", filepath.c_str());
			return false;
		}
	}
	else
	{
		if (!Serialization::SaveJsonFile(filepath.c_str(), *this))
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Could not save dialog file %s", filepath.c_str());
			return false;
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------------
const CDialogLine* CDialogLineDatabase::GetLineByID(const CHashedString& lineID, int* pOutPriority)
{
	for (CDialogLineSet& currentLineSet : m_lineSets)
	{
		if (currentLineSet.GetLineId() == lineID)
		{
			if (pOutPriority)
			{
				*pOutPriority = currentLineSet.GetPriority();
			}
			return currentLineSet.PickLine();
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
uint CDialogLineDatabase::GetLineSetCount() const
{
	return m_lineSets.size();
}

//--------------------------------------------------------------------------------------------------
IDialogLineSet* CDialogLineDatabase::GetLineSetByIndex(uint index)
{
	if (index < m_lineSets.size())
	{
		return &m_lineSets[index];
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
const IDialogLineSet* const CDialogLineDatabase::GetLineSetById(const CHashedString& lineID) const
{
	for (const CDialogLineSet& currentLineSet : m_lineSets)
	{
		if (currentLineSet.GetLineId() == lineID)
		{
			return &currentLineSet;
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
IDialogLineSet* CDialogLineDatabase::InsertLineSet(uint index)
{
	CDialogLineSet newSet;
	newSet.SetLineId(GenerateUniqueId("NEW_LINE"));
	return &(*m_lineSets.insert(m_lineSets.begin() + index, newSet));
}

//--------------------------------------------------------------------------------------------------
void CDialogLineDatabase::RemoveLineSet(uint index)
{
	if (index < m_lineSets.size())
	{
		m_lineSets.erase(m_lineSets.begin() + index);
	}
}

//--------------------------------------------------------------------------------------------------
CHashedString CDialogLineDatabase::GenerateUniqueId(const string& root) const
{
	int num = 0;
	CHashedString hash(root);
	while (GetLineSetById(hash))
	{
		hash = CHashedString(root + "_" + CryStringUtils::toString(++num));
	}
	return hash;
}

//--------------------------------------------------------------------------------------------------
void CDialogLineDatabase::SerializeLinesHistory(Serialization::IArchive& ar)
{
#if defined(DRS_COLLECT_DEBUG_DATA)
	CResponseSystem::GetInstance()->m_responseSystemDebugDataProvider.SerializeDialogLinesHistory(ar);
#endif
}

//--------------------------------------------------------------------------------------------------
bool CDialogLineDatabase::ExecuteScript(uint32 index)
{
#if defined(CRY_PLATFORM_WINDOWS)
	const string linescriptpath = PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR "linescript.bat";

	if (gEnv->pCryPak->IsFileExist(linescriptpath, ICryPak::eFileLocation_Any))
	{
		DRS::IDialogLineSet* pLineSet = GetLineSetByIndex(index);

		if (pLineSet)
		{
			for (int i = 0; i < pLineSet->GetLineCount(); i++)
			{
				const IDialogLine* pCurrentLine = pLineSet->GetLineByIndex(i);
				string startTriggerWithoutPrefix = pCurrentLine->GetStartAudioTrigger();

				const int prefixPos = startTriggerWithoutPrefix.find('_');
				if (prefixPos != string::npos)
				{
					startTriggerWithoutPrefix = startTriggerWithoutPrefix.c_str() + prefixPos + 1;
				}

				char szBuffer[1024];
				cry_sprintf(szBuffer, "@SET LINE_ID=%s&SET SUBTITLE=%s&SET AUDIO_TRIGGER=%s&SET AUDIO_TRIGGER_CLEANED=%s&SET ANIMATION_NAME=%s&SET STANDALONE_FILE=%s&%s",
				            pLineSet->GetLineId().GetText().c_str(),
				            pCurrentLine->GetText().c_str(),
				            pCurrentLine->GetStartAudioTrigger().c_str(),
				            startTriggerWithoutPrefix.c_str(),
				            pCurrentLine->GetLipsyncAnimation().c_str(),
				            pCurrentLine->GetStandaloneFile().c_str(),
				            linescriptpath.c_str());

				system(szBuffer);
			}
			return true;
		}
	}
#endif //CRY_PLATFORM_WINDOWS

	return false;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CDialogLineSet::CDialogLineSet()
	: m_priority(50)
	, m_flags((int)IDialogLineSet::EPickModeFlags::Random)
	, m_lastPickedLine(0)
{
	m_lines.push_back(CDialogLine());
}

//--------------------------------------------------------------------------------------------------
const CDialogLine* CDialogLineSet::PickLine()
{
	const int numLines = m_lines.size();

	if (numLines == 1)
	{
		return &m_lines[0];
	}
	else if (numLines == 0)
	{
		DrsLogWarning("DialogLineSet without a single concrete line in it detected");
		return nullptr;
	}

	if ((m_flags & (int)IDialogLineSet::EPickModeFlags::Random) > 0)
	{
		const int randomLineIndex = cry_random(0, numLines - 1);
		return &m_lines[randomLineIndex];
	}
	else if ((m_flags & (int)IDialogLineSet::EPickModeFlags::Sequential) > 0)
	{
		if (m_lastPickedLine >= numLines)
		{
			m_lastPickedLine = 0;
		}
		return &m_lines[m_lastPickedLine++];
	}
	else
	{
		DrsLogWarning("Could not pick a line from the dialogLineSet, because the RandomMode was set to an illegal value");
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
typedef std::pair<CHashedString, CDialogLineSet> DialogLinePair;
typedef std::pair<string, CDialogLineSet>        SortedDialogLinePair;
namespace std
{
struct DialogLinePairSerializer
{
	DialogLinePairSerializer(DialogLinePair& pair) : pair_(pair) {}
	void Serialize(Serialization::IArchive& ar)
	{
		ar(pair_.first, "lineID", "^LineID");
		ar(pair_.second, "value", "^+[+]");
	}
	DialogLinePair& pair_;
};
struct SortedDialogLinePairSerializer
{
	SortedDialogLinePairSerializer(SortedDialogLinePair& pair) : pair_(pair) {}
	void Serialize(Serialization::IArchive& ar)
	{
		ar(pair_.first, "lineID", "^LineID");
		ar(pair_.second, "value", "^+[+]");
	}
	SortedDialogLinePair& pair_;
};
bool Serialize(Serialization::IArchive& ar, DialogLinePair& pair, const char* name, const char* label)
{
	DialogLinePairSerializer keyValue(pair);
	return ar(keyValue, name, label);
}
bool Serialize(Serialization::IArchive& ar, SortedDialogLinePair& pair, const char* name, const char* label)
{
	SortedDialogLinePairSerializer keyValue(pair);
	return ar(keyValue, name, label);
}
}

//--------------------------------------------------------------------------------------------------
IDialogLine* CDialogLineSet::GetLineByIndex(uint index)
{
	if (index < m_lines.size())
	{
		return &m_lines[index];
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
IDialogLine* CDialogLineSet::InsertLine(uint index)
{
	return &(*m_lines.insert(m_lines.begin() + index, CDialogLine()));
}

//--------------------------------------------------------------------------------------------------
void CDialogLineSet::RemoveLine(uint index)
{
	if (index < m_lines.size())
	{
		m_lines.erase(m_lines.begin() + index);
	}
}

//--------------------------------------------------------------------------------------------------
void CDialogLineSet::Serialize(Serialization::IArchive& ar)
{
	ar(m_lineId, "lineID", "lineID");
	ar(m_priority, "priority", "Priority");
	ar(m_lastPickedLine, "lastPicked", nullptr);
	ar(m_flags, "flags", "+Flags");    //bitmask editor widget?
	ar(m_lines, "lineVariations", "+Lines");

#if !defined(_RELEASE)
	if (ar.isEdit())
	{
		if (m_lines.empty())
		{
			ar.error(m_priority, "DialogLineSet without a single concrete line");
		}
		if ((m_flags & (uint32)(EPickModeFlags::Random)) == 0)
		{
			ar.error(m_priority, "DialogLineSet without a correct LinePickMode found.");
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
CDialogLine::CDialogLine()
{}

//--------------------------------------------------------------------------------------------------
void CDialogLine::Serialize(Serialization::IArchive& ar)
{
	ar(m_text, "text", "^Text");
	ar(m_audioStartTrigger, "audioStartTrigger", "AudioStartTrigger");
	ar(m_audioStopTrigger, "audioStopTrigger", "AudioStopTrigger");
	ar(m_lipsyncAnimation, "lipsyncAnim", "LipsyncAnim");
	ar(m_standaloneFile, "standaloneFile", "StandaloneFile");

	if (ar.isEdit())
	{
		if (m_text.empty() && m_audioStopTrigger.empty() && m_audioStartTrigger.empty() && m_standaloneFile.empty())
		{
			ar.warning(m_text, "DialogLine without any data found");
		}
	}
}
