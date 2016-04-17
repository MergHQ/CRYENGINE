// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef ITransformationPinning_h
#define ITransformationPinning_h

#include <CryAnimation/ICryAnimation.h>

struct ITransformationPinning : public IAnimationPoseModifier
{
	CRYINTERFACE_DECLARE(ITransformationPinning, 0xcc34ddea972e47db, 0x93f9cdcb98c28c8f);

	virtual void SetBlendWeight(float factor)		= 0;
	virtual void SetJoint(uint32 jntID)				= 0;
	virtual void SetSource(ICharacterInstance* source)	= 0;
};

DECLARE_SHARED_POINTERS(ITransformationPinning);


#endif // ITransformationPinning_h
