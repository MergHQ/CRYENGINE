// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScriptElement.h>

// Forward declare classes.
class QWidget;
class QPoint;

namespace Schematyc
{
// Forward declare interfaces.
struct IScriptActionInstance;
struct IScriptClass;
struct IScriptComponentInstance;
struct IScriptEnum;
struct IScriptFunction;
struct IScriptImport;
struct IScriptInterface;
struct IScriptInterfaceFunction;
struct IScriptInterfaceImpl;
struct IScriptInterfaceTask;
struct IScriptModule;
struct IScriptSignal;
struct IScriptSignalReceiver;
struct IScriptState;
struct IScriptStateMachine;
struct IScriptStruct;
struct IScriptTimer;
struct IScriptVariable;

namespace ScriptBrowserUtils
{
bool                      CanAddScriptElement(EScriptElementType elementType, IScriptElement* pScope);
bool                      CanRemoveScriptElement(const IScriptElement& element);
bool                      CanRenameScriptElement(const IScriptElement& element);
const char*               GetScriptElementTypeName(EScriptElementType scriptElementType);
const char*               GetScriptElementFilterName(EScriptElementType scriptElementType);
const char*               GetScriptElementIcon(const IScriptElement& scriptElement);
void                      MakeScriptElementNameUnique(CStackString& name, IScriptElement* pScope);
void                      FindReferences(const IScriptElement& element);

IScriptModule*            AddScriptModule(IScriptElement* pScope);
IScriptImport*            AddScriptImport(IScriptElement* pScope);
IScriptEnum*              AddScriptEnum(IScriptElement* pScope);
IScriptStruct*            AddScriptStruct(IScriptElement* pScope);
IScriptSignal*            AddScriptSignal(IScriptElement* pScope);
IScriptFunction*          AddScriptFunction(IScriptElement* pScope);
IScriptInterface*         AddScriptInterface(IScriptElement* pScope);
IScriptInterfaceFunction* AddScriptInterfaceFunction(IScriptElement* pScope);
IScriptInterfaceTask*     AddScriptInterfaceTask(IScriptElement* pScope);
IScriptClass*             AddScriptClass(IScriptElement* pScope);
IScriptStateMachine*      AddScriptStateMachine(IScriptElement* pScope);
IScriptState*             AddScriptState(IScriptElement* pScope);
IScriptVariable*          AddScriptVariable(IScriptElement* pScope);
IScriptTimer*             AddScriptTimer(IScriptElement* pScope);
IScriptSignalReceiver*    AddScriptSignalReceiver(IScriptElement* pScope);
IScriptInterfaceImpl*     AddScriptInterfaceImpl(IScriptElement* pScope);
IScriptComponentInstance* AddScriptComponentInstance(IScriptElement* pScope, const QPoint* pPostion = nullptr);
IScriptActionInstance*    AddScriptActionInstance(IScriptElement* pScope);

bool                      RenameScriptElement(IScriptElement& element, const char* szName);
void                      RemoveScriptElement(const IScriptElement& element);
} // ScriptBrowserUtils
} // Schematyc
