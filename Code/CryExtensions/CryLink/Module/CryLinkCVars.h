// Copyright 2001-2019 Crytek GmbH. All rights reserved.

#pragma once

namespace CryLinkService 
{
	class CVars
	{
	public:
		static CVars& GetInstance() { return m_instance; }

		void Register();
		void Unregister();

	private: 
		CVars();
		~CVars();

	public:
		ICVar* cryLinkAllowedClient;
		ICVar* cryLinkServicePassword;
		int32 cryLinkEditorServicePort;
		int32 cryLinkGameServicePort;

	private: 
		static CVars m_instance;

		bool m_bRegistered;
	};
}