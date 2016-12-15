// XXX - ATG

//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "StdAfx.h"

#if CRY_PLATFORM_DURANGO && USE_DURANGOLIVE

	#using <windows.winmd>
	#using <platform.winmd>
	#using <Microsoft.Xbox.Services.winmd>
	#using <Microsoft.Xbox.GameChat.winmd>

	#include "ChatIntegrationLayer.h"

	#include "../CryDurangoLiveLobby.h"
	#include <robuffer.h>
using namespace Microsoft::WRL;
using namespace Windows::Storage::Streams;

void LogComment(Platform::String^ strText)
{
	NetLog("[chat] %ls", strText->Data());
}

void LogCommentFormat(LPCWSTR strMsg, ...)
{
	NetLog("[chat] %ls", strMsg);
}

void LogCommentWithError(Platform::String^ strText, HRESULT hr)
{
	NetLog("[chat] %ls, Error: %x", strText->Data(), hr);
}

ChatIntegrationLayer::ChatIntegrationLayer() :
	m_ackTimer(0.0f),
	m_pMatchMaking(nullptr)
{
	Initialize(true, nullptr);
}

ChatIntegrationLayer::~ChatIntegrationLayer()
{
	Shutdown();
}

inline uint ProcessorToAffinityMask(DWORD cpuCore)
{
	return ((DWORD_PTR)(0x1) << cpuCore);
}

ECryLobbyError ChatIntegrationLayer::Initialize(bool jitterBufferEnabled, CCryDurangoLiveMatchMaking* pMatchMaking, DWORD cpuCore)
{
	if (pMatchMaking == nullptr)
	{
		return eCLE_InvalidParam;
	}
	else
	{
		m_pMatchMaking = pMatchMaking;
	}

	ECryLobbyError error = eCLE_Success;

	PREFAST_SUPPRESS_WARNING(6387) // incorrect throwing of 6387 (no param to be given)
	m_chatManager = ref new Microsoft::Xbox::GameChat::ChatManager();

	if (m_chatManager == nullptr)
	{
		error = eCLE_OutOfMemory;
	}
	else
	{
		// Optionally, change the default settings below as desired by commenting out and editing any of the following lines
		// Otherwise these defaults are used.
		//
		// m_chatManager = ref new Microsoft::Xbox::GameChat::ChatManager( ChatSessionPeriod::ChatPeriodOf40Milliseconds );
		// m_chatManager->ChatSettings->AudioThreadPeriodInMilliseconds = 40;
		// m_chatManager->ChatSettings->AudioThreadAffinityMask = XAUDIO2_DEFAULT_PROCESSOR; // <- this means is core 5, same as the default XAudio2 core
		m_chatManager->ChatSettings->AudioThreadAffinityMask = ProcessorToAffinityMask(cpuCore);
		// m_chatManager->ChatSettings->AudioThreadPriorityClass = REALTIME_PRIORITY_CLASS;
		// m_chatManager->ChatSettings->AudioEncodingQuality = Windows::Xbox::Chat::EncodingQuality::Normal;
		//m_chatManager->ChatSettings->JitterBufferEnabled = jitterBufferEnabled;

	#ifdef PROFILE
		m_chatManager->ChatSettings->PerformanceCountersEnabled = true;
	#endif

		// Upon enter constrained mode, mute everyone.
		// Upon leaving constrained mode, unmute everyone who was previously muted.
		m_tokenResourceAvailabilityChanged = Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged +=
		                                       ref new Windows::Foundation::EventHandler<Platform::Object^>([this](Platform::Object^, Platform::Object^)
		{
			if (Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Constrained)
			{
			  if (m_chatManager != nullptr)
			  {
			    m_chatManager->MuteAllUsersFromAllChannels();
			  }
			}
			else if (Windows::ApplicationModel::Core::CoreApplication::ResourceAvailability == Windows::ApplicationModel::Core::ResourceAvailability::Full)
			{
			  if (m_chatManager != nullptr)
			  {
			    m_chatManager->UnmuteAllUsersFromAllChannels();

			    // The title should remember who was muted so when the Resume even occurs
			    // to avoid unmuting users who has been previously muted.  Simply re-mute them here
			  }
			}
		});

		RegisterChatManagerEventHandlers();
	}

	return error;
}

