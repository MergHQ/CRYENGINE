#include "StdAfx.h"
#include "DesignerEntityComponent.h"

void CDesignerEntityComponent::Initialize()
{
	OnResetState();
}

void CDesignerEntityComponent::ProcessEvent(SEntityEvent& event)
{
	switch (event.event)
	{
			// Physicalize on level start for Launcher
		case ENTITY_EVENT_START_LEVEL:
			// Editor specific, physicalize on reset, property change or transform change
		case ENTITY_EVENT_RESET:
		case ENTITY_EVENT_EDITOR_PROPERTY_CHANGED:
		case ENTITY_EVENT_XFORM_FINISHED_EDITOR:
			OnResetState();
			break;
	}
}