#pragma once

////////////////////////////////////////////////////////
// Base entity for native C++ entities that require the use of Editor properties
////////////////////////////////////////////////////////
class CDesignerEntityComponent
	: public IEntityComponent
{
public:
	virtual ~CDesignerEntityComponent() {}

	// IEntityComponent
	virtual void Initialize() override;

	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual uint64 GetEventMask() const override { return BIT64(ENTITY_EVENT_START_LEVEL) | BIT64(ENTITY_EVENT_RESET) | BIT64(ENTITY_EVENT_EDITOR_PROPERTY_CHANGED) | BIT64(ENTITY_EVENT_XFORM_FINISHED_EDITOR); }
	// ~IEntityComponent

	void ActivateFlowNodeOutput(const int portIndex, const TFlowInputData& inputData)
	{
		SEntityEvent evnt;
		evnt.event = ENTITY_EVENT_ACTIVATE_FLOW_NODE_OUTPUT;
		evnt.nParam[0] = portIndex;

		evnt.nParam[1] = (INT_PTR)&inputData;
		GetEntity()->SendEvent(evnt);
	}

	// Helper for loading geometry or characters
	void LoadMesh(int slot, const char *path)
	{
		if (IsCharacterFile(path))
		{
			GetEntity()->LoadCharacter(slot, path);
		}
		else
		{
			GetEntity()->LoadGeometry(slot, path);
		}
	}

	// Called when the entity needs to have its model, physics etc reset.
	// Usually on level load or when properties are changed.
	virtual void OnResetState() {}
};
