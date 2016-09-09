// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleProfiler.h"
#include "ParticleComponentRuntime.h"
#include "ParticleEmitter.h"

CRY_PFX2_DBG

namespace pfx2
{

const CParticleProfiler::SStatDisplay statDisplay[] =
{
	{ 280.0f + 100.0f * 0.0f, EPS_Jobs,                "Jobs"                  },
	{ 280.0f + 100.0f * 1.0f, EPS_RendereredParticles, "Rendered"              },
	{ 280.0f + 100.0f * 2.0f, EPS_ActiveParticles,     "Active"                },
	{ 280.0f + 100.0f * 3.0f, EPS_AllocatedParticles,  "Alloc"                 },
	{ 280.0f + 100.0f * 5.0f, EPS_NewBornTime,         "New Borns (ms)"        },
	{ 280.0f + 100.0f * 6.0f, EPS_UpdateTime,          "Update (ms)"           },
	{ 280.0f + 100.0f * 7.0f, EPS_ComputeVerticesTime, "Compute Vertices (ms)" },
};

// Output stats to display or file
struct IStatOutput
{
	virtual void Column(cstr text) = 0;
	virtual void NewLine() = 0;
	virtual void NewPage() = 0;
	virtual bool IsDisplay() const { return false; }
};

struct CScreenOutput : IStatOutput
{
	CScreenOutput()
		: fontSz(1.2f)
		, lineSz(10)
		, columnSz(100)
		, startY(10)
		, color(Col_White)
	{
		m_curY = startY;
		m_curColumn = 0;
	}

	float              fontSz;
	float              lineSz;
	float              columnSz;
	float              startY;
	ColorF             color;
	std::vector<float> columnX;

	virtual void Column(cstr text)
	{
		float x = m_curColumn < (int)columnX.size() ?
		          columnX[m_curColumn] :
		          columnX.size() ? columnX.back() + columnSz * (m_curColumn + 1 - columnX.size()) :
		          columnSz * m_curColumn;
		IRenderAuxText::Draw2dLabel(x, m_curY, fontSz, color, false, "%s", text);
		m_curColumn++;
	}
	virtual void NewLine()
	{
		m_curY += lineSz;
		m_curColumn = 0;
	}
	virtual void NewPage()
	{
		m_curY = startY;
		m_curColumn = 0;
	}
	virtual bool IsDisplay() const
	{
		return true;
	}

private:
	float m_curY;
	int   m_curColumn;
};

struct CCSVFileOutput : IStatOutput
{
	CCSVFileOutput(cstr fileName)
	{
		m_file = gEnv->pCryPak->FOpen(fileName, "w");
	}
	~CCSVFileOutput()
	{
		gEnv->pCryPak->FClose(m_file);
	}

