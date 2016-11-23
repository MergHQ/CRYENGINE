#pragma once

#include "Helpers/DesignerEntityComponent.h"

#include <CrySerialization/STL.h>

class CCommentEntity final 
	: public CDesignerEntityComponent
	, public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CCommentEntity, "Comment", 0x979E412F68B94866, 0xB61EEB87CFA757CF);

public:
	virtual ~CCommentEntity() {}

	// ISimpleExtension
	virtual void Initialize() override;

	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual uint64 GetEventMask() const override { return BIT64(ENTITY_EVENT_UPDATE); }

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }
	// ~ISimpleExtension

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "Comment Properties"; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override
	{
		archive(m_text, "Text", "Text");
		archive.doc("Text to draw to screen");

		archive(m_maxDistance, "ViewDistance", "ViewDistance");
		archive.doc("Maximum entity distance from player");

		archive(m_fontSize, "FontSize", "FontSize");
		archive.doc("Maximum entity distance from player");

		if (archive.isInput())
		{
			OnResetState();
		}
	}
	// ~IEntityPropertyGroup

protected:
	ICVar* m_pEnableCommentsVar;

	float m_maxDistance = 100.f;
	float m_fontSize = 1.2f;

	string m_text;
};
