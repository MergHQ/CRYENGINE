%include "CryEngine.swig"

%import "CryCommon.i"

%import "CryAction.i"

%{
#include <CryGame/IGameVolumes.h>
#include <CryGame/IGameStatistics.h>
#include <CryGame/IGameTokens.h>
#include <CryEntitySystem/IEntityComponent.h>
%}

%import "../../../../CryEngine/CryCommon/CryEntitySystem/IEntityComponent.h"

%include "../../../../CryEngine/CryCommon/CryGame/IGameRef.h"
%ignore SNodeLocator::SNodeLocator;
%include "../../../../CryEngine/CryCommon/CryGame/IGameStatistics.h"
%include "../../../../CryEngine/CryCommon/CryGame/IGameStartup.h"
%include "../../../../CryEngine/CryCommon/CryGame/IGameVolumes.h"
%feature("director") IGameFrameworkListener;
%feature("director") IGameWarningsListener;
%feature("director") IBreakEventListener;
%include "../../../../CryEngine/CryCommon/CryGame/IGameFramework.h"
%ignore IGame::FullSerialize;
%include "../../../../CryEngine/CryCommon/CryGame/IGame.h"
%feature("director") IGameTokenEventListener;
%include "../../../../CryEngine/CryCommon/CryGame/IGameTokens.h"