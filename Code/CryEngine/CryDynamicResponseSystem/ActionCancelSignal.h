// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   the cancel signal processing action class

   /************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

namespace CryDRS
{
class CActionCancelSignal final : public DRS::IResponseAction
{
public:
	CActionCancelSignal() : m_bOnAllActors(true) {}
	CActionCancelSignal(const CHashedString& signalName) : m_signalName(signalName), m_bOnAllActors(true) {}
	virtual ~CActionCancelSignal() {}

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override { return m_signalName.GetText() + "'"; }
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override        { return "Cancel Signal"; }
	//////////////////////////////////////////////////////////

private:
	CHashedString m_signalName;
	bool          m_bOnAllActors;
};
}