void ChatIntegrationLayer::Shutdown()
{
	if (m_chatManager != nullptr)
	{
		for (UINT i = 0; i < m_remoteConsoleConnections.size(); i++)
		{
			Platform::Object^ uniqueConsoleId = Uint32ToPlatformObject(m_remoteConsoleConnections[i].remoteConsoleId);
			m_chatManager->RemoveRemoteConsoleAsync(uniqueConsoleId);
		}

		m_chatManager->OnDebugMessage -= m_tokenOnDebugMessage;
		m_chatManager->OnOutgoingChatPacketReady -= m_tokenOnOutgoingChatPacketReady;
		m_chatManager->OnCompareUniqueConsoleIdentifiers -= m_tokenOnCompareUniqueConsoleIdentifiers;
		Windows::ApplicationModel::Core::CoreApplication::ResourceAvailabilityChanged -= m_tokenResourceAvailabilityChanged;
		Windows::Xbox::System::User::AudioDeviceAdded -= m_tokenAudioDeviceAdded;
		m_chatManager = nullptr;
	}

	m_pMatchMaking = nullptr;
	m_remoteConsoleConnections.clear();
}

void ChatIntegrationLayer::Reset()
{
	auto temp = m_pMatchMaking;
	Shutdown();
	Initialize(true, temp);
}

void ChatIntegrationLayer::RegisterChatManagerEventHandlers()
{
	assert(m_chatManager != nullptr);

	m_tokenOnDebugMessage = m_chatManager->OnDebugMessage += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::DebugMessageEventArgs^>(
	                          [this](Platform::Object^, Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args)
	{
		OnDebugMessageReceived(args);
	});

	m_tokenOnOutgoingChatPacketReady = m_chatManager->OnOutgoingChatPacketReady += ref new Windows::Foundation::EventHandler<Microsoft::Xbox::GameChat::ChatPacketEventArgs^>(
	                                     [this](Platform::Object^, Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args)
	{
		OnOutgoingChatPacketReady(args);
	});

	m_tokenOnCompareUniqueConsoleIdentifiers = m_chatManager->OnCompareUniqueConsoleIdentifiers += ref new Microsoft::Xbox::GameChat::CompareUniqueConsoleIdentifiersHandler([this](Platform::Object^ obj1, Platform::Object^ obj2)
	{
		return CompareUniqueConsoleIdentifiers(obj1, obj2);
	});

	m_tokenAudioDeviceAdded = Windows::Xbox::System::User::AudioDeviceAdded += ref new Windows::Foundation::EventHandler<Windows::Xbox::System::AudioDeviceAddedEventArgs^>(
	                            [this](Platform::Object^, Windows::Xbox::System::AudioDeviceAddedEventArgs^ eventArgs)
	{
		OnUserAudioDeviceAdded(eventArgs);
	});
}

void ChatIntegrationLayer::OnUserAudioDeviceAdded(Windows::Xbox::System::AudioDeviceAddedEventArgs^ eventArgs)
{
	assert(eventArgs != nullptr);

	Windows::Xbox::System::User^ pUser = eventArgs->User;

	if (pUser != nullptr)
	{
		GetChatIntegrationLayer()->RemoveUserFromChatChannel(0, pUser);
		GetChatIntegrationLayer()->AddLocalUserToChatChannel(0, pUser);
	}
}

void ChatIntegrationLayer::OnDebugMessageReceived(
  __in Microsoft::Xbox::GameChat::DebugMessageEventArgs^ args
  )
{
	// To integrate the Chat DLL in your game,
	// change this to false and remove the LogComment calls,
	// or integrate with your game's own UI/debug message logging system
	bool outputToUI = true;

	if (outputToUI)
	{
		if (args->ErrorCode == S_OK)
		{
			LogComment(args->Message);
		}
		else
		{
			LogCommentWithError(args->Message, args->ErrorCode);
		}
	}
	else
	{
		// The string appear in the Visual Studio Output window
		NetLog("[chat] %ls", args->Message->Data());
	}
}

void ChatIntegrationLayer::OnOutgoingChatPacketReady(
  __in Microsoft::Xbox::GameChat::ChatPacketEventArgs^ args
  )
{
	CCryDurangoLiveLobbyPacket packet;
	CreatePacket(&packet, args->PacketBuffer, args->SendReliable);

	if (args->SendPacketToAllConnectedConsoles)
	{
		// Send the chat packet to all connected consoles
		for (UINT i = 0; i < m_remoteConsoleConnections.size(); i++)
		{
			m_pMatchMaking->Send(CryMatchMakingInvalidTaskID, &packet, m_remoteConsoleConnections[i].sessionHandle, m_remoteConsoleConnections[i].connectionID);
		}
	}
	else
	{
		// Send the chat message to a specific console
		uint32 remoteConsoleId = PlatformObjectToUint32(args->UniqueTargetConsoleIdentifier);       // XXX - DEBUG, is this valid?
		for (uint i = 0; i < m_remoteConsoleConnections.size(); i++)
		{
			if (m_remoteConsoleConnections[i].remoteConsoleId == remoteConsoleId)
			{
				m_pMatchMaking->Send(CryMatchMakingInvalidTaskID, &packet, m_remoteConsoleConnections[i].sessionHandle, m_remoteConsoleConnections[i].connectionID);
			}
		}
	}
}

