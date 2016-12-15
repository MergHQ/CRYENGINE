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
	struct SProperties
	{
		void Serialize(Serialization::IArchive& archive);

		float yaw = 0.0f;
		float pitch = 45.0f;
		float roll = 0.0f;
		float fov = 60.0f;
		float distance = 10.0f;
		bool  bActive = true;
		bool  bAlwaysUpdate = false;
	};

	virtual ~CEntityOrbitCameraControllerComponent();

	// CComponent
	virtual void Run(ESimulationMode simulationMode) override;
	virtual bool Init() override;
	// ~CComponent

	static SGUID ReflectSchematycType(CTypeInfo<CEntityOrbitCameraControllerComponent>& typeInfo);
	static void  Register(IEnvRegistrar& registrar);

	void         SetDistance(float distance, bool bRelative);
	void         SetYaw(float angle, bool bRelative);   //in degree
	void         SetPitch(float angle, bool bRelative); //in degree
	void         SetRoll(float angle, bool bRelative);  //in degree
	void         SetFov(float fov, bool bRelative);
	void         SetActive(bool bEnable);
	//TODO: camera shake?

private:

	void         QueueUpdate();

	virtual void UpdateView(SViewParams& params) override;
	virtual void PostUpdateView(SViewParams& params) override {}

private:

	unsigned int m_viewId = -1;
	float        m_distance;
	float        m_fov;
	Ang3         m_rotation;
	bool         m_bAlwaysUpdate = false;
	bool         m_bNeedUpdate = true;
};

//#TODO:
//C3rdPersonCameraControllerComponent
//CFpsCameraControllerComponent

} // Schematyc
