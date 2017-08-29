// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Connection.h"

#include "ProtobufHelpers.h"

#include <CryCore/functor.h>
#include <CryCloud/IOnlineResponseHandler.h>

#include "ConnectionChannel.h"

#include "GeneratedFiles/Header.pb.h"
#include "GeneratedFiles/ProtocolMetadata.pb.h"

#define HEARTBEAT_INTERVAL 0 // secs

namespace CryCloud
{
	CRYTIMEVAL CConnection::s_zeroTimeout = { 0, 0 };

	CConnection::CConnection()
		: m_port(0)
		, m_socket(0)
		, m_lastRequestId(InvalidMessageId)
		, m_connectionStatus(EConnectionStatus::Inactive)
		, m_recvDataLength(0)
		, m_pRecvDataBuffer(nullptr)
		, m_pCurrentSend(0)
		, m_bThreadRunning(false)
		, m_bThreadQuit(false)
	{
		m_channels.reserve(32);

		CreateDefaultChannel();
	}

	CConnection::~CConnection()
	{
		Shutdown();
	}

	void CConnection::Init(COnlineConfigurationPtr pConfiguration)
	{
		FillServerEntries(pConfiguration);

		const SServerAddress& address = SelectNextServer();

		m_hostname = address.address;
		m_port = address.port;
		m_connectionTimeout = CTimeout(pConfiguration->GetConnectionTimeout());

		StartThread();
	}

	void CConnection::AddChannel(const SConnectionChannelConfig& config)
	{
		if (FindChannel(config.id) != nullptr)
			return;

		CConnectionChannel* pChannel = new CConnectionChannel(config.id, config.szName, config.maxPendingMessages, config.maxPartSize, this);
		{
			AUTO_MODIFYLOCK(m_channelsLock);
			m_channels.push_back(pChannel);
		}
	}

	void CConnection::Request(EConnectionRequest request)
	{
		m_connectionRequest.Push(request);
	}

	EConnectionStatus CConnection::GetStatus() const
	{
		// Note: This function can read from different thread. 
		//       Although race conditions could happen, they should not cause issues
		return m_connectionStatus;
	}

	void CConnection::ThreadEntry()
	{
		while (!m_bThreadQuit)
		{
			OnThreadTick();

			CrySleep(100);
		}
	}

	void CConnection::StartThread()
	{
		if (m_bThreadRunning)
			return;

		m_bThreadRunning = gEnv->pThreadManager->SpawnThread(this, "CryCloudClient_Connection");
	}

	void CConnection::StopThread()
	{
		if (m_bThreadRunning)
		{
			m_bThreadQuit = true;
			gEnv->pThreadManager->JoinThread(this, eJM_Join);
			m_bThreadRunning = false;
		}
	}

	void CConnection::Shutdown()
	{
		StopThread();

		Disconnect(EDisconnectReason::Shutdown);

		for (CConnectionChannel* pChannel : m_channels)
		{
			SAFE_DELETE(pChannel);
		}
		m_channels.clear();

		SAFE_DELETE(m_pCurrentSend);
		SAFE_DELETE_ARRAY(m_pRecvDataBuffer);
	}

	void CConnection::PollConnectionStatus()
	{
		if ((m_connectionStatus != EConnectionStatus::Connected) && m_socket > 0)
		{
			// ret == 1: Connection has completely established.
			// ret == 0: Connection timed out
			// ret < 0:  Connection error.
			
			CRYTIMEVAL timeout = s_zeroTimeout;

			const int result = CrySock::WaitForWritableSocket(m_socket, &timeout);

			const bool bConnectionError = (result <= 0) && (result != CrySock::eCSE_EWOULDBLOCK_CONN);
			const bool bConnectionTimeout = m_connectionTimeout.HasExpired();
			if (bConnectionError || bConnectionTimeout)
			{
				CRYCLOUD_CLIENT_LOG("Connection %s. Result: %d - Socket Error: %d", 
					bConnectionTimeout ? "Timed out" : "Failed",
					result, CrySock::TranslateSocketError(result));
				Disconnect();
				return;
			}

			if (result == 1)
			{
				CRYCLOUD_CLIENT_LOG("Connection Succeeded");
				m_connectionStatus = EConnectionStatus::Connected;
				m_lastSentDataTime = gEnv->pTimer->GetCurrTime();

				// FIX_ME - workaround for compiling with latest Engine interface that redefines CRYSOCKLEN as CRYSOCKLEN_T. Update this when we migrate the main branch
	#if defined(CRY_PLATFORM_ORBIS) || defined(CRY_PLATFORM_LINUX)
				unsigned int len = sizeof(int);
	#else
				int len = sizeof(int);
	#endif
				m_pRecvDataBuffer = new char[eRecvBufferSize];
			}
		}
	}

