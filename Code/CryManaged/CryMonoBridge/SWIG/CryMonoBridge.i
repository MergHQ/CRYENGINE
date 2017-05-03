%include "CryEngine.swig"

%import "CryCommon.i"
%import "CryAISystem.i"
%import "CryEntitySystem.i"

%{
#include <CryMono/IMonoRuntime.h>
#include <CryAISystem/BehaviorTree/IBehaviorTree.h>
%}
%feature("director") IMonoListener;
%feature("director") ICryEngineBasePlugin;
%feature("director") IManagedNodeCreator;
%include "../../../../CryEngine/CryCommon/CryMono/IMonoRuntime.h"