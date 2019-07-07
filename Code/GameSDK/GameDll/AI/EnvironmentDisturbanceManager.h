// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef EnvironmentDisturbanceManager_h
#define EnvironmentDisturbanceManager_h

#include <CryAISystem/IVisionMap.h>
#include <CryAISystem/ISignal.h>

namespace GameAI
{
	class ObservableEvent
	{
	public:
		ObservableEvent()
			: m_expirationTime(0.0f)
			, m_position(ZERO)
			, m_pSignalDescription(nullptr)
		{};

		void Initialize( const Vec3& position, float duration, uint8 faction, const AISignals::ISignalDescription& signalDescription);
		void Release();

		bool IsExpired( CTimeValue currentTime );
		bool HasBeenObservedBy( EntityId entityId );
		void SetObservedBy( EntityId entityId );
		VisionID GetVisionId() { return m_visionId; }
		const Vec3& GetPosition() { return m_position; }
		const AISignals::ISignalDescription& GetSignalDescription() 
		{ 
			if (!m_pSignalDescription)
			{
				return gEnv->pAISystem->GetSignalManager()->GetBuiltInSignalDescriptions().GetNone();
			}
			return *m_pSignalDescription; 
		}

	private:
		CTimeValue                           m_expirationTime;
		VisionID                             m_visionId;
		Vec3                                 m_position;
		const AISignals::ISignalDescription* m_pSignalDescription;

		std::vector<EntityId>                m_obeservedBy;
	};

	class EnvironmentDisturbanceManager
	{
	public:
		EnvironmentDisturbanceManager()
		{
		};

		~EnvironmentDisturbanceManager();;

		void Reset();
		void Update();

		void AddObservableEvent( const Vec3& position, float duration, const AISignals::ISignalDescription& signalDescription, EntityId sourceEntityId = 0 );

	private:
		void RemoveExpiredEvents();

		typedef std::vector<ObservableEvent> ObservableEvents;
		ObservableEvents m_observableEvents;
	};
}

#endif //EnvironmentDisturbanceManager_h
