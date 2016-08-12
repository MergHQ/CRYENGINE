#pragma once

#include "ISimpleExtension.h"

struct SNativeEntityPropertyInfo;

////////////////////////////////////////////////////////
// Base entity for native C++ entities that require the use of Editor properties
////////////////////////////////////////////////////////
class CNativeEntityBase
	: public ISimpleExtension
{
public:
	virtual ~CNativeEntityBase() {}

	// Retrieves the current string value of a property
	const char *GetPropertyValue(int index) const;
	// Sets the string value of a property
	void SetPropertyValue(int index, const char *value);

	// Gets the converted float value of a property
	float GetPropertyFloat(int index) const;
	// Gets the converted integer value of a property
	int GetPropertyInt(int index) const;
	// Gets the converted ColorF value of a property
	ColorF GetPropertyColor(int index) const;
	// Gets the converted boolean value of a property
	bool GetPropertyBool(int index) const;
	// Gets the converted Vec3 value of a property
	Vec3 GetPropertyVec3(int index) const;

	// Helper to set a property value as float
	void SetPropertyFloat(int index, float value);
	// Helper to set a property value as an integer
	void SetPropertyInt(int index, int value);
	// Helper to set a property value as a boolean
	void SetPropertyBool(int index, bool value);
	// Helper to set a property value as a vector
	void SetPropertyVec3(int index, Vec3 value);

	void ActivateFlowNodeOutput(const int portIndex, const TFlowInputData &inputData)
	{
		SEntityEvent evnt;
		evnt.event     = ENTITY_EVENT_ACTIVATE_FLOW_NODE_OUTPUT;
		evnt.nParam[0] = portIndex;

		evnt.nParam[1] = (INT_PTR)&inputData;
		GetEntity()->SendEvent(evnt);
	}
};