	void CConnection::OnThreadTick()
	{
		const EConnectionRequest pendingRequest = m_connectionRequest.Pop();
		const EConnectionStatus status = GetStatus();

		if (status == EConnectionStatus::Connected)
		{
			CTimeValue t = gEnv->pTimer->GetFrameStartTime();
			if (HEARTBEAT_INTERVAL && (t.GetDifferenceInSeconds(m_lastSentDataTime) > HEARTBEAT_INTERVAL))
			{
				SendHeartbeat();
				CRYCLOUD_CLIENT_LOG("Heartbeat Sent");
			}

			SendPackets();
			RecvPackets();
		}
		else if (status == EConnectionStatus::Establishing)
		{
			PollConnectionStatus();
		}
		else
		{
			if (pendingRequest == EConnectionRequest::Connect)
			{
				Connect();
			}
		}

		if (pendingRequest == EConnectionRequest::Disconnect)
		{
			Disconnect();
		}
	}

	void CConnection::SendPackets()
	{
		while (IsConnected())
		{
			if (!m_pCurrentSend)
				m_pCurrentSend = PrepareNextPartToSend();

			if (!m_pCurrentSend)
				break;

			bool writable = CanWrite();
			if (!writable) 
				break;

			int bytesLeft = m_pCurrentSend->length - m_pCurrentSend->written;
			int ret = SendData(&m_pCurrentSend->pData[m_pCurrentSend->written], bytesLeft);

			if (ret > 0 && ret < bytesLeft)
			{
				m_pCurrentSend->written += ret;
				break;
			}
			else if (ret <= 0)
			{
				CRYCLOUD_CLIENT_LOG("[Error] Send packets failed. Error code: %d", ret);
				if (ret != CrySock::eCSE_EWOULDBLOCK && ret != CrySock::eCSE_ENOBUFS)
				{
					Disconnect();
				}
				break;
			}

			SAFE_DELETE(m_pCurrentSend);
		}
	}

	void CConnection::RecvPackets()
	{
		while (IsConnected() && CanRead())
		{
			int ret = RecvData(m_pRecvDataBuffer, eRecvBufferSize);
			if (ret > 0)
			{
				ParserFeed(m_pRecvDataBuffer, ret);
			}
			else if (ret < 0)
			{
				if (ret != CrySock::eCSE_EWOULDBLOCK)
				{
					CRYCLOUD_CLIENT_LOG("[Error] Recv packets failed. Error code: %d", ret);
					Disconnect();
					break;
				}
			}
			else
			{
				CRYCLOUD_CLIENT_LOG("Connection closed by remote peer. Disconnecting...");
				Disconnect();
				break;
			}
		}
	}

	bool CConnection::CanRead() const
	{
		if (!IsConnected())
			return false;

		return CrySock::IsRecvPending(m_socket, &s_zeroTimeout) > 0;
	}

	bool CConnection::CanWrite() const
	{
		if (!IsConnected())
			return false;

		return CrySock::WaitForWritableSocket(m_socket, &s_zeroTimeout) > 0;
	}

	void CConnection::SendPacket(CConnectionChannel* pChannel, const FP::ReqHeader& header, const google::protobuf::Message* pMessage)
	{
		CConnectionChannel::SPacket* pPacket = new CConnectionChannel::SPacket(header.message_id());
		ProtobufHelpers::SerializeMessage(header, pMessage, pPacket->packetData);

		pChannel->SendPacket(pPacket);

		m_lastSentDataTime = gEnv->pTimer->GetFrameStartTime();
	}

	EClientError CConnection::SendPacketToChannel(const char* szChannelName, const FP::ReqHeader& header, const google::protobuf::Message* pMessage)
	{
		CConnectionChannel* pChannel = FindChannel(szChannelName);
		if (!pChannel)
		{
			CRYCLOUD_CLIENT_LOG("[Error] No channel '%s' found. Message could not be sent.", szChannelName);
			return eCCCE_ChannelNotFound;
		}

		SendPacket(pChannel, header, pMessage);
		return eCCCE_Success;
	}

