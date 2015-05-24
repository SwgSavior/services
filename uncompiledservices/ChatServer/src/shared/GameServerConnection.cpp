// GameServerConnection.cpp
// Copyright 2000-02, Sony Online Entertainment Inc., all rights reserved. 
// Author: Justin Randall

//-----------------------------------------------------------------------

#include "FirstChatServer.h"
#include "ChatInterface.h"
#include "ChatServer.h"
#include "ConfigChatServer.h"
#include "GameServerConnection.h"
#include "VChatInterface.h"
#include "sharedDebug/Profiler.h"
#include "sharedLog/Log.h"
#include "sharedNetworkMessages/ChatAvatarId.h"
#include "sharedNetworkMessages/ChatAvatarIdArchive.h"
#include "sharedNetworkMessages/ChatChangeFriendStatus.h"
#include "sharedNetworkMessages/ChatChangeIgnoreStatus.h"
#include "sharedNetworkMessages/ChatCreateRoom.h"
#include "sharedNetworkMessages/ChatDeleteAllPersistentMessages.h"
#include "sharedNetworkMessages/ChatDestroyRoomByName.h"
#include "sharedNetworkMessages/ChatDestroyRoomByName.h"
#include "sharedNetworkMessages/ChatGetFriendsList.h"
#include "sharedNetworkMessages/ChatGetIgnoreList.h"
#include "sharedNetworkMessages/ChatInviteAvatarToRoom.h"
#include "sharedNetworkMessages/ChatInviteGroupMembersToRoom.h"
#include "sharedNetworkMessages/ChatMessageFromGame.h"
#include "sharedNetworkMessages/GameNetworkMessage.h"
#include "sharedNetworkMessages/ChatPutAvatarInRoom.h"
#include "sharedNetworkMessages/ChatRemoveAvatarFromRoom.h"
#include "sharedNetworkMessages/ChatOnRequestLog.h"
#include "sharedNetworkMessages/ChatRequestLog.h"
#include "sharedNetworkMessages/ChatUninviteFromRoom.h"
#include "sharedNetworkMessages/GenericValueTypeMessage.h"
#include "UnicodeUtils.h"

#include "sharedNetworkMessages/VoiceChatChannelInfo.h"
#include "sharedNetworkMessages/VoiceChatMiscMessages.h"

#include <list>
#include <string>

//----------------------------------------------------------------------

GameServerConnection::GameServerConnection(UdpConnectionMT * u, TcpClient * t) :
ServerConnection (u, t),
m_gameCode ("SWG"), //@todo: this class must not depend on SWG
m_connectionId(0)
{
	ChatServer::fileLog(true, "GameServerConnection", "Connection created...listening on (%s:%d)", getRemoteAddress().c_str(), static_cast<int>(getRemotePort()));
}

//-----------------------------------------------------------------------

GameServerConnection::~GameServerConnection()
{
	if(ChatServer::instance().getGameServerConnectionFromId(m_connectionId))
	{
		ChatServer::fileLog(true, "GameServerConnection", "~GameServerConnection still registered myID(%u) %p",m_connectionId, this);
	}
	else
	{
		ChatServer::fileLog(true, "GameServerConnection", "~GameServerConnection not registered anymore myID(%u) %p",m_connectionId, this);
	}

	ChatServer::instance().unregisterGameServerConnection(m_connectionId);
}

//-----------------------------------------------------------------------

void GameServerConnection::onConnectionClosed()
{
	ChatServer::fileLog(true, "GameServerConnection", "onConnectionClosed() myID(%u) %p",m_connectionId, this);

	ChatServer::clearGameServerConnection(this);
	ChatServer::instance().unregisterGameServerConnection(m_connectionId);
}

//-----------------------------------------------------------------------

void GameServerConnection::onConnectionOpened()
{
	if(m_connectionId == 0)
	{
		m_connectionId = ChatServer::instance().registerGameServerConnection(this);
		ChatServer::fileLog(true, "GameServerConnection", "onConnectionOpened() new registration myID(%u) %p",m_connectionId, this);
	}
	else
	{
		ChatServer::fileLog(true, "GameServerConnection", "onConnectionOpened() ALREADY REGISTERED myID(%u) %p",m_connectionId, this);
	}
}

//-----------------------------------------------------------------------

