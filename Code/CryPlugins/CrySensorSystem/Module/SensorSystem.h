// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Interface/ISensorSystem.h"

class CSensorTagLibrary;
class CSensorTagLibrary;

class CSensorSystem : public ISensorSystem, public ISystemEventListener
{
public:

	CSensorSystem();
	~CSensorSystem();

	// ISensorMapManager
	virtual ISensorTagLibrary& GetTagLibrary() override;
	virtual ISensorMap&        GetMap() override;
	// ~ISensorMapManager

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
	// ~ISystemEventListener

	void                  Update();

	static CSensorSystem& GetInstance();

private:

	void        RegisterSchematycEnvPackage(Schematyc::IEnvRegistrar& registrar);

	static void SetOctreeDepthCommand(IConsoleCmdArgs* pArgs);
	static void SetOctreeBoundsCommand(IConsoleCmdArgs* pArgs);

private:

	std::unique_ptr<ISensorTagLibrary> m_pTagLibrary;
	std::unique_ptr<ISensorMap>        m_pMap;

	static CSensorSystem*              ms_pInstance;
};
