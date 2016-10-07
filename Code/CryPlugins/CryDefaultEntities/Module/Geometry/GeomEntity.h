#pragma once

#include "Helpers/NativeEntityBase.h"

class CGeomEntity : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Model = 0,
		eProperty_Mass,

		eNumProperties,
	};

public:
	virtual ~CGeomEntity() {}

	// ISimpleExtension
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~ISimpleExtension

protected:
	void Reset();
};