void GameServerConnection::onReceive(const Archive::ByteStream & message)
{
	Archive::ReadIterator ri = message.begin();
	GameNetworkMessage gameNetworkMessage(ri);
	ri = message.begin();

	static int count = 0;
	if ((count % 10) == 0)
	{
		ChatServer::getChatInterface()->Process();
	}
	++count;

	//LOG("GameServerConnection", ("onReceive - %s", gameNetworkMessage.getCmdName().c_str()));
	//DEBUG_REPORT_LOG(true, ("***ChatServ: GameServerConnection::onReceive() message(%s)\n", gameNetworkMessage.getCmdName().c_str()));

	if(gameNetworkMessage.isType("ChatDeleteAllPersistentMessages"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatDeleteAllPersistentMessages");
		ChatDeleteAllPersistentMessages chatDeleteAllPersistentMessages(ri);
		ChatServer::deleteAllPersistentMessages(chatDeleteAllPersistentMessages.getSourceNetworkId(), chatDeleteAllPersistentMessages.getTargetNetworkId());
	}
	else if(gameNetworkMessage.isType("ChatCreateRoom"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatCreateRoom");
//printf("GameServerConnection -- ChatCreateRoom\n");
		ChatCreateRoom chat(ri);
		unsigned int sequence = chat.getSequence();
		ChatServer::addGameServerConnection(sequence, this);
		ChatServer::createRoom(NetworkId::cms_invalid, sequence, chat.getRoomName(), chat.getIsModerated(), chat.getIsPublic(), chat.getRoomTitle());
	}
	else if(gameNetworkMessage.isType("ChatRequestLog"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatRequestLog");
		ChatRequestLog chatRequestLog(ri);
		std::vector<ChatLogEntry> chatLog;
		ChatServer::getChatLog(chatRequestLog.getPlayer(), chatLog);

		ChatOnRequestLog chatOnRequestLog(chatRequestLog.getSequence(), chatLog);

		static Archive::ByteStream a;
		a.clear();
		chatOnRequestLog.pack(a);

		(static_cast<Connection *>(this))->send(a, true);
	}
	else if(gameNetworkMessage.isType("ChatDestroyRoomByName"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatDestroyRoomByName");
//printf("GameServerConnection -- ChatDestroyRoomByName\n");
		ChatDestroyRoomByName chat(ri);
		ChatServer::destroyRoom(chat.getRoomPath());
	}
	else if(gameNetworkMessage.isType("ChatPutAvatarInRoom"))
	{
//printf("GameServerConnection -- ChatPutAvatarInRoom\n");
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatPutAvatarInRoom");
		ChatPutAvatarInRoom chat(ri);
		ChatAvatarId id;
		id.cluster = ConfigChatServer::getClusterName();
		id.gameCode = m_gameCode;
		id.name = chat.getAvatarName();
		size_t pos = id.name.find(" ");
		if(pos != std::string::npos)
			id.name = id.name.substr(0, pos);
		ChatServer::enterRoom(id, chat.getRoomName(), chat.getForceCreate(), chat.getCreatePrivate());

	}
	else if(gameNetworkMessage.isType("ChatInviteAvatarToRoom"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatInviteAvatarToRoom");
//printf("GameServerConnection -- ChatInviteAvatarToRoom\n");
		ChatInviteAvatarToRoom chat(ri);
		ChatAvatarId characterName = chat.getAvatarId();
		if(characterName.gameCode == "")
			characterName.gameCode = "SWG";
		if(characterName.cluster == "")
			characterName.cluster = ConfigChatServer::getClusterName();
		ChatServer::invite(NetworkId::cms_invalid, characterName, chat.getRoomName());
	}
	else if(gameNetworkMessage.isType("ChatInviteGroupMembersToRoom"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatInviteGroupMembersToRoom");
//printf("GameServerConnection -- ChatInviteGroupMembersToRoom\n");
		ChatInviteGroupMembersToRoom chat(ri);
		ChatServer::inviteGroupMembers(chat.getInvitorNetworkId(), chat.getGroupLeaderId(), chat.getRoomName(), chat.getInvitedMembers());
	}
	else if(gameNetworkMessage.isType("ChatUninviteFromRoom"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatUninviteAvatarToRoom");
//printf("GameServerConnection -- ChatUninviteAvatarFromRoom\n");
		ChatUninviteFromRoom chat(ri);
		ChatAvatarId characterName = chat.getAvatar();
		if(characterName.gameCode == "")
			characterName.gameCode = "SWG";
		if(characterName.cluster == "")
			characterName.cluster = ConfigChatServer::getClusterName();
		ChatServer::uninvite(ChatServer::getNetworkIdByAvatarId(characterName), chat.getSequence(), characterName, chat.getRoomName());
	}
	else if (gameNetworkMessage.isType("ChatChangeFriendStatus")) 
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatChangeFriendStatus");
//printf("GameServerConnection -- ChatChangeFriendStatus\n");
		ChatChangeFriendStatus chat(ri);

		if (chat.getAdd()) 
		{
			ChatServer::addFriend(
				ChatServer::getNetworkIdByAvatarId(chat.getCharacterName()),
				chat.getSequence(), chat.getFriendName());
		} 
		else 
		{
			ChatServer::removeFriend(
				ChatServer::getNetworkIdByAvatarId(chat.getCharacterName()),
				chat.getSequence(), chat.getFriendName());
		}

	}
	else if (gameNetworkMessage.isType("ChatGetFriendsList"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatGetFriendsList");
//printf("GameServerConnection -- ChatGetFriendsList\n");
		ChatGetFriendsList chat(ri);

		ChatServer::getFriendsList(chat.getCharacterName());
	}
	else if (gameNetworkMessage.isType("ChatChangeIgnoreStatus")) 
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatChangeIgnoreStatus");
//printf("GameServerConnection -- ChatChangeIgnoreStatus\n");
		ChatChangeIgnoreStatus chat(ri);

		if (chat.getIgnore()) 
		{
			ChatServer::addIgnore(
				ChatServer::getNetworkIdByAvatarId(chat.getCharacterName()),
				chat.getSequence(), chat.getIgnoreName());
		} 
		else 
		{
			ChatServer::removeIgnore(
				ChatServer::getNetworkIdByAvatarId(chat.getCharacterName()),
				chat.getSequence(), chat.getIgnoreName());
		}

	}
	else if (gameNetworkMessage.isType("ChatGetIgnoreList"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatGetIgnoreList");
//printf("GameServerConnection -- ChatGetIgnoreList\n");
		ChatGetIgnoreList chat(ri);

		ChatServer::getIgnoreList(chat.getCharacterName());
	}
	else if(gameNetworkMessage.isType("ChatRemoveAvatarFromRoom"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatRemoveAvatarFromRoom");
//printf("GameServerConnection -- ChatRemoveAvatarFromRoom\n");
		ChatRemoveAvatarFromRoom chat(ri);
		ChatAvatarId id = chat.getAvatarId();
		size_t pos = id.name.find(" ");
		if(pos != std::string::npos)
			id.name = id.name.substr(0, pos);
		ChatServer::removeAvatarFromRoom(id, chat.getRoomName());
	}
	else if(gameNetworkMessage.isType("ChatMessageFromGame"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatMessageFromGame");
//printf("GameServerConnection -- ChatMessageFromGame\n");
		ChatMessageFromGame chat(ri);
		std::string recipient = chat.getTo();
		size_t pos = recipient.find(" ");
		if (pos != std::string::npos)
		{
			recipient = recipient.substr(0, pos);
		}
		switch(chat.getMessageType())
		{
		case ChatMessageFromGame::INSTANT:
			{
				ChatAvatarId from(chat.getFrom());
				if (from.cluster.empty())
					from.cluster = ConfigChatServer::getClusterName();
				if (from.gameCode.empty())
					from.gameCode = m_gameCode;

				ChatAvatarId to(recipient);
				if (to.cluster.empty())
					to.cluster = ConfigChatServer::getClusterName();
				if (to.gameCode.empty())
					to.gameCode = m_gameCode;

				ChatServer::sendInstantMessage(from, to, chat.getMessage(), chat.getOutOfBand());
			}
			break;
		case ChatMessageFromGame::PERSISTENT:
			{
				ChatAvatarId from(chat.getFrom());
				if (from.cluster.empty())
					from.cluster = ConfigChatServer::getClusterName();
				if (from.gameCode.empty())
					from.gameCode = m_gameCode;
				
				ChatAvatarId to(recipient);
				if (to.cluster.empty())
					to.cluster = ConfigChatServer::getClusterName();
				if (to.gameCode.empty())
					to.gameCode = m_gameCode;
				
				ChatServer::sendPersistentMessage(from, to, chat.getSubject(), chat.getMessage(), chat.getOutOfBand());
			}
			break;
		case ChatMessageFromGame::ROOM:
			{
				ChatAvatarId from(chat.getFrom());
				if (from.cluster.empty())
					from.cluster = ConfigChatServer::getClusterName();
				if (from.gameCode.empty())
					from.gameCode = m_gameCode;

				ChatServer::sendStandardRoomMessage(from, chat.getRoom(), chat.getMessage(), chat.getOutOfBand());
			}
			break;
		default:
			REPORT_LOG(true, ("Unkown Game Chat Message type\n"));
			break;
		}
	}
	else if (gameNetworkMessage.isType("SetUnsquelchTime"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - SetUnsquelchTime");
//printf("GameServerConnection -- SetUnsquelchTime\n");
		GenericValueTypeMessage<std::pair<NetworkId, int> > setUnsquelchTime(ri);

		ChatServer::setUnsquelchTime(setUnsquelchTime.getValue().first, static_cast<time_t>(setUnsquelchTime.getValue().second));
	}
	else if (gameNetworkMessage.isType("ChatStatisticsGS"))
	{
		PROFILER_AUTO_BLOCK_DEFINE("GameServerConnection - ChatStatisticsGS");
//printf("GameServerConnection -- ChatStatisticsGS\n");
		GenericValueTypeMessage<std::pair<std::pair<std::pair<NetworkId, int>, int>, std::pair<int, int> > > chatStatistics(ri);

		ChatServer::handleChatStatisticsFromGameServer(chatStatistics.getValue().first.first.first, static_cast<time_t>(chatStatistics.getValue().first.first.second), static_cast<time_t>(chatStatistics.getValue().first.second), chatStatistics.getValue().second.first, chatStatistics.getValue().second.second);
	}
	//it eats at my soul to participate in this elseif chain
	else if (gameNetworkMessage.isType(VoiceChatGetChannel::cms_name))
	{
		Archive::ReadIterator ri = message.begin();
		VoiceChatGetChannel createRoomMessage(ri);
		std::list<std::string> mods;
		ChatServer::instance().requestGetChannel(ReturnAddress(m_connectionId),createRoomMessage.getRoomName(), createRoomMessage.getIsPublic(), createRoomMessage.getIsPersistant(), createRoomMessage.getLimit(), mods);
	}
	else if (gameNetworkMessage.isType(VoiceChatDeleteChannel::cms_name))
	{
		Archive::ReadIterator ri = message.begin();
		VoiceChatDeleteChannel destroyRoomMessage(ri);
		ChatServer::instance().requestDeleteChannel(ReturnAddress(m_connectionId),destroyRoomMessage.getRoomName());
	}
	else if (gameNetworkMessage.isType(VoiceChatAddClientToChannel::cms_name))
	{
		Archive::ReadIterator ri = message.begin();
		VoiceChatAddClientToChannel addMessage(ri);
		ChatServer::instance().requestAddClientToChannel(addMessage.getClientId(), addMessage.getClientName(), addMessage.getChannelName(), addMessage.getForceShortlist());
	}
	else if (gameNetworkMessage.isType(VoiceChatRemoveClientFromChannel::cms_name))
	{
		Archive::ReadIterator ri = message.begin();
		VoiceChatRemoveClientFromChannel msg(ri);
		ChatServer::instance().requestRemoveClientFromChannel(msg.getClientId(), msg.getClientName(), msg.getChannelName());
	}
	else if (gameNetworkMessage.isType(VoiceChatChannelCommand::cms_name))
	{
		Archive::ReadIterator ri = message.begin();
		VoiceChatChannelCommand msg(ri);
		ChatServer::instance().requestChannelCommand(ReturnAddress(m_connectionId), msg.getSourceUserName(),msg.getTargetUserName(),msg.getChannelName(),msg.getCommandType(),msg.getBanTimeout());
	}
	else if (gameNetworkMessage.isType("BroadcastGlobalChannel"))
	{
		Archive::ReadIterator ri = message.begin();

		typedef std::pair<std::pair<std::string,std::string>, bool> PayloadType;
		GenericValueTypeMessage<PayloadType> msg(ri);

		PayloadType const & payload = msg.getValue();
		std::string const & channelName = payload.first.first;
		std::string const & messageText = payload.first.second;
		bool const & isRemove = payload.second;

		LOG("CustomerService", ("BroadcastVoiceChannel: ChatServer got BroadcastGlobalChannel on GameServerConnection chan(%s) text(%s) remove(%d)",
			channelName.c_str(), messageText.c_str(), (isRemove?1:0)));
		ChatServer::requestBroadcastChannelMessage(channelName, messageText, isRemove);
	}
	else if (gameNetworkMessage.isType("ChatDestroyAvatar"))
	{
		GenericValueTypeMessage<std::string> const msg(ri);
		ChatServer::getChatInterface()->DestroyAvatar(msg.getValue());
	}
}

//-----------------------------------------------------------------------