	virtual void Column(cstr text)
	{
		if (m_file)
			gEnv->pCryPak->FPrintf(m_file, "%s, ", text);
	}
	virtual void NewLine()
	{
		if (m_file)
			gEnv->pCryPak->FPrintf(m_file, "\n");
	}
	virtual void NewPage()
	{
	}

private:
	FILE* m_file;
};

CParticleProfiler::SStatistics::SStatistics()
{
	memset(m_values, 0, sizeof(m_values));
}

CParticleProfiler::CParticleProfiler()
	: m_entries(gEnv->pJobManager->GetNumWorkerThreads() + 1)
{
}

void CParticleProfiler::Display()
{
	CRY_PROFILE_FUNCTION(PROFILE_PARTICLE);

	CVars* pCVars = static_cast<C3DEngine*>(gEnv->p3DEngine)->GetCVars();
	if (pCVars->e_ParticlesDebug & AlphaBits('pf'))
	{
		if (pCVars->e_ParticlesDebug & AlphaBit('f'))
		{
#ifndef _RELEASE
			pCVars->e_ParticlesDebug &= ~AlphaBit('f');
			SaveToFile();
#endif
		}
		else
			SortEntries();

		if (pCVars->e_ParticlesDebug & AlphaBit('p'))
		{
			CScreenOutput output;
			output.columnX.push_back(20);
			output.columnX.push_back(40);
			for (const auto& stat : statDisplay)
				output.columnX.push_back(40 + stat.m_offsetX);

			WriteEntries(output);
		}
	}
	for (auto& entries : m_entries)
		entries.clear();
}

void CParticleProfiler::SaveToFile(cstr fileName)
{
	string genName;
	if (!fileName)
	{
		static char strDefault[] = "%USER%/ParticleProfile/EmitterProfile";
		genName = string(strDefault) + ".csv";
		for (uint i = 1; gEnv->pCryPak->IsFileExist(genName); ++i)
		{
			genName.Format("%s-%d.csv", strDefault, i);
		}
		fileName = genName;
	}
	CCSVFileOutput output(fileName);
	SortEntries();
	WriteEntries(output);
}

void CParticleProfiler::SortEntries()
{
	auto& finalElements = m_entries[0];
	for (auto& elements : m_entries)
	{
		finalElements.insert(finalElements.end(), elements.begin(), elements.end());
	}

	if (finalElements.empty())
		return;

	auto predicate = [](const SEntry& entry0, const SEntry& entry1)
	{
		if (entry0.m_pRuntime->GetEmitter()->GetEffect() != entry1.m_pRuntime->GetEmitter()->GetEffect())
			return entry0.m_pRuntime->GetEmitter()->GetEffect() < entry1.m_pRuntime->GetEmitter()->GetEffect();
		if (entry0.m_pRuntime->GetEmitter() != entry1.m_pRuntime->GetEmitter())
			return entry0.m_pRuntime->GetEmitter() < entry1.m_pRuntime->GetEmitter();
		if (entry0.m_pRuntime != entry1.m_pRuntime)
			return entry0.m_pRuntime < entry1.m_pRuntime;
		return entry0.m_type < entry1.m_type;
	};
	std::sort(finalElements.begin(), finalElements.end(), predicate);
}

void CParticleProfiler::WriteEntries(IStatOutput& output) const
{
	const auto& finalElements = m_entries[0];
	if (finalElements.size() <= 1)
		return;

	output.Column(output.IsDisplay() ? "" : "Effect");
	output.Column(output.IsDisplay() ? "" : "Component");
	for (const auto& stat : statDisplay)
		output.Column(stat.m_statName);
	output.NewLine();

	SStatistics componentStats;
	SStatistics emitterStats;
	SStatistics totalStats;

	const CParticleEmitter* pEmitter = nullptr;

	for (const SEntry& entry : finalElements)
	{
		if (entry.m_pRuntime->GetEmitter() != pEmitter)
		{
			pEmitter = entry.m_pRuntime->GetEmitter();
			if (output.IsDisplay())
			{
				output.Column(pEmitter->GetEffect()->GetFullName());
				output.NewLine();
			}
		}

		if (uint(entry.m_type) >= EPST_Float)
		{
			componentStats.m_values[entry.m_type].m_float += entry.m_value.m_float;
			emitterStats.m_values[entry.m_type].m_float += entry.m_value.m_float;
			totalStats.m_values[entry.m_type].m_float += entry.m_value.m_float;
		}
		else
		{
			componentStats.m_values[entry.m_type].m_int += entry.m_value.m_int;
			emitterStats.m_values[entry.m_type].m_int += entry.m_value.m_int;
			totalStats.m_values[entry.m_type].m_int += entry.m_value.m_int;
		}

		const SEntry* pNextEntry = &entry < &finalElements.back() ? &entry + 1 : nullptr;
		if (!pNextEntry || pNextEntry->m_pRuntime != entry.m_pRuntime)
		{
			// Component stats
			WriteStats(output, pEmitter->GetEffect()->GetFullName(), entry.m_pRuntime->GetComponent()->GetName(), componentStats);
			componentStats = SStatistics();

			if (!pNextEntry || pNextEntry->m_pRuntime->GetEmitter() != pEmitter)
			{
				// Emitter stats
				WriteStats(output, pEmitter->GetEffect()->GetFullName(), "total", emitterStats);
				emitterStats = SStatistics();
			}
		}
	}

	WriteStats(output, "", "Pfx2 Total", totalStats);
}

void CParticleProfiler::WriteStats(IStatOutput& output, cstr emitterName, cstr componentName, const SStatistics& stats) const
{
	output.Column(output.IsDisplay() ? "" : emitterName);
	output.Column(componentName);
	for (const auto& stat : statDisplay)
		WriteStat(output, stat, stats.m_values[stat.m_stat]);
	output.NewLine();
}

void CParticleProfiler::WriteStat(IStatOutput& output, SStatDisplay stat, SValue value) const
{
	if (stat.m_stat >= EPST_Float)
	{
		output.Column(string().Format("%2.3f", value.m_float * 1000.0f));
	}
	else
	{
		output.Column(string().Format("%d", value.m_int));
	}
}

}
