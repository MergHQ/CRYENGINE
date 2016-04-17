// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.


#pragma once


class CPlayer : public CGameObjectExtensionHelper<CPlayer, ISimpleExtension>
{
public:
	CPlayer();
	virtual ~CPlayer();

	//ISimpleExtension
	virtual bool Init(IGameObject* pGameObject) override;
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void Release() override;
	//~ISimpleExtension

private:
	std::vector<string> m_extensions;
};
