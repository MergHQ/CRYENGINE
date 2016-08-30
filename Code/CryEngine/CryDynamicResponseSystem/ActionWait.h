// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   This action will simply wait for the specified time, and therefore delay
   the execution of all following further actions.

   /************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

namespace CryDRS
{
class CActionWait final : public DRS::IResponseAction
{
public:
	CActionWait() : m_timeToWait(0.0f) {}
	CActionWait(float timeToWait) : m_timeToWait(timeToWait) {}
	virtual ~CActionWait() {}

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Do Nothing"; }
	//////////////////////////////////////////////////////////

	float GetTimeToWait() const { return m_timeToWait; }

private:
	float m_timeToWait;    //1/100 seconds
};

//////////////////////////////////////////////////////////////////////////

class CActionWaitInstance final : public DRS::IResponseActionInstance
{
public:
	CActionWaitInstance(float timeToWait);
	virtual ~CActionWaitInstance() {}

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual eCurrentState Update() override;
	virtual void          Cancel() override { m_finishTime = 0.0f; }
	//////////////////////////////////////////////////////////

private:
	float m_finishTime;
};

} // namespace CryDRS
