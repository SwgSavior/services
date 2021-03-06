// ChatServerConnection.cpp
// Copyright 2000-02, Sony Online Entertainment Inc., all rights reserved. 
// Author: Justin Randall

//-----------------------------------------------------------------------

#include "FirstConnectionServer.h"
#include "ChatServerConnection.h"
#include "ClientConnection.h"
#include "serverNetworkMessages/GameConnectionServerMessages.h"
#include "sharedNetwork/Service.h"
#include "sharedNetworkMessages/ChatOnChangeFriendStatus.h"
#include "sharedNetworkMessages/ChatOnChangeIgnoreStatus.h"
#include "sharedNetworkMessages/ChatOnEnteredRoom.h"
#include "sharedNetworkMessages/ChatOnGetFriendsList.h"
#include "sharedNetworkMessages/ChatOnGetIgnoreList.h"
#include "sharedNetworkMessages/ChatOnLeaveRoom.h"
#include "sharedNetworkMessages/GenericValueTypeMessage.h"

//-----------------------------------------------------------------------
/*
void putClientsInRoom(const unsigned int roomId, const std::vector<NetworkId> & clients)
{
	std::vector<NetworkId>::const_iterator i;
	for(i = clients.begin(); i != clients.end(); ++i)
	{

		Client* client = ConnectionServer::getClient((*i));
		DEBUG_REPORT_LOG(!client, ("Error, could not map %s to a client\n", (*i).getValueString().c_str()));
		if (client)
			client->enterRoom(roomId);
	}
}
*/

//-----------------------------------------------------------------------
/*
void removeClientsFromRoom(const unsigned int roomId, const std::vector<NetworkId> & clients)
{
	std::vector<NetworkId>::const_iterator i;
	for(i = clients.begin(); i != clients.end(); ++i)
	{

		Client* client = ConnectionServer::getClient((*i));
		DEBUG_REPORT_LOG(!client, ("Error, could not map %s to a client\n", (*i).getValueString().c_str()));
		if (client)
			client->leaveRoom(roomId);
	}
}
*/
//-----------------------------------------------------------------------

ChatServerConnection::ChatServerConnection(UdpConnectionMT * u, TcpClient * t) :
		ServerConnection(u, t),
		clients()
{
}

//-----------------------------------------------------------------------

ChatServerConnection::~ChatServerConnection()
{
}

//-----------------------------------------------------------------------

void ChatServerConnection::addClient(Client * newClient)
{
	if (clients.find(newClient) == clients.end())
		IGNORE_RETURN( clients.insert(newClient) );
	else
		DEBUG_WARNING(true, ("called ChatServerConnection::addClient with a client that already exists in the map."));
}

//-----------------------------------------------------------------------

void ChatServerConnection::onConnectionClosed()
{
	ServerConnection::onConnectionClosed();
	static MessageConnectionCallback m("ChatServerConnectionClosed");
	emitMessage(m);
}

//-----------------------------------------------------------------------

const std::set<Client *> & ChatServerConnection::getClients() const
{
	return clients;
}

//-----------------------------------------------------------------------

void ChatServerConnection::onConnectionOpened()
{
	ServerConnection::onConnectionOpened();
	static MessageConnectionCallback m("ChatServerConnectionOpened");
	emitMessage(m);
}

//-----------------------------------------------------------------------