//--------------------------------------------------------------------------------------
// Name: GetPointerToBufferData()
// Desc: Returns the pointer to the IBuffer data.
// NOTE: The pointer is only valid for the lifetime of the governing IBuffer^
//--------------------------------------------------------------------------------------
void* GetPointerToBufferData(Windows::Storage::Streams::IBuffer^ buffer)
{
	// Obtain IBufferByteAccess from IBuffer
	ComPtr<IUnknown> pBuffer((IUnknown*)buffer);
	ComPtr<IBufferByteAccess> pBufferByteAccess;
	pBuffer.As(&pBufferByteAccess);

	// Get pointer to data
	byte* pData = nullptr;
	if (FAILED(pBufferByteAccess->Buffer(&pData)))
	{
		// Buffer is not annotated with _COM_Outpr, so if it were to fail, then the value of pData is undefined
		pData = nullptr;
	}

	return pData;
}

void ChatIntegrationLayer::ReceivePacket(CCryDurangoLiveLobbyPacket* liveLobbyPacket)
{
	if (m_chatManager)
	{
		if (liveLobbyPacket == nullptr)
		{
			NetLog("[chat] Empty chat packet received");
			return;
		}

		// Unpack the chat packet from the Cry packet
		byte* pData = nullptr;
		uint size = 0;
		UnwrapPacket(liveLobbyPacket, &pData, &size);          // NOTE: Remember to free pData that was alloceted here

		if (pData == nullptr || size == 0)
		{
			NetLog("[chat] Empty chat packet received");
			return;
		}

		uint32 remoteConsoleID = liveLobbyPacket->GetFromConnectionUID().m_uid;
		Platform::Object^ uniqueRemoteConsoleIdentifier = Uint32ToPlatformObject(remoteConsoleID);

		// XXX - NOTE, do we have to ref new each time and copy the bytes?
		PREFAST_SUPPRESS_WARNING(6387); // size cannot be 0
		Windows::Storage::Streams::Buffer^ chatMessage = ref new Windows::Storage::Streams::Buffer(size);
		void* srcBufferBytes = GetPointerToBufferData(chatMessage);
		chatMessage->Length = size;
		memcpy(srcBufferBytes, pData, size);

		OnIncomingChatMessage(chatMessage, uniqueRemoteConsoleIdentifier);

		chatMessage = nullptr;
		delete[] pData;
	}
}

void ChatIntegrationLayer::OnIncomingChatMessage(
  Windows::Storage::Streams::IBuffer^ chatMessage,
  Platform::Object^ uniqueRemoteConsoleIdentifier
  )
{
	if (m_chatManager)
	{
		m_chatManager->ProcessIncomingChatMessage(chatMessage, uniqueRemoteConsoleIdentifier);
	}
}

// Only add people who intend to play.
void ChatIntegrationLayer::AddAllLocallySignedInUsersToChatClient(
  __in uint8 channelIndex,
  __in Windows::Foundation::Collections::IVectorView<Windows::Xbox::System::User^>^ locallySignedInUsers
  )
{
	// To integrate the Chat DLL in your game,
	// add all locally signed in users to the chat client
	// For now, you need to supply a uint8 which is a uniquely identifies this console across the session
	// If you do not have this in your title, you can use a random number and it'll work most of the time
	// which isn't great but is good enough for early integration.
	// This requirement will be removed in an upcoming revision
	/*    for each( Windows::Xbox::System::User^ user in locallySignedInUsers )
	    {
	        if( user != nullptr )
	        {
	            LogComment(L"Adding Local User to Chat Client");

	            m_chatManager->AddLocalUserToChatChannel(
	                DEFAULT_CHANNEL_INDEX,
	                user
	                );
	        }
	    }*/
}

void ChatIntegrationLayer::AddLocalUserToChatChannel(
  __in uint8 channelIndex,
  __in Windows::Xbox::System::IUser^ user
  )
{
	if (user && m_chatManager)
	{
		m_chatManager->AddLocalUserToChatChannelAsync(channelIndex, user);
	}
}

