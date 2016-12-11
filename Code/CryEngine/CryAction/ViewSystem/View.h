// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: View System interfaces.

   -------------------------------------------------------------------------
   History:
   - 24:9:2004 : Created by Filippo De Luca

*************************************************************************/
#ifndef __VIEW_H__
#define __VIEW_H__

#if _MSC_VER > 1000
	#pragma once
#endif

#include "IViewSystem.h"
#include <CrySystem/VR/IHMDDevice.h>
#include <CrySystem/VR/IHMDManager.h>

class CGameObject;
struct IGameObjectView;

class CView : public IView, public IEntityEventListener, public IHmdDevice::IAsyncCameraCallback
{
public:

	explicit CView(ISystem* const pSystem);
	virtual ~CView();

	//shaking
	struct SShake
	{
		bool      updating;
		bool      flip;
		bool      doFlip;
		bool      groundOnly;
		bool      permanent;
		bool      interrupted; // when forcefully stopped
		bool      isSmooth;

		int const ID;

		float     nextShake;
		float     timeDone;
		float     sustainDuration;
		float     fadeInDuration;
		float     fadeOutDuration;

		float     frequency;
		float     ratio;

		float     randomness;

		Quat      startShake;
		Quat      startShakeSpeed;
		Vec3      startShakeVector;
		Vec3      startShakeVectorSpeed;

		Quat      goalShake;
		Quat      goalShakeSpeed;
		Vec3      goalShakeVector;
		Vec3      goalShakeVectorSpeed;

		Ang3      amount;
		Vec3      amountVector;

		Quat      shakeQuat;
		Vec3      shakeVector;

		explicit SShake(int const shakeID)
			: updating(false)
			, flip(false)
			, doFlip(false)
			, groundOnly(false)
			, permanent(false)
			, interrupted(false)
			, isSmooth(false)
			, ID(shakeID)
			, nextShake(0.0f)
			, timeDone(0.0f)
			, sustainDuration(0.0f)
			, fadeInDuration(0.0f)
			, fadeOutDuration(0.0f)
			, frequency(0.0f)
			, ratio(0.0f)
			, randomness(0.5f)
			, startShake(IDENTITY)
			, startShakeSpeed(IDENTITY)
			, startShakeVector(ZERO)
			, startShakeVectorSpeed(ZERO)
			, goalShake(IDENTITY)
			, goalShakeSpeed(IDENTITY)
			, goalShakeVector(ZERO)
			, goalShakeVectorSpeed(ZERO)
			, amount(ZERO)
			, amountVector(ZERO)
			, shakeQuat(IDENTITY)
			, shakeVector(ZERO)
		{}

		void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/ }
	};

	// IView
	virtual void               Release();
	virtual void               Update(float frameTime, bool isActive);
	virtual void               ProcessShaking(float frameTime);
	virtual void               ProcessShake(SShake* pShake, float frameTime);
	virtual void               ResetShaking();
	virtual void               ResetBlending() { m_viewParams.ResetBlending(); }
	//FIXME: keep CGameObject *  or use IGameObject *?
	virtual void               LinkTo(IGameObject* follow);
	virtual void               LinkTo(IEntity* follow, IGameObjectView* callback);
	virtual EntityId           GetLinkedId()                         { return m_linkedTo; };
	virtual void               SetCurrentParams(SViewParams& params) { m_viewParams = params; };
	virtual const SViewParams* GetCurrentParams()                    { return &m_viewParams; }
	virtual void               SetViewShake(Ang3 shakeAngle, Vec3 shakeShift, float duration, float frequency, float randomness, int shakeID, bool bFlipVec = true, bool bUpdateOnly = false, bool bGroundOnly = false);
	virtual void               SetViewShakeEx(const SShakeParams& params);
	virtual void               StopShake(int shakeID);
	virtual void               SetFrameAdditiveCameraAngles(const Ang3& addFrameAngles);
	virtual void               SetScale(const float scale);
	virtual void               SetZoomedScale(const float scale);
	virtual void               SetActive(bool const bActive);
	// ~IView

	// IEntityEventListener
	virtual void OnEntityEvent(IEntity* pEntity, SEntityEvent& event);
	// ~IEntityEventListener

	void     Serialize(TSerialize ser);
	void     PostSerialize();
	CCamera& GetCamera() { return m_camera; };
	void     UpdateAudioListener(Matrix34 const& rMatrix);

	void     GetMemoryUsage(ICrySizer* s) const;

protected:

	CGameObject* GetLinkedGameObject();
	IEntity*     GetLinkedEntity();
	void         ProcessShakeNormal(SShake* pShake, float frameTime);
	void         ProcessShakeNormal_FinalDamping(SShake* pShake, float frameTime);
	void         ProcessShakeNormal_CalcRatio(SShake* pShake, float frameTime, float endSustain);
	void         ProcessShakeNormal_DoShaking(SShake* pShake, float frameTime);

	void         ProcessShakeSmooth(SShake* pShake, float frameTime);
	void         ProcessShakeSmooth_DoShaking(SShake* pShake, float frameTime);

	void         ApplyFrameAdditiveAngles(Quat& cameraOrientation);

	const float  GetScale();

	// IAsyncCameraCallback
	virtual bool OnAsyncCameraCallback(const HmdTrackingState& state, IHmdDevice::AsyncCameraContext& context);
	// ~IAsyncCameraCallback

private:

	void GetRandomQuat(Quat& quat, SShake* pShake);
	void GetRandomVector(Vec3& vec3, SShake* pShake);
	void CubeInterpolateQuat(float t, SShake* pShake);
	void CubeInterpolateVector(float t, SShake* pShake);
	void CreateAudioListener();

protected:

	EntityId            m_linkedTo;
	IGameObjectView*	m_linkedEntityCallback;

	SViewParams         m_viewParams;
	CCamera             m_camera;

	ISystem* const      m_pSystem;

	std::vector<SShake> m_shakes;

	IEntity*            m_pAudioListener;
	Ang3                m_frameAdditiveAngles; // Used mainly for cinematics, where the game can slightly override camera orientation

	float               m_scale;
	float               m_zoomedScale;
};

#endif //__VIEW_H__
