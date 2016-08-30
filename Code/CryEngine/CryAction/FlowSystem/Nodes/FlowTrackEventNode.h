// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/********************************************************************
   ---------------------------------------------------------------------
   File name:   FlowTrackEventNode.h
   Description: Dynamic node for Track Event logic
   ---------------------------------------------------------------------
   History:
   - 10:04:2008 : Created by Kevin Kirst

 *********************************************************************/

#pragma once

#include <CryFlowGraph/IFlowBaseNode.h>
#include <CryMovie/IMovieSystem.h>

class CFlowTrackEventNode : public CFlowBaseNode<eNCT_Instanced>, public ITrackEventListener
{
public:
	CFlowTrackEventNode(SActivationInfo*);
	CFlowTrackEventNode(CFlowTrackEventNode const& obj);
	CFlowTrackEventNode& operator=(CFlowTrackEventNode const& obj);
	virtual ~CFlowTrackEventNode();

	// IFlowNode
	virtual void         AddRef();
	virtual void         Release();
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo);
	virtual void         GetConfiguration(SFlowNodeConfig& config);
	virtual void         ProcessEvent(EFlowEvent event, SActivationInfo*);
	virtual bool         SerializeXML(SActivationInfo* pActInfo, const XmlNodeRef& root, bool reading);
	virtual void         Serialize(SActivationInfo*, TSerialize ser);

	// ~ITrackEventListener
	virtual void OnTrackEvent(IAnimSequence* pSequence, int reason, const char* event, void* pUserData);

	void         GetMemoryUsage(class ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_outputStrings);
	}
protected:
	// Add to sequence listener
	void AddListener(SActivationInfo* pActInfo);

private:
	typedef std::vector<string> StrArray;

	int                m_refs;
	int                m_nOutputs;
	StrArray           m_outputStrings;
	SOutputPortConfig* m_outputs;

	SActivationInfo    m_actInfo;
	IAnimSequence*     m_pSequence;
};
