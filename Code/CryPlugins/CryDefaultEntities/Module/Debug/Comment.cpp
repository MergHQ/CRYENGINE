#include "StdAfx.h"
#include "Comment.h"

#include <CryRenderer/IRenderAuxGeom.h>

class CCommentEntityRegistrator
	: public IEntityRegistrator
{
	virtual void Register() override
	{
		if (gEnv->pEntitySystem->GetClassRegistry()->FindClass("Comment") != nullptr)
		{
			// Skip registration of default engine class if the game has overridden it
			CryLog("Skipping registration of default engine entity class Comment, overridden by game");
			return;
		}

		RegisterEntityWithDefaultComponent<CCommentEntity>("Comment", "Debug");
		auto* pEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("Comment");
		auto* pPropertyHandler = pEntityClass->GetPropertyHandler();

		RegisterEntityProperty<string>(pPropertyHandler, "Text", "", "", "Text to draw to screen");
		RegisterEntityProperty<float>(pPropertyHandler, "ViewDistance", "fMaxDist", "", "Maximum entity distance from player", 0.1f, 255);

		RegisterEntityProperty<float>(pPropertyHandler, "FontSize", "fSize", "1.2", "Size of the text", 0, 100);
	}
};

CCommentEntityRegistrator g_commentEntityRegistrator;

void CCommentEntity::PostInit(IGameObject* pGameObject)
{
	GetEntity()->SetUpdatePolicy(ENTITY_UPDATE_VISIBLE);
	pGameObject->EnableUpdateSlot(this, 0);

	m_pEnableCommentsVar = gEnv->pConsole->GetCVar("cl_comment");
}

void CCommentEntity::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	if (m_pEnableCommentsVar->GetIVal() == 0)
		return;

	const float maxDistance = GetPropertyFloat(eProperty_MaxDistance);
	float cameraDistance = (GetEntity()->GetWorldPos() - gEnv->pSystem->GetViewCamera().GetPosition()).GetLength();

	float alpha;

	if (maxDistance >= 255.f)
	{
		alpha = 1.f;
	}
	else if(cameraDistance < maxDistance)
	{
		alpha = cameraDistance / maxDistance;
		alpha = 1.f - alpha * alpha;
	}
	else
	{
		alpha = 0.f;
	}

	if (alpha == 0.f)
		return;

	float color[4] = { 1, 1, 1, alpha };

	IRenderAuxText::DrawLabelEx(GetEntity()->GetWorldPos(), GetPropertyFloat(eProperty_FontSize), color, true, true, GetPropertyValue(eProperty_Text));
}