void ChatServerConnection::onReceive(const Archive::ByteStream & message)
{
	Archive::ReadIterator ri = message.begin();
    GameNetworkMessage m(ri);
    ri = message.begin();

    if (m.isType("GameClientMessage"))
    {
            //we're receiving a message to forward to the client.
            //it is prefixed with NetworkId and reliable.
        const GameClientMessage msg(ri);
		Archive::ReadIterator mri(msg.getByteStream());
		GameNetworkMessage gnm(mri);
		mri = msg.getByteStream().begin();

		// The connection server wants to trap room
		// enter/leave messages to cache clients
		// in particular rooms for autorecovery when
		// a chat server process is stopped then later
		// restarted
		//
		// ************ THIS CODE DOES NOT WORK ***********
		// The following code has been commented out since
		// everyone in the group is sent these messages but
		// the logic assumes that only the person entering
		// or leaving a chat room is sent the message.
		/*
		if(gnm.isType("ChatOnEnteredRoom"))
		{
			Archive::ReadIterator cri(msg.getByteStream());
			ChatOnEnteredRoom chat(cri);
			putClientsInRoom(chat.getRoomId(), msg.getDistributionList());
		}
		else if(gnm.isType("ChatOnLeaveRoom"))
		{
			Archive::ReadIterator cri(msg.getByteStream());
			ChatOnLeaveRoom chat(cri);
			removeClientsFromRoom(chat.getRoomId(), msg.getDistributionList());
		}
		*/
		
		const std::vector<NetworkId> & d = msg.getDistributionList();
		std::vector<NetworkId>::const_iterator i;
		for(i = d.begin(); i != d.end(); ++i)
		{
			if ((*i) == NetworkId::cms_invalid)
			{
				//broadcast to everyone
				Service *service = ConnectionServer::getClientServicePrivate();
				LogicalPacket const * p = service->createPacket(msg.getByteStream().getBuffer(), msg.getByteStream().getSize());
				const ConnectionServer::ClientMap clientMap(ConnectionServer::getClientMap());
				ConnectionServer::ClientMap::const_iterator i;
				for(i = clientMap.begin(); i != clientMap.end(); ++i)
				{
					Client* client = (*i).second;
					if (client)
					{
						client->getClientConnection()->sendSharedPacket(p, msg.getReliable());				
					}
				}
				service->releasePacket(p);
				break;
			}

			Client* client = ConnectionServer::getClient((*i));
			DEBUG_REPORT_LOG(!client, ("Error, could not map %s to a client\n", (*i).getValueString().c_str()));
			if (client)
			{
				if (!gnm.isType("ChatStatisticsCS"))
					client->getClientConnection()->sendByteStream(msg.getByteStream(), msg.getReliable());

				GameConnection *gc = client->getGameConnection();
				if (gc) 
				{
					if (gnm.isType("ChatOnGetFriendsList")) 
					{
						Archive::ReadIterator cri(msg.getByteStream());
						ChatOnGetFriendsList c(cri);
						gc->send(c, true);
						//DEBUG_REPORT_LOG(true, ("Sending a ChatOnGetFriendsList msg to game server\n"));
					}
					else if (gnm.isType("ChatOnChangeIgnoreStatus")) 
					{
						Archive::ReadIterator cri(msg.getByteStream());
						ChatOnChangeIgnoreStatus c(cri);
						gc->send(c, true);
						//DEBUG_REPORT_LOG(true, ("Sending a ChatOnChangeIgnoreStatus msg to game server\n"));
					}
					else if (gnm.isType("ChatOnChangeFriendStatus")) 
					{
						Archive::ReadIterator cri(msg.getByteStream());
						ChatOnChangeFriendStatus c(cri);
						gc->send(c, true);
						//DEBUG_REPORT_LOG(true, ("Sending a ChatOnChangeFriendStatus msg to game server\n"));
					}
					else if (gnm.isType("ChatOnGetIgnoreList")) 
					{
						Archive::ReadIterator cri(msg.getByteStream());
						ChatOnGetIgnoreList c(cri);
						gc->send(c, true);
						//DEBUG_REPORT_LOG(true, ("Sending a ChatOnGetIgnoreList msg to game server\n"));
					}
					else if (gnm.isType("ChatStatisticsCS"))
					{
						Archive::ReadIterator cri(msg.getByteStream());
						GenericValueTypeMessage<std::pair<std::pair<NetworkId, int>, std::pair<int, int> > > chatStatistics(cri);
						gc->send(chatStatistics, true);
						//DEBUG_REPORT_LOG(true, ("Sending a ChatStatisticsCS msg to game server\n"));
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------

void ChatServerConnection::removeClient(Client * oldClient)
{
	std::set<Client *>::iterator f = clients.find(oldClient);
	if(f != clients.end())
		clients.erase(f);
}

//-----------------------------------------------------------------------


