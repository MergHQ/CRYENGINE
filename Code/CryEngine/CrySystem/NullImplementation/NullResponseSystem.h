// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   A null implementation of the Dynamic Response System, that does nothing. Just a stub, if a real implementation was not provided or failed to initialize

   /************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CryDynamicResponseSystem/IDynamicResponseCondition.h>
#include <CrySerialization/ClassFactory.h>

namespace NullDRS
{
struct CVariableCollection final : public DRS::IVariableCollection
{
	CVariableCollection() : m_name("NullVariableCollection"), m_userString("Null") {}

	//////////////////////////////////////////////////////////
	// DRS::IVariableCollection implementation
	virtual DRS::IVariable*      CreateVariable(const CHashedString& name, int initialValue) override                                                                          { return nullptr; }
	virtual DRS::IVariable*      CreateVariable(const CHashedString& name, float initialValue) override                                                                        { return nullptr; }
	virtual DRS::IVariable*      CreateVariable(const CHashedString& name, bool initialValue) override                                                                         { return nullptr; }
	virtual DRS::IVariable*      CreateVariable(const CHashedString& name, const CHashedString& initialValue) override                                                         { return nullptr; }
	virtual bool                 SetVariableValue(const CHashedString& name, int newValue, bool createIfNotExisting = true, float resetTime = -1.0f) override                  { return true; }
	virtual bool                 SetVariableValue(const CHashedString& name, float newValue, bool createIfNotExisting = true, float resetTime = -1.0f) override                { return true; }
	virtual bool                 SetVariableValue(const CHashedString& name, bool newValue, bool createIfNotExisting = true, float resetTime = -1.0f) override                 { return true; }
	virtual bool                 SetVariableValue(const CHashedString& name, const CHashedString& newValue, bool createIfNotExisting = true, float resetTime = -1.0f) override { return true; }
	virtual DRS::IVariable*      GetVariable(const CHashedString& name) const override                                                                                         { return nullptr; }
	virtual const CHashedString& GetName() const override                                                                                                                      { return m_name; }
	virtual void                 SetUserString(const char* szUserString) override                                                                                              {}
	virtual const string&        GetUserString() override                                                                                                                      { return m_userString; }
	virtual void                 Serialize(Serialization::IArchive& ar) override                                                                                               {}
	//////////////////////////////////////////////////////////

private:
	const CHashedString m_name;
	const string        m_userString;
};

//////////////////////////////////////////////////////////

class CResponseActor final : public DRS::IResponseActor
{
public:
	CResponseActor() : m_name("NullResponseActor") {}

	//////////////////////////////////////////////////////////
	// DRS::IResponseActor implementation
	virtual const CHashedString&            GetName() const override           { return m_name; }
	virtual DRS::IVariableCollection*       GetLocalVariables() override       { return static_cast<DRS::IVariableCollection*>(&m_pLocalVariables); }
	virtual const DRS::IVariableCollection* GetLocalVariables() const override { return static_cast<const DRS::IVariableCollection*>(&m_pLocalVariables); }
	virtual EntityId                        GetLinkedEntityID() const override { return INVALID_ENTITYID; }
	virtual IEntity*                        GetLinkedEntity() const override   { return nullptr; }
	virtual DRS::SignalInstanceId           QueueSignal(const CHashedString& signalName, DRS::IVariableCollectionSharedPtr pSignalContext = nullptr, DRS::IResponseManager::IListener* pSignalListener = nullptr) override
	{ return DRS::s_InvalidSignalId; }
	//////////////////////////////////////////////////////////

private:
	const CHashedString m_name;
	CVariableCollection m_pLocalVariables;
};

//////////////////////////////////////////////////////////

typedef std::shared_ptr<CVariableCollection> NullVariableCollectionSharedPtr;

struct CSystem final : public DRS::IDynamicResponseSystem
{
public:
	CSystem() : m_nullVariableCollectionSharedPtr(new CVariableCollection) {}

	//////////////////////////////////////////////////////////
	// DRS::IDynamicResponseSystem implementation
	virtual bool                                     Init(const char* pFilesFolder) override                                                                                                      { return true; }
	virtual bool                                     ReInit() override                                                                                                                            { return true; }
	virtual void                                     Update() override                                                                                                                            {}
	virtual DRS::IVariableCollection*                CreateVariableCollection(const CHashedString& name) override                                                                                 { return &m_nullVariableCollection; }
	virtual void                                     ReleaseVariableCollection(DRS::IVariableCollection* pToBeReleased) override                                                                  {}
	virtual DRS::IVariableCollection*                GetCollection(const CHashedString& name) override                                                                                            { return &m_nullVariableCollection; }
	virtual DRS::IVariableCollection*                GetCollection(const CHashedString& name, DRS::IResponseInstance* pResponseInstance) override                                                 { return &m_nullVariableCollection; }
	virtual DRS::IVariableCollectionSharedPtr        CreateContextCollection() override                                                                                                           { return m_nullVariableCollectionSharedPtr; }
	virtual void                                     CancelSignalProcessing(const CHashedString& signalName, DRS::IResponseActor* pSender) override                                               {}
	virtual DRS::IResponseActor*                     CreateResponseActor(const CHashedString& pActorName, EntityId entityID = INVALID_ENTITYID) override                                          { return &m_nullActor; }
	virtual bool                                     ReleaseResponseActor(DRS::IResponseActor* pActorToFree) override                                                                             { return true; }
	virtual DRS::IResponseActor*                     GetResponseActor(const CHashedString& pActorName) override                                                                                   { return nullptr; }
	virtual void                                     Reset(uint32 resetHint = -1) override                                                                                                        {}
	virtual DRS::IDataImportHelper*                  GetCustomDataformatHelper() override                                                                                                         { return nullptr; }
	virtual void                                     Serialize(Serialization::IArchive& ar) override                                                                                              { static string dummy = "NullResponseSystem"; ar(dummy, "name", "Name"); }
	virtual DRS::ActionSerializationClassFactory&    GetActionSerializationFactory() override                                                                                                     { return Serialization::ClassFactory<DRS::IResponseAction>::the(); }
	virtual DRS::ConditionSerializationClassFactory& GetConditionSerializationFactory() override                                                                                                  { return Serialization::ClassFactory<DRS::IResponseCondition>::the(); }
	virtual DRS::ISpeakerManager*                    GetSpeakerManager() const override                                                                                                           { return nullptr; }
	virtual DRS::IDialogLineDatabase*                GetDialogLineDatabase() const override                                                                                                       { return nullptr; }
	virtual DRS::IResponseManager*                   GetResponseManager() const override                                                                                                          { return nullptr; }
	virtual void                                     GetCurrentState(DRS::VariableValuesList* pOutCollectionsList, uint32 saveHints = IDynamicResponseSystem::SaveHints_Variables) const override {}
	virtual void                                     SetCurrentState(const DRS::VariableValuesList& outCollectionsList) override                                                                  {}
	//////////////////////////////////////////////////////////

private:
	CVariableCollection             m_nullVariableCollection;
	NullVariableCollectionSharedPtr m_nullVariableCollectionSharedPtr;
	CResponseActor                  m_nullActor;
};
}  //namespace NullDRS
