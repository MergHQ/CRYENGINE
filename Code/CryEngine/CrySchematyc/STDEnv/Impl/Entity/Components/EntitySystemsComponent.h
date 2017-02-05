// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <Schematyc/Types/MathTypes.h>
#include <Schematyc/Types/ResourceTypes.h>
#include "CryGame/IGameTokens.h"

namespace Schematyc
{
	// Forward declare interfaces.
	struct IEnvRegistrar;

	class CEntitySystemsComponent final : public CComponent, public IGameTokenEventListener
	{
	private:

		struct SGametokenChangedSignal
		{
			SGametokenChangedSignal() {}
			SGametokenChangedSignal(CSharedString tokenname, CSharedString value) : m_tokenname(tokenname), m_value(value) {}

			static void ReflectType(CTypeDesc<SGametokenChangedSignal>& desc);

			CSharedString m_tokenname;
			CSharedString m_value;
		};

	public:

		CEntitySystemsComponent();

		// CComponent
		virtual bool Init() override;
		virtual void Run(ESimulationMode simulationMode) override;
		virtual void Shutdown() override;
		// ~CComponent

		// IGameTokenEventListener
		virtual void OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken) override;
		virtual void GetMemoryUsage(class ICrySizer* pSizer) const override {}
		// ~IGameTokenEventListener

		void SetGametoken(CSharedString tokenName, CSharedString valueToSet, bool bCreateIfNotExisting);

		static void ReflectType(CTypeDesc<CEntitySystemsComponent>& desc);
		static void Register(IEnvRegistrar& registrar);

	private:

		//filter for which gametoken(s) to listen to
	};
} // Schematyc
