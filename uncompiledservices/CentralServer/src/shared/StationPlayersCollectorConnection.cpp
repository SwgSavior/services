// StationPlayersCollectorConnection.cpp
// copyright 2005 Sony Online Entertainment
// Author: Doug Mellencamp


//-----------------------------------------------------------------------

#include "FirstCentralServer.h"
#include "ConfigCentralServer.h"
#include "ConnectionServerConnection.h"
#include "ConsoleConnection.h"
#include "sharedLog/Log.h"
#include "sharedNetwork/NetworkSetupData.h"
#include "sharedNetworkMessages/ConsoleChannelMessages.h"
#include "sharedNetworkMessages/GenericValueTypeMessage.h"
#include "StationPlayersCollectorConnection.h"

//-----------------------------------------------------------------------

StationPlayersCollectorConnection::StationPlayersCollectorConnection(const std::string & a, const unsigned short p) :
ServerConnection(a, p, NetworkSetupData())
{
}

//-----------------------------------------------------------------------

StationPlayersCollectorConnection::~StationPlayersCollectorConnection()
{
}

//-----------------------------------------------------------------------

void StationPlayersCollectorConnection::onConnectionClosed()
{
	static MessageConnectionCallback m("StationPlayersCollectorConnectionClosed");
	emitMessage(m);
	DEBUG_REPORT_LOG(true, ("[CentralServer] : StationPlayersCollectorConnection::onConnectionClosed().\n"));
}

//-----------------------------------------------------------------------

void StationPlayersCollectorConnection::onConnectionOpened()
{
	LOG("StationPlayersCollectorConnection", ("Connection with the StationPlayersCollector server is open, sending CentralGalaxyName message(%s)", ConfigCentralServer::getClusterName()));
	static MessageConnectionCallback m("StationPlayersCollectorConnectionOpened");
	emitMessage(m);
	DEBUG_REPORT_LOG(true, ("[CentralServer] : StationPlayersCollectorConnection::onConnectionOpened().\n"));
}

//-----------------------------------------------------------------------

void StationPlayersCollectorConnection::onReceive(const Archive::ByteStream & message)
{
	Archive::ReadIterator ri = message.begin();
	const GameNetworkMessage msg(ri);
	ri = message.begin();

	if(msg.isType("OnSPCharacterProfileMessage"))
	{
	}
	else if(msg.isType("ConGenericMessage"))
	{
		ConGenericMessage con(ri);
		ConsoleConnection::onCommandComplete(con.getMsg(), static_cast<int>(con.getMsgId()));	 
	}
}

//-----------------------------------------------------------------------
