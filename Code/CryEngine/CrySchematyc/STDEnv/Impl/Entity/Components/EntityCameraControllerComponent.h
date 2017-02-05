// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Component.h>
#include <CrySerialization/Forward.h>

#include <IGameObject.h>  //IGameObjectView

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvRegistrar;

class CEntityOrbitCameraControllerComponent final : public CComponent, IGameObjectView
{
public:
	virtual ~CEntityOrbitCameraControllerComponent();

	// CComponent
	virtual void Run(ESimulationMode simulationMode) override;
	virtual bool Init() override;
	// ~CComponent

	static void ReflectType(CTypeDesc<CEntityOrbitCameraControllerComponent>& desc);
	static void Register(IEnvRegistrar& registrar);

	void        SetDistance(float distance, bool bRelative);
	void        SetYaw(float angle, bool bRelative);    //in degree
	void        SetPitch(float angle, bool bRelative);  //in degree
	void        SetRoll(float angle, bool bRelative);   //in degree
	void        SetFov(float fov, bool bRelative);
	void        SetActive(bool bEnable);
	//TODO: camera shake?

private:

	void         QueueUpdate();

	virtual void UpdateView(SViewParams& params) override;
	virtual void PostUpdateView(SViewParams& params) override {}

private:

	float        m_yaw = 0.0f;
	float        m_pitch = 45.0f;
	float        m_roll = 0.0f;
	float        m_distance = 10.0f;
	float        m_fov = 60.0f;
	bool         m_bActive = true;
	bool         m_bAlwaysUpdate = false;

	unsigned int m_viewId = -1;
	Ang3         m_rotation = Ang3(ZERO);
	bool         m_bNeedUpdate = true;
};

//#TODO:
//C3rdPersonCameraControllerComponent
//CFpsCameraControllerComponent

} // Schematyc