	CConnection::SSendNode* CConnection::PrepareNextPartToSend()
	{
		char* pData = 0;
		size_t size = 0;

		AUTO_READLOCK(m_channelsLock);

		for (auto pChannel : m_channels)
		{
			if (pChannel->GetPartToSend(pData, size))
			{
				CConnectionChannel::STransportPacket* pTransportPacket = CConnectionChannel::STransportPacket::Allocate(size, pChannel->GetId());
				memcpy(pTransportPacket->data, pData, size);

				SSendNode* pNode = new SSendNode((char*)pTransportPacket, pTransportPacket->dataSize + sizeof(uint32));

				return pNode;
			}
		}

		return 0;
	}

	void CConnection::SendHeartbeat()
	{
		MessageId messageId = GenerateRequestId();

		FP::ReqHeader requestHeader;
		requestHeader.set_grain_id(0);
		requestHeader.set_message_id(messageId);
		requestHeader.set_invoking_id((uint32)FP::DefaultInvokingIds::HeartBeatId);
		requestHeader.set_body_length(0);

		SendPacketToChannel(CRYCLOUD_CLIENT_DEFAULT_CHANNEL_NAME, requestHeader, nullptr);
	}


	MessageId CConnection::SendRequest(GrainId grainId, uint32 invokeId, const google::protobuf::Message& message, IResponseHandlerPtr pResponseHandler, const char* szChannelName)
	{
		// protocol reqeust data frame definition
		// ----------------------------------------------------------------------
		// uint4 sizeOfData
		// uint4 messageId
		// int4 sizeofMethodName
		// string(sizeofMethodName) methodName
		// uint4 body_length
		//
		// ----------------------------------------------------------------------
		const MessageId messageId = GenerateRequestId();

		FP::ReqHeader requestHeader;
		requestHeader.set_grain_id(grainId);
		requestHeader.set_message_id(messageId);
		requestHeader.set_invoking_id(invokeId);
		requestHeader.set_body_length(message.ByteSize());

		EClientError err = SendPacketToChannel(szChannelName, requestHeader, &message);
		if (err != eCCCE_Success)
		{
			if (pResponseHandler)
				pResponseHandler->OnError(messageId, (ErrorCode)err);

			return messageId;
		}

		{
			CryAutoLock<CryMutex> lock(m_pendingRequestsLock);
			m_pendingRequests[messageId] = pResponseHandler;

			CRYCLOUD_CLIENT_LOG("Sending request: Invoke (0x%08x) remotely on server with messageId = %d, grainId = %ld", invokeId, messageId, grainId);
			return messageId;
		}
	}

	void CConnection::PumpResponses()
	{
		TResponsesQueue responses;
		{
			CryAutoLock<CryMutex> lock(m_responsesQueueLock);
			responses.swap(m_responsesQueue);
		}

		while (!responses.empty())
		{
			IResponseHandlerPtr pRspHandler = responses.front();
			pRspHandler->Dispatch();

			responses.pop();
		}
	}

	void CConnection::ParseTransportFrame(char* pBuffer, int length)
	{
		CConnectionChannel::STransportPacket* pPacket = (CConnectionChannel::STransportPacket*)pBuffer;
		if(!pPacket->IsValidPacket(length))
		{
			CRYCLOUD_CLIENT_LOG("[Error] Invalid transport packet (Disconnecting...)");
			Disconnect();
			return;
		}

		CConnectionChannel* pChannel = FindChannel(pPacket->channelId);
		if (!pChannel)
		{
			CRYCLOUD_CLIENT_LOG("[Error] Invalid receive channel (Disconnecting...)");
			Disconnect();
			return;
		}

		pChannel->ReceiveTransportPacket(pPacket);
	}

	void CConnection::ParserFeed(char buffer[], int length)
	{
		if( length <= 0 ) 
		{
			CRYCLOUD_CLIENT_LOG("[Error] Parsing feed length <= 0 (Disconnecting...)");
			Disconnect();
			return;
		}
	
		// size of data
		// size of responseHeader
		// responseHeader
		// contentBody

		int bufferIndex = 0;
		while(bufferIndex<length)
		{
			if (m_sizeToRead == 0)
			{
				if( m_recvDataLength == 0 )
				{
					m_sizeToRead = sizeof(uint32);
					m_read = 0;
					m_pReceivingData = m_recvDataLengthArray;
				}
				else
				{
					m_sizeToRead = m_recvDataLength + sizeof(uint32);
					m_read = sizeof(uint32);
					m_pReceivingData = new char[m_sizeToRead];

					CConnectionChannel::STransportPacket* pPacket = (CConnectionChannel::STransportPacket*)m_pReceivingData;
					pPacket->dataSize = m_recvDataLength;
				}
			}

			int thisRead = std::min<int>(m_sizeToRead-m_read, length-bufferIndex);
			memcpy(m_pReceivingData+m_read, buffer+bufferIndex, thisRead);
			m_read += thisRead;
			bufferIndex += thisRead;
			if(m_read == m_sizeToRead)
			{
				m_sizeToRead = 0;
				if(m_recvDataLength == 0)
					memcpy(&m_recvDataLength, m_pReceivingData, sizeof(uint32));
				else
				{
					ParseTransportFrame(m_pReceivingData, m_recvDataLength);
					SAFE_DELETE_ARRAY(m_pReceivingData);
					m_recvDataLength=0;
				}
			}
		}
	}

