#pragma once

#include "Helpers/NativeEntityBase.h"

class CCommentEntity : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Text = 0,
		eProperty_MaxDistance,

		eProperty_FontSize,

		eNumProperties,
	};

public:
	virtual ~CCommentEntity() {}

	// ISimpleExtension
	virtual void PostInit(IGameObject* pGameObject) override;

	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override;
	// ~ISimpleExtension

protected:
	ICVar* m_pEnableCommentsVar;
};