void ChatIntegrationLayer::RemoveRemoteConsole(
  Platform::Object^ uniqueRemoteConsoleIdentifier
  )
{
	// uniqueConsoleIdentifier is a Platform::Object^ and can be cast or unboxed to most types.
	// What exactly you use doesn't matter, but optimally it would be something that uniquely identifies a console on in the session.
	// A Windows::Xbox::Networking::SecureDeviceAssociation^ is perfect to use if you have access to it.

	// This is how you would convert from an int to a Platform::Object^
	// Platform::Object obj = IntToPlatformObject(5);

	if (m_chatManager)
	{
		m_chatManager->RemoveRemoteConsoleAsync(uniqueRemoteConsoleIdentifier);
	}
}

void ChatIntegrationLayer::RemoveUserFromChatChannel(
  __in uint8 channelIndex,
  __in Windows::Xbox::System::IUser^ user
  )
{
	if (user && m_chatManager)
	{
		m_chatManager->RemoveLocalUserFromChatChannelAsync(channelIndex, user);
	}
}

void ChatIntegrationLayer::OnSecureDeviceAssocationConnectionEstablished(
  __in Platform::Object^ uniqueConsoleIdentifier
  )
{
	if (m_chatManager != nullptr)
	{
		m_chatManager->HandleNewRemoteConsole(uniqueConsoleIdentifier);
	}
}

Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ ChatIntegrationLayer::GetChatUsers()
{
	if (m_chatManager != nullptr)
	{
		return m_chatManager->GetChatUsers();
	}

	return nullptr;
}

bool ChatIntegrationLayer::HasMicFocus()
{
	if (m_chatManager != nullptr)
	{
		return m_chatManager->HasMicFocus;
	}

	return false;
}

inline bool IsStringEqualCaseInsenstive(Platform::String^ val1, Platform::String^ val2)
{
	return (_wcsicmp(val1->Data(), val2->Data()) == 0);
}

void ChatIntegrationLayer::HandleChatChannelChanged(
  __in uint8 oldChatChannelIndex,
  __in uint8 newChatChannelIndex,
  __in INT index
  )
{
	// We remember if the local user was currently muted from all channels. And when we switch channels,
	// we ensure that the state persists. For remote users, title should implement this themselves
	// based on title game design if they want to persist the muting state.

	bool wasUserMuted = false;
	Windows::Xbox::System::IUser^ userBeingRemoved = nullptr;

	auto chatUsers = GetChatUsers();
	if (chatUsers != nullptr)
	{
		Microsoft::Xbox::GameChat::ChatUser^ chatUser = chatUsers->GetAt(index);
		if (chatUser != nullptr && chatUser->IsLocal)
		{
			wasUserMuted = chatUser->IsMuted;
			userBeingRemoved = chatUser->User;
			if (userBeingRemoved != nullptr)
			{
				RemoveUserFromChatChannel(oldChatChannelIndex, userBeingRemoved);
				AddLocalUserToChatChannel(newChatChannelIndex, userBeingRemoved);
			}
		}
	}

	// If the local user was muted earlier, get the latest chat users and mute him again on the newly added channel.
	if (wasUserMuted && userBeingRemoved != nullptr)
	{
		if (chatUsers != nullptr)
		{
			for (UINT chatUserIndex = 0; chatUserIndex < chatUsers->Size; chatUserIndex++)
			{
				Microsoft::Xbox::GameChat::ChatUser^ chatUser = chatUsers->GetAt(chatUserIndex);
				if (chatUser != nullptr &&
				    IsStringEqualCaseInsenstive(chatUser->XboxUserId, userBeingRemoved->XboxUserId))
				{
					m_chatManager->MuteUserFromAllChannels(chatUser);
					break;
				}
			}
		}
	}
}

void ChatIntegrationLayer::HandleChatUserMuteStateChanged(
  __in INT chatUserIndex
  )
{
	Windows::Foundation::Collections::IVectorView<Microsoft::Xbox::GameChat::ChatUser^>^ chatUsers = GetChatUsers();
	Microsoft::Xbox::GameChat::ChatUser^ chatUser = chatUsers->GetAt(chatUserIndex);
	if (m_chatManager != nullptr)
	{
		if (chatUser != nullptr)
		{
			if (chatUser->IsMuted)
			{
				m_chatManager->UnmuteUserFromAllChannels(chatUser);
			}
			else
			{
				m_chatManager->MuteUserFromAllChannels(chatUser);
			}
		}
	}
}