	/*static*/ bool CConnection::TestPacketCompletion(const std::vector<char>& data)
	{
		if (data.size() < sizeof(uint32))
			return false;

		uint32 totalSize = *(uint32*)&data[0];
		if (data.size() < totalSize + sizeof(uint32))
			return false;

		return true;
	}

	void CConnection::ParseDataFrame(char* pBuffer, size_t cbLen)
	{
		ProtobufHelpers::CFrameParser frameParser(pBuffer, cbLen);

		const MessageId msgId = frameParser.GetMessageId();
		{
			IResponseHandlerPtr pResponse;
			{
				CryAutoLock<CryMutex> lock(m_pendingRequestsLock);
				if (m_pendingRequests.find(msgId) != m_pendingRequests.end())
				{
					pResponse = m_pendingRequests[msgId];

					if (pResponse->IsStream())
					{
						// since the response handler is stateful, we must have one instance for each individual
						// coming msg of that type, otherwise data gets overwritten

						// TODO: having fully dynamic response handler is a bit costy, while in real scenario callback
						//       sites are often static -- more so for subscriptions, in that case only
						//       the real message need to be passed down the queue
						pResponse = pResponse->Clone();
					}
					else
					{
						m_pendingRequests.erase(msgId);
					}

					if(frameParser.GetUnsubscribeId() != 0) 
						m_pendingRequests.erase(frameParser.GetUnsubscribeId());
				}
			}

			if (pResponse != nullptr)
			{
				switch (frameParser.GetErrorCode())
				{
				case FP::RespErrorCode::Success:
					{
						CRYCLOUD_CLIENT_LOG("RECV (messageid: %d, unsubscribe_id: %d)", frameParser.GetMessageId(), frameParser.GetUnsubscribeId());

						pResponse->OnResponse(msgId, frameParser.GetMessageBody(), frameParser.GetMessageBodyLength());
					}
					break;
				case FP::RespErrorCode::FepOverload:
					Disconnect();
					//break; // Pass-through (response is not in 'pendingRequest' map, we have to still queue the error notification)
				default:
					CRYCLOUD_CLIENT_LOG("ERROR (messageid: %d) %s", msgId, frameParser.GetErrorMessage());
					pResponse->OnError(msgId, frameParser.GetErrorCode());
					break;
				}

				// Push to queue for later dispatch
				{
					CryAutoLock<CryMutex> lock(m_responsesQueueLock);
					m_responsesQueue.push(pResponse);
				}
			}

			{
				AUTO_READLOCK(m_channelsLock);

				for (auto pChannel : m_channels)
					pChannel->OnReceivedResponse(msgId);
			}
		}
	}

	int CConnection::SendData(const char* pData, int size)
	{
		return CrySock::send(m_socket, pData, size, 0);
	}

	int CConnection::RecvData(char* pData, int size)
	{
		return CrySock::recv(m_socket, pData, size, 0);
	}

	bool CConnection::Connect()
	{
		m_socket = CrySock::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket < 0)
		{
			CRYCLOUD_CLIENT_LOG("[Error]  Failed to create a valid socket");
			m_socket = 0;
			return false;
		}
	
		if (!CrySock::MakeSocketNonBlocking(m_socket))
		{
			CRYCLOUD_CLIENT_LOG("[Error] Failed to make socket non-blocking");
			// TODO pavloi 2017.02.14: close socket to prevent resource leaking?
			return false;
		}

		CRYSOCKADDR_IN sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(m_port);
		sa.sin_addr.s_addr = CrySock::DNSLookup(m_hostname.c_str());

		m_read = 0;
		m_recvDataLength = 0;
		m_sizeToRead = 0;
		m_pReceivingData = nullptr;
		m_connectionStatus = EConnectionStatus::Establishing;

