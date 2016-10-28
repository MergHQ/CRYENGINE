// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CQueryHistoryInGameGUI::SHistoricQueryShortInfo
		//
		//===================================================================================

		CQueryHistoryInGameGUI::SHistoricQueryShortInfo::SHistoricQueryShortInfo(const ColorF& _color, const CQueryID& _queryID, const CQueryID& _parentQueryID, string&& _shortInfo)
			: color(_color)
			, queryID(_queryID)
			, parentQueryID(_parentQueryID)
			, shortInfo(_shortInfo)
		{}

		//===================================================================================
		//
		// CQueryHistoryInGameGUI::SHistoricQueryDetailedTextLine
		//
		//===================================================================================

		CQueryHistoryInGameGUI::SHistoricQueryDetailedTextLine::SHistoricQueryDetailedTextLine(const ColorF& _color, string&& _textLine)
			: color(_color)
			, textLine(_textLine)
		{}

		//===================================================================================
		//
		// CQueryHistoryInGameGUI::SFocusedItemDetailedTextLine
		//
		//===================================================================================

		CQueryHistoryInGameGUI::SFocusedItemDetailedTextLine::SFocusedItemDetailedTextLine(const ColorF& _color, string&& _textLine)
			: color(_color)
			, textLine(_textLine)
		{}

		//===================================================================================
		//
		// CQueryHistoryInGameGUI
		//
		//===================================================================================

		CQueryHistoryInGameGUI::CQueryHistoryInGameGUI(IQueryHistoryManager& queryHistoryManager)
			: m_queryHistoryManager(queryHistoryManager)
			, m_scrollIndexInHistoricQueries(s_noScrollIndex)
		{
			m_queryHistoryManager.RegisterQueryHistoryListener(this);
			GetISystem()->GetIInput()->AddEventListener(this);
		}

		CQueryHistoryInGameGUI::~CQueryHistoryInGameGUI()
		{
			m_queryHistoryManager.UnregisterQueryHistoryListener(this);
			GetISystem()->GetIInput()->RemoveEventListener(this);
		}

		void CQueryHistoryInGameGUI::OnQueryHistoryEvent(EEvent ev)
		{
			switch (ev)
			{
			case EEvent::QueryHistoryDeserialized:
				if (m_queryHistoryManager.GetCurrentQueryHistory() == IQueryHistoryManager::EHistoryOrigin::Deserialized)
				{
					RefreshListOfHistoricQueries();
					RefreshDetailedInfoAboutCurrentHistoricQuery();
					RefreshDetailedInfoAboutFocusedItem();
				}
				break;

			case EEvent::HistoricQueryJustFinishedInLiveQueryHistory:
				if (m_queryHistoryManager.GetCurrentQueryHistory() == IQueryHistoryManager::EHistoryOrigin::Live)
				{
					RefreshListOfHistoricQueries();
				}
				break;

			case EEvent::LiveQueryHistoryCleared:
				if (m_queryHistoryManager.GetCurrentQueryHistory() == IQueryHistoryManager::EHistoryOrigin::Live)
				{
					RefreshListOfHistoricQueries();
					RefreshDetailedInfoAboutCurrentHistoricQuery();
					RefreshDetailedInfoAboutFocusedItem();
				}
				break;

			case EEvent::DeserializedQueryHistoryCleared:
				if (m_queryHistoryManager.GetCurrentQueryHistory() == IQueryHistoryManager::EHistoryOrigin::Deserialized)
				{
					RefreshListOfHistoricQueries();
					RefreshDetailedInfoAboutCurrentHistoricQuery();
					RefreshDetailedInfoAboutFocusedItem();
				}
				break;

			case EEvent::CurrentQueryHistorySwitched:
				RefreshListOfHistoricQueries();
				RefreshDetailedInfoAboutCurrentHistoricQuery();
				RefreshDetailedInfoAboutFocusedItem();
				break;

			case EEvent::DifferentHistoricQuerySelected:
				RefreshListOfHistoricQueries();	// little hack to get the color of the currently selected historic query also updated
				RefreshDetailedInfoAboutCurrentHistoricQuery();
				RefreshDetailedInfoAboutFocusedItem();
				break;

			case EEvent::FocusedItemChanged:
				RefreshDetailedInfoAboutFocusedItem();
				break;
			}
		}

		void CQueryHistoryInGameGUI::AddHistoricQuery(const SHistoricQueryOverview& overview)
		{
			shared::CUqsString queryIdAsString;
			overview.queryID.ToString(queryIdAsString);

			string shortInfo;
			shortInfo.Format("#%s: '%s' / '%s' (%i / %i items) [%.2f ms]", queryIdAsString.c_str(), overview.querierName, overview.queryBlueprintName, (int)overview.numResultingItems, (int)overview.numGeneratedItems, overview.timeElapsedUntilResult.GetMilliSeconds());

			m_historicQueries.emplace_back(overview.color, overview.queryID, overview.parentQueryID, std::move(shortInfo));
		}

		void CQueryHistoryInGameGUI::AddTextLineToCurrentHistoricQuery(const ColorF& color, const char* fmt, ...)
		{
			string textLine;

			va_list args;
			va_start(args, fmt);
			textLine.FormatV(fmt, args);
			va_end(args);

			m_textLinesOfCurrentHistoricQuery.emplace_back(color, std::move(textLine));
		}

		void CQueryHistoryInGameGUI::AddTextLineToFocusedItem(const ColorF& color, const char* fmt, ...)
		{
			string textLine;

			va_list args;
			va_start(args, fmt);
			textLine.FormatV(fmt, args);
			va_end(args);

			m_textLinesOfFocusedItem.emplace_back(color, std::move(textLine));
		}

		bool CQueryHistoryInGameGUI::OnInputEvent(const SInputEvent& event)
		{
			// don't scroll through the history if it's not being drawn at all
			if (!SCvars::debugDraw)
				return false;

			bool bSwallowEvent = false;

			// - scroll through the history entries via PGUP/PGDOWN
			// - switch between the "live" and "deserialized" query history via END
			if (event.deviceType == eIDT_Keyboard && event.state == eIS_Pressed)
			{
				switch (event.keyId)
				{
				case eKI_PgUp:
					{
						const size_t historySize = m_historicQueries.size();

						if (historySize > 0)
						{
							// standing on "no entry" -> start with the latest one again (as if we were wrapping around)
							if (m_scrollIndexInHistoricQueries == s_noScrollIndex)
							{
								m_scrollIndexInHistoricQueries = historySize - 1;
							}
							else if (m_scrollIndexInHistoricQueries-- == 0)
							{
								// scrolling backwards beyond the first entry doesn't yet wrap to the latest entry again, but rather sticks to "no entry" so that we can have no debug drawing
								m_scrollIndexInHistoricQueries = s_noScrollIndex;
							}

							CQueryID queryIdOfHistoricQueryToSwitchTo = (m_scrollIndexInHistoricQueries == s_noScrollIndex) ? CQueryID::CreateInvalid() : m_historicQueries[m_scrollIndexInHistoricQueries].queryID;
							m_queryHistoryManager.MakeHistoricQueryCurrentForInWorldRendering(m_queryHistoryManager.GetCurrentQueryHistory(), queryIdOfHistoricQueryToSwitchTo);
						}

						bSwallowEvent = true;
					}
					break;

				case eKI_PgDn:
					{
						const size_t historySize = m_historicQueries.size();

						if (historySize > 0)
						{
							// standing on "no entry" -> wrap to first entry again
							if (m_scrollIndexInHistoricQueries == s_noScrollIndex)
							{
								m_scrollIndexInHistoricQueries = 0;
							}
							else if (++m_scrollIndexInHistoricQueries == historySize)
							{
								// scrolling forwards beyond the last entry doesn't yet wrap to the first entry again, but rather sticks to "no entry" so that we have no debug drawing
								m_scrollIndexInHistoricQueries = s_noScrollIndex;
							}

							CQueryID queryIdOfHistoricQueryToSwitchTo = (m_scrollIndexInHistoricQueries == s_noScrollIndex) ? CQueryID::CreateInvalid() : m_historicQueries[m_scrollIndexInHistoricQueries].queryID;
							m_queryHistoryManager.MakeHistoricQueryCurrentForInWorldRendering(m_queryHistoryManager.GetCurrentQueryHistory(), queryIdOfHistoricQueryToSwitchTo);
						}

						bSwallowEvent = true;
					}
					break;

				case eKI_End:
					{
						switch (m_queryHistoryManager.GetCurrentQueryHistory())
						{
						case IQueryHistoryManager::EHistoryOrigin::Live:
							m_queryHistoryManager.MakeQueryHistoryCurrent(IQueryHistoryManager::EHistoryOrigin::Deserialized);
							break;

						case IQueryHistoryManager::EHistoryOrigin::Deserialized:
							m_queryHistoryManager.MakeQueryHistoryCurrent(IQueryHistoryManager::EHistoryOrigin::Live);
							break;

						default:
							assert(0);
						}
						bSwallowEvent = true;
					}
					break;
				}
			}

			return bSwallowEvent;
		}

		void CQueryHistoryInGameGUI::Draw() const
		{
			const float xPos = (float)(gEnv->pRenderer->GetWidth() / 2 + 50);        // position found out by trial and error
			int row = 1;

			row = DrawQueryHistoryOverview(IQueryHistoryManager::EHistoryOrigin::Live, "live", xPos, row);
			row = DrawQueryHistoryOverview(IQueryHistoryManager::EHistoryOrigin::Deserialized, "deserialized", xPos, row);
			++row;
			row = DrawListOfHistoricQueries(xPos, row);
			++row;
			row = DrawDetailsAboutCurrentHistoricQuery(xPos, row);
			row = DrawDetailsAboutFocusedItem(xPos, row);
		}

		void CQueryHistoryInGameGUI::RefreshListOfHistoricQueries()
		{
			const IQueryHistoryManager::EHistoryOrigin currentHistory = m_queryHistoryManager.GetCurrentQueryHistory();

			m_historicQueries.clear();
			m_queryHistoryManager.EnumerateHistoricQueries(currentHistory, *this);
			FindScrollIndexInHistoricQueries();
		}

		void CQueryHistoryInGameGUI::RefreshDetailedInfoAboutCurrentHistoricQuery()
		{
			const IQueryHistoryManager::EHistoryOrigin currentHistory = m_queryHistoryManager.GetCurrentQueryHistory();
			const CQueryID queryIdToScrollTo = m_queryHistoryManager.GetCurrentHistoricQueryForInWorldRendering(currentHistory);

			m_textLinesOfCurrentHistoricQuery.clear();
			m_queryHistoryManager.GetDetailsOfHistoricQuery(currentHistory, queryIdToScrollTo, *this);
			FindScrollIndexInHistoricQueries();
		}

		void CQueryHistoryInGameGUI::RefreshDetailedInfoAboutFocusedItem()
		{
			m_textLinesOfFocusedItem.clear();
			m_queryHistoryManager.GetDetailsOfFocusedItem(*this);
		}

		void CQueryHistoryInGameGUI::FindScrollIndexInHistoricQueries()
		{
			const IQueryHistoryManager::EHistoryOrigin currentHistory = m_queryHistoryManager.GetCurrentQueryHistory();
			const CQueryID queryIdToScrollTo = m_queryHistoryManager.GetCurrentHistoricQueryForInWorldRendering(currentHistory);

			m_scrollIndexInHistoricQueries = s_noScrollIndex;

			for (size_t scrollIndex = 0; scrollIndex < m_historicQueries.size(); ++scrollIndex)
			{
				if (m_historicQueries[scrollIndex].queryID == queryIdToScrollTo)
				{
					m_scrollIndexInHistoricQueries = scrollIndex;
					break;
				}
			}
		}

		int CQueryHistoryInGameGUI::DrawQueryHistoryOverview(IQueryHistoryManager::EHistoryOrigin whichHistory, const char* descriptiveHistoryName, float xPos, int row) const
		{
			static const ColorF colorOfSelectedQueryHistory = Col_Cyan;
			static const ColorF colorOfNonSelectedQueryHistory = Col_White;

			static const char* markerOfSelectedQueryHistory = "*";
			static const char* markerOfNonSelectedQueryHistory = " ";

			ColorF color;
			const char* marker;
			const char* helpTextForKeyboardControl = "";

			if (m_queryHistoryManager.GetCurrentQueryHistory() == whichHistory)
			{
				color = colorOfSelectedQueryHistory;
				marker = markerOfSelectedQueryHistory;
				helpTextForKeyboardControl = " - press 'PGUP'/'PGDN'";
			}
			else
			{
				color = colorOfNonSelectedQueryHistory;
				marker = markerOfNonSelectedQueryHistory;
				helpTextForKeyboardControl = " - press 'END'";
			}

			CDrawUtil2d::DrawLabel(xPos, row, color, "=== %s %i UQS queries in %s history log (%i KB)%s ===",
				marker,
				(int)m_queryHistoryManager.GetHistoricQueriesCount(whichHistory),
				descriptiveHistoryName,
				(int)m_queryHistoryManager.GetRoughMemoryUsageOfQueryHistory(whichHistory) / 1024,
				helpTextForKeyboardControl);
			++row;
			return row;
		}

		int CQueryHistoryInGameGUI::DrawListOfHistoricQueries(float xPos, int row) const
		{
			//
			// - draw the list of historic queries as 2D text rows (for scrolling through them via PGUP/PGDOWN)
			// - this list is displayed for a short moment before automatically disappearing (pressing PGUP/PGDOWN brings it up again)
			//

			CDrawUtil2d::DrawLabel(xPos, row, Col_White, "=== Query history (%i queries logged - %i KB) ===",
				(int)m_historicQueries.size(), (int)m_queryHistoryManager.GetRoughMemoryUsageOfQueryHistory(m_queryHistoryManager.GetCurrentQueryHistory()) / 1024);
			++row;

			const int windowHeight = 100;

			const int historySize = m_historicQueries.size();

			// offset the rows in case there are more than would fit onto the screen
			const int maxRowsWeCanDraw = (int)((float)windowHeight / CDrawUtil2d::GetRowSize());

			int firstRowToDraw = (int)m_scrollIndexInHistoricQueries - maxRowsWeCanDraw / 2;

			if (firstRowToDraw > historySize - maxRowsWeCanDraw)
				firstRowToDraw = historySize - maxRowsWeCanDraw;

			if (firstRowToDraw < 0)
				firstRowToDraw = 0;

			int lastRowToDraw = firstRowToDraw + maxRowsWeCanDraw;
			if (lastRowToDraw >= historySize)
				lastRowToDraw = historySize - 1;

			for (int i = firstRowToDraw; i <= lastRowToDraw; ++i, ++row)
			{
				const bool bIsCurrentlySelectedHistoryEntry = ((size_t)i == m_scrollIndexInHistoricQueries);
				const SHistoricQueryShortInfo& queryInfo = m_historicQueries[i];
				const float indentSize = CDrawUtil2d::GetIndentSize() * (float)ComputeIndentationLevelOfHistoricQuery(queryInfo.queryID);
				const char* formatString = bIsCurrentlySelectedHistoryEntry ? "* %s" : "  %s";
				CDrawUtil2d::DrawLabel(xPos + indentSize, row, queryInfo.color, formatString, queryInfo.shortInfo.c_str());
			}

			return row;
		}

		int CQueryHistoryInGameGUI::DrawDetailsAboutCurrentHistoricQuery(float xPos, int row) const
		{
			for (const SHistoricQueryDetailedTextLine& info : m_textLinesOfCurrentHistoricQuery)
			{
				CDrawUtil2d::DrawLabel(xPos, row, info.color, "%s", info.textLine.c_str());
				++row;
			}
			return row;
		}

		int CQueryHistoryInGameGUI::DrawDetailsAboutFocusedItem(float xPos, int row) const
		{
			for (const SFocusedItemDetailedTextLine& info : m_textLinesOfFocusedItem)
			{
				CDrawUtil2d::DrawLabel(xPos, row, info.color, "%s", info.textLine.c_str());
				++row;
			}
			return row;
		}

		int CQueryHistoryInGameGUI::ComputeIndentationLevelOfHistoricQuery(CQueryID queryID) const
		{
			int indentation = 0;

			while (1)
			{
				// find the entry for the current query ID
				const SHistoricQueryShortInfo* pQueryShortInfo = nullptr;
				for (size_t i = 0, n = m_historicQueries.size(); i < n; ++i)
				{
					const SHistoricQueryShortInfo& shortInfo = m_historicQueries[i];
					if (shortInfo.queryID == queryID)
					{
						pQueryShortInfo = &shortInfo;
						break;
					}
				}
				assert(pQueryShortInfo);

				if (pQueryShortInfo->parentQueryID.IsValid())
				{
					// recurse up
					++indentation;
					queryID = pQueryShortInfo->parentQueryID;
				}
				else
				{
					// we've hit a top-level query
					break;
				}
			}

			return indentation;
		}

	}
}