Microsoft::Xbox::GameChat::ChatUser^ ChatIntegrationLayer::GetChatUserByXboxUserId(
  __in Platform::String^ xboxUserId
  )
{
	auto chatUsers = GetChatUsers();
	if (chatUsers)
	{
		auto numChatUsers = chatUsers->Size;
		for (uint i = 0; i < numChatUsers; i++)
		{
			auto chatUser = chatUsers->GetAt(i);
			if (chatUser != nullptr &&
			    IsStringEqualCaseInsenstive(chatUser->XboxUserId, xboxUserId))
			{
				return chatUser;
			}
		}
	}

	return nullptr;
}

void ChatIntegrationLayer::CreatePacket(_Inout_ CCryDurangoLiveLobbyPacket* pPacket, _In_ Windows::Storage::Streams::IBuffer^ pBuffer, _In_ bool reliable)
{
	if (pPacket == nullptr || pBuffer == nullptr || pBuffer->Length == 0)
	{
		return;
	}

	// Setup Cry packet
	const uint16 chatPacketSize = (uint16)pBuffer->Length;
	const uint32 packetSize = CryLobbyPacketHeaderSize + sizeof(chatPacketSize) + pBuffer->Length;

	// Start writing into the packet
	if (pPacket->CreateWriteBuffer(packetSize))
	{
		pPacket->StartWrite(eDurangoLivePT_Chat, reliable);

		// Write chat packet size
		pPacket->WriteUINT16(chatPacketSize);

		// Write chat data into the packet
		void* pData = GetPointerToBufferData(pBuffer);
		pPacket->WriteData(pData, chatPacketSize);
	}
}

void ChatIntegrationLayer::UnwrapPacket(_In_ CCryDurangoLiveLobbyPacket* pPacket, _Out_ byte** ppData, _Out_ uint* size)
{
	if (pPacket == nullptr)
	{
		*ppData = nullptr;
		*size = 0;
		return;
	}

	if (pPacket->StartRead() == eDurangoLivePT_Chat)
	{
		// Read chat data from the packet
		const uint16 chatSize = pPacket->ReadUINT16();
		*size = (uint)chatSize;
		*ppData = new byte[chatSize];
		pPacket->ReadData(*ppData, chatSize);
	}
	else
	{
		*ppData = nullptr;
		*size = 0;
		NetLog("[chat] ERROR: Wrong packet type!");
		return;
	}
}

void ChatIntegrationLayer::AddRemoteConnection(CryLobbySessionHandle h, CryMatchMakingConnectionID connectionID, uint32 consoleId)
{
	// NOTE: The sessionHandle and remoteConsoleId is unique, but the connectionID will increment each time until an ACK was received from the
	// remote, i.e. we want to just store the last connectionID in this list

	// first check if this connection is not in the list
	for (UINT i = 0; i < m_remoteConsoleConnections.size(); i++)
	{
		if (m_remoteConsoleConnections[i].sessionHandle == h &&
		    m_remoteConsoleConnections[i].remoteConsoleId == consoleId)
		{
			if (m_remoteConsoleConnections[i].connectionID == connectionID)
			{
				// For some reason we are trying to add the exact same information to the list
				return;
			}
			else
			{
				// Here we need to update the already added connection with the latest connectionID
				m_remoteConsoleConnections[i].connectionID = connectionID;
				auto uniqueConsoleId = ChatIntegrationLayer::Uint32ToPlatformObject(consoleId);
				OnSecureDeviceAssocationConnectionEstablished(uniqueConsoleId);
				return;
			}
		}
	}

	// At this point it is a brand new connection we need to add/
	ConnectionData connectionData;
	connectionData.sessionHandle = h;
	connectionData.connectionID = connectionID;
	connectionData.remoteConsoleId = consoleId;
	m_remoteConsoleConnections.push_back(connectionData);

	auto uniqueConsoleId = ChatIntegrationLayer::Uint32ToPlatformObject(consoleId);
	OnSecureDeviceAssocationConnectionEstablished(uniqueConsoleId);
}

void ChatIntegrationLayer::RemoveRemoteConnection(CryLobbySessionHandle h, CryMatchMakingConnectionID connectionID, uint32 consoleId)
{
	if (m_chatManager)
	{
		Platform::Object^ uniqueConsoleId = Uint32ToPlatformObject(consoleId);
		m_chatManager->RemoveRemoteConsoleAsync(uniqueConsoleId);
	}

	for (UINT i = 0; i < m_remoteConsoleConnections.size(); i++)
	{
		if (m_remoteConsoleConnections[i].sessionHandle == h &&
		    m_remoteConsoleConnections[i].connectionID == connectionID &&
		    m_remoteConsoleConnections[i].remoteConsoleId == consoleId)
		{
			m_remoteConsoleConnections.erase(m_remoteConsoleConnections.begin() + i);
			return;
		}
	}
}

#endif
