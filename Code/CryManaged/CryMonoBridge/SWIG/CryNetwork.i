%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryNetwork/INetwork.h>
#include <CryNetwork/ICryOnlineStorage.h>
#include <CryLobby/ICryReward.h>
#include <CryLobby/ICrySignIn.h>
#include <CryLobby/ICryStats.h>
#include <CryLobby/ICryTCPService.h>
#include <CryNetwork/ISimpleHttpServer.h>
%}
%csconstvalue("0xFFFFFFFFu") eEA_All;
%ignore ISerializableInfo;
%ignore IBreakDescriptionInfo::SerialiseSimpleBreakage;
%ignore INetSender::ser;
%ignore IGameContext::SynchObject;
%ignore IGameContext::HandleRMI;
%ignore IGameContext::ReceiveSimpleBreakage;
%ignore SRMIBenchmarkParams::SerializeWith;
%ignore INetMessage::WritePayload;
%ignore IRMICppLogger::SerializeParams;
%ignore IRMIMessageBody::SerializeWith;
%ignore CreateNetwork;
%ignore operator<(const SMessageTag& left, const SMessageTag& right);
%ignore operator==(const SDefenseFileInfo& lhs, const SDefenseFileInfo& rhs);
%include "../../../../CryEngine/CryCommon/CryNetwork/INetwork.h"
%ignore g_szMCMString;
%ignore g_szColliderModeString;
%ignore g_szColliderModeLayerString;
%include "../../../../CryEngine/CryCommon/CryNetwork/ICryOnlineStorage.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryReward.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICrySignIn.h"
%ignore ICrysis3AuthenticationHandler::SPersona::SPersona;
%include "../../../../CryEngine/CryCommon/CryLobby/ICryStats.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryTCPService.h"
%include "../../../../CryEngine/CryCommon/CryNetwork/ISimpleHttpServer.h"