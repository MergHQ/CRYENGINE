%include "CryEngine.swig"

%import "CryCommon.i"

%{
#include <CryLobby/ICryLobby.h>
#include <CryLobby/ICryLobbyUI.h>
#include <CryLobby/ICryMatchMaking.h>
#include <CryLobby/ICryVoice.h>
#include <CryLobby/CryLobbyPacket.h>
#include <CryLobby/ICryFriends.h>
#include <CryLobby/ICryFriendsManagement.h>
%}

%include "../../../../CryEngine/CryCommon/CryLobby/ICryLobby.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryLobbyUI.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryMatchMaking.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryVoice.h"
%include "../../../../CryEngine/CryCommon/CryLobby/CryLobbyPacket.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryFriends.h"
%include "../../../../CryEngine/CryCommon/CryLobby/ICryFriendsManagement.h"