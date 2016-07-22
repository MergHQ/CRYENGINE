// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once


class CViewExtension : public CGameObjectExtensionHelper<CViewExtension, ISimpleExtension>, IGameObjectView
{
public:
	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void Release() override;
	// ~ISimpleExtension

	// IGameObjectView
	virtual void UpdateView(SViewParams& params) override;
	virtual void PostUpdateView(SViewParams& viewParams) override {}
	// ~IGameObjectView

	CViewExtension();
	virtual ~CViewExtension();

private:
	void CreateView();

	unsigned int m_viewId;
	float m_camFOV;
};
