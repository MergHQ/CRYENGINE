#pragma once

#include "Entities/Helpers/ISimpleExtension.h"

#include <ICryMannequin.h>

class CPlayer;

////////////////////////////////////////////////////////
// Player extension to manage animation handling and playback via Mannequin
////////////////////////////////////////////////////////
class CPlayerAnimations
	: public CGameObjectExtensionHelper<CPlayerAnimations, ISimpleExtension>
{
public:
	CPlayerAnimations();
	virtual ~CPlayerAnimations();

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;

	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override;

	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~ISimpleExtension

	void OnPlayerModelChanged();

protected:
	void ActivateMannequinContext(const char *contextName, ICharacterInstance &character, const SControllerDef &controllerDefinition, const IAnimationDatabase &animationDatabase);

protected:
	CPlayer *m_pPlayer;

	IActionController *m_pActionController;
	SAnimationContext *m_pAnimationContext;

	_smart_ptr<IAction> m_pIdleFragment;

	TagID m_rotateTagId;
	TagID m_walkTagId;
};