		if (sa.sin_addr.s_addr)
		{
			int result = CrySock::connect(m_socket, (const CRYSOCKADDR*)&sa, sizeof(sa));
			result = CrySock::TranslateSocketError(result);
			if (result != CrySock::eCSE_NO_ERROR 
			    && result != CrySock::eCSE_EWOULDBLOCK_CONN 
			    && result != CrySock::eCSE_EWOULDBLOCK
			    && result != CrySock::eCSE_EINPROGRESS)
			{
				CRYCLOUD_CLIENT_LOG("[Error] Connection with '%s:%d' failed. Error code: %d", m_hostname.c_str(), m_port, result);
				Disconnect();
				return false;
			}
		}
		else
		{
			CRYCLOUD_CLIENT_LOG("[Error] Resolving Hostname '%s' failed", m_hostname.c_str());
			return false;
		}

		CRYCLOUD_CLIENT_LOG("Connection with '%s:%d' open", m_hostname.c_str(), m_port);

		m_connectionTimeout.Start();

		return true;
	}
	void CConnection::Disconnect(const EDisconnectReason reason /*= EDisconnectReason::None*/)
	{
		if (m_socket > 0)
		{
			CRYCLOUD_CLIENT_LOG("Disconnect");

			if(m_pReceivingData != m_recvDataLengthArray)
			{
				SAFE_DELETE_ARRAY(m_pReceivingData);
			}

			CrySock::shutdown(m_socket, 2);
			CrySock::closesocket(m_socket);

			m_socket = 0;
			m_connectionStatus = EConnectionStatus::Disconnected;
		}

		// Push an error message to all pending requests

		TPendingRequestsMap pendingRequests;
		{
			CryAutoLock<CryMutex> lock(m_pendingRequestsLock);
			pendingRequests.swap(m_pendingRequests);
		}

		const bool bNotifyError = (reason != EDisconnectReason::Shutdown);
		if (bNotifyError)
		{
			CryAutoLock<CryMutex> lock(m_responsesQueueLock);

			for (auto it = pendingRequests.begin(); it != pendingRequests.end(); ++it)
			{
				IResponseHandlerPtr pRspHandler = it->second;
				if (pRspHandler)
				{
					pRspHandler->OnError(it->first, eCCCE_ConnectionClosed);
					m_responsesQueue.push(pRspHandler);
				
					CRYCLOUD_CLIENT_LOG("ERROR (messageid: %d) %s", it->first, "Connection Loss");
				}
			}
		}

		{
			AUTO_READLOCK(m_channelsLock);

			for (auto pChannel : m_channels)
			{
				pChannel->OnDisconnect();
			}
		}
	}

	bool CConnection::IsConnected() const
	{
		return (GetStatus() == EConnectionStatus::Connected);
	}

	CRYSOCKET CConnection::GetSocketHandle() const
	{
		return m_socket;
	}

	void CConnection::FillServerEntries(COnlineConfigurationPtr pConfiguration)
	{
		if (m_serverEntries.empty())
		{
			TServersList serverList;
			pConfiguration->GetServerList(serverList);

			for (const auto& server : serverList)
			{
				SServerEntry serverEntry;
				serverEntry.address = server;
				serverEntry.lastConnectionTime.SetSeconds(cry_random(0.0f, 1.0f));

				m_serverEntries.push_back(serverEntry);
			}
		}
	}

	const SServerAddress& CConnection::SelectNextServer()
	{
		if (!m_serverEntries.empty())
		{
			std::sort(m_serverEntries.begin(), m_serverEntries.end());

			auto& entry = m_serverEntries[0];
			entry.lastConnectionTime = gEnv->pTimer->GetAsyncTime();

			return entry.address;
		}
		else
		{
			return m_defaultServer.address;
		}
	}

	MessageId CConnection::GenerateRequestId()
	{
		MessageId requestId = InvalidMessageId;
		do 
		{
			requestId = (MessageId)CryInterlockedIncrement((int*)&m_lastRequestId);

		} while (requestId == InvalidMessageId);

		return requestId;
	}

	CConnectionChannel* CConnection::FindChannel(const string& name) const
	{
		AUTO_READLOCK(m_channelsLock);

		for (auto pChannel : m_channels)
		{
			if (pChannel->GetName() == name)
				return pChannel;
		}

		return nullptr;
	}

	CConnectionChannel* CConnection::FindChannel(uint32 id) const
	{
		AUTO_READLOCK(m_channelsLock);

		for (auto pChannel : m_channels)
		{
			if (pChannel->GetId() == id)
				return pChannel;
		}

		return nullptr;
	}

	void CConnection::CreateDefaultChannel()
	{
		const SConnectionChannelConfig config(0, CRYCLOUD_CLIENT_DEFAULT_CHANNEL_NAME, 0, 1000);
		AddChannel(config);
	}
}
