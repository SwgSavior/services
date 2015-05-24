// CentralServerConnection.cpp
// copyright 2000 Verant Interactive
// Author: Justin Randall


//-----------------------------------------------------------------------

#include "FirstLoginServer.h"
#include "Archive/ByteStream.h"
#include "sharedFoundation/NetworkIdArchive.h"
#include "CentralServerConnection.h"
#include "ConsoleManager.h"
#include "CSToolConnection.h"
#include "ClientConnection.h"
#include "DatabaseConnection.h"
#include "serverNetworkMessages/CSToolResponse.h"
#include "serverNetworkMessages/LoginClusterName.h"
#include "serverNetworkMessages/LoginClusterName2.h"
#include "serverNetworkMessages/ToggleAvatarLoginStatus.h"
#include "serverNetworkMessages/TransferAccountData.h"
#include "serverNetworkMessages/TransferAccountDataArchive.h"
#include "serverNetworkMessages/TransferCharacterData.h"
#include "serverNetworkMessages/TransferCharacterDataArchive.h"
#include "serverNetworkMessages/TransferReplyCharacterList.h"
#include "sharedLog/Log.h"
#include "sharedNetwork/NetworkSetupData.h"
#include "sharedNetworkMessages/ConsoleChannelMessages.h"
#include "sharedNetworkMessages/DeleteCharacterMessage.h"
#include "sharedNetworkMessages/GenericValueTypeMessage.h"
#include "Unicode.h"
#include "UnicodeUtils.h"

//-----------------------------------------------------------------------

namespace CentralServerConnectionNamespace
{
	std::map<std::string, CentralServerConnection *>  s_centralConnectionsByName;
}

using namespace CentralServerConnectionNamespace;

//-----------------------------------------------------------------------

CentralServerConnection::CentralServerConnection(UdpConnectionMT * u, TcpClient * t) :
		ServerConnection(u, t),
		m_clusterName(),
		m_networkVersion(),
		m_clusterId(0)
{
}

// ----------------------------------------------------------------------

CentralServerConnection::CentralServerConnection(
	const std::string & address,
	const unsigned short port,
	const std::string &clusterName,
	const uint32 clusterId) :
		ServerConnection(address,port, NetworkSetupData()),
		m_clusterName( clusterName ),
		m_networkVersion(),
		m_clusterId( clusterId )
{
	s_centralConnectionsByName[m_clusterName] = this;
}

//-----------------------------------------------------------------------

CentralServerConnection::~CentralServerConnection()
{	
	if(! m_clusterName.empty())
	{
		std::map<std::string, CentralServerConnection *>::iterator f = s_centralConnectionsByName.find(m_clusterName);
		if(f != s_centralConnectionsByName.end())
		{
			s_centralConnectionsByName.erase(f);
		}
		
		LOG("CentralServerConnection", ("Galaxy [%s] disconnected", m_clusterName.c_str()));
	}
	else
	{
		LOG("CentralServerConnection", ("Galaxy connection destroyed (never fully connected)"));
	}
}

//-----------------------------------------------------------------------

void CentralServerConnection::onConnectionOpened()
{
	ServerConnection::onConnectionOpened();
}

//-----------------------------------------------------------------------

void CentralServerConnection::onReceive(const Archive::ByteStream & message)
{
	// emit the message in case someone else wants to use it
	ServerConnection::onReceive(message);

	Archive::ReadIterator ri = message.begin();
	GameNetworkMessage m(ri);
	ri = message.begin();
	// handle messages the connection object itself may be interested in
	if(m.isType("LoginClusterName"))
	{
		const LoginClusterName c(ri);
		ri = message.begin();
		setClusterName(c.getClusterName());
		LOG("CentralServerConnection", ("Galaxy [%s] connected", c.getClusterName().c_str()));
	}
	else if ( m.isType( "LoginClusterName2" ) )
	{
		const LoginClusterName2 c( ri );
		ri = message.begin();
		
		const std::string &clusterName    = c.getClusterName();
		const std::string &branch         = c.getBranch();
		const int         changelist      = c.getChangelist();
		const std::string &networkVersion = c.getNetworkVersion();
		
		DEBUG_REPORT_LOG( true, ( "!!!!!!!!!!!!!!!! name=%s branch=%s changelist=%d net=%s\n", clusterName.c_str(), branch.c_str(), changelist, networkVersion.c_str() ) );
		
		setClusterName( clusterName  );
		setNetworkVersion( c.getNetworkVersion() );
		LOG("CentralServerConnection", ("Galaxy [%s] connected", getClusterName().c_str()));
		
		LoginServer::getInstance().setClusterInfoByName( clusterName, branch, changelist, networkVersion );
		
	}
	else if(m.isType("ToggleAvatarLoginStatus"))
	{
		const ToggleAvatarLoginStatus t(ri);
		if(t.getEnabled())
		{
			LOG("CustomerService", ("CharacterTransfer: ToggleAvatarLoginStatus(%s, %d, %s, true)", t.getClusterName().c_str(), t.getStationId(), t.getCharacterId().getValueString().c_str()));
		}
		else
		{
			// send message to the cluster to drop connected clients for the
			// station id in case the avatar being disabled is currently logged in
			GenericValueTypeMessage<unsigned int> const closeRequest("TransferCloseClientConnection", t.getStationId());
			LoginServer::getInstance().sendToCluster(LoginServer::getInstance().getClusterIDByName(t.getClusterName()), closeRequest);

			LOG("CustomerService", ("CharacterTransfer: ToggleAvatarLoginStatus(%s, %d, %s, false)\n", t.getClusterName().c_str(), t.getStationId(), t.getCharacterId().getValueString().c_str()));
		}
		DatabaseConnection::getInstance().toggleDisableCharacter(LoginServer::getInstance().getClusterIDByName(t.getClusterName()), t.getCharacterId(), t.getStationId(), t.getEnabled());
	}
	else if(m.isType("CtsCompletedForcharacter"))
	{
		const GenericValueTypeMessage<std::pair<std::string, NetworkId> > msg(ri);
		LoginServer::getInstance().sendToCluster(LoginServer::getInstance().getClusterIDByName(msg.getValue().first), msg);
	}
	else if(m.isType("TransferRequestCharacterList"))
	{
		const GenericValueTypeMessage<TransferCharacterData> request(ri);
		const TransferCharacterData & d = request.getValue();
		DatabaseConnection::getInstance().requestAvatarListForAccount(d.getSourceStationId(), &d);
	}
	else if(m.isType("TransferReplyLoginLocationData"))
	{
		const GenericValueTypeMessage<TransferCharacterData> reply(ri);
		const TransferCharacterData & d = reply.getValue();
		LOG("CustomerService", ("CharacterTransfer: Received login location data. %s", d.toString().c_str()));
	}
	else if(m.isType("TransferGetCharacterDataFromLoginServer"))
	{
		// the TransferServer sent a request to the central server
		// to retrieve a character ID from the login database given
		// a source station ID and a source character name. Retrieve
		// this data from the login database.
		const GenericValueTypeMessage<TransferCharacterData> request(ri);
		TransferCharacterData d = request.getValue();
		DatabaseConnection::getInstance().requestAvatarListForAccount(d.getSourceStationId(), &d);
		LOG("CustomerService", ("CharacterTransfer: Received TransferGetCharactetrDataFromLoginServer from CentralServer. %s", d.toString().c_str()));
	}
	else if(m.isType("TransferRenameCharacterInLoginDatabase"))
	{
		const GenericValueTypeMessage<TransferCharacterData> request(ri);
		LOG("CustomerService", ("CharacterTransfer: Received TransferRenameCharacterInLoginDatabase : %s", request.getValue().toString().c_str()));
		const TransferCharacterData & requestData = request.getValue();
		DatabaseConnection::getInstance().renameCharacter(getClusterId(), requestData.getCharacterId(), Unicode::narrowToWide(requestData.getDestinationCharacterName()), &requestData);
	}
	else if(m.isType("TransferKickConnectedClients"))
	{
		const GenericValueTypeMessage<unsigned int> kick(ri);
		ClientConnection * clientConnection = LoginServer::getInstance().getValidatedClient(kick.getValue());
		if(! clientConnection)
		{
			clientConnection = LoginServer::getInstance().getUnvalidatedClient(kick.getValue());
		}

		if(clientConnection)
		{
			clientConnection->disconnect();
		}
	}
	else if(m.isType("TransferAccountRequestLoginServer"))
	{
		const GenericValueTypeMessage<TransferAccountData> request(ri);
		LOG("CustomerService", ("CharacterTransfer: Received TransferAccountRequestLoginServer from station ID %d to from station ID %d", request.getValue().getSourceStationId(), request.getValue().getDestinationStationId()));
		const TransferAccountData requestData = request.getValue();
		DatabaseConnection::getInstance().requestAvatarListAccountTransfer(&requestData);
	}
	else if(m.isType("EnableCharacterMessage"))
	{
		const GenericValueTypeMessage<std::pair<std::pair<StationId, NetworkId>, std::string> > msg(ri);

		LOG("LoginServer", ("EnableCharacter %d, %s request from %s\n", msg.getValue().first.first, msg.getValue().first.second.getValueString().c_str(), msg.getValue().second.c_str()));

		DatabaseConnection::getInstance().enableCharacter(msg.getValue().first.first, msg.getValue().first.second, msg.getValue().second, true, m_clusterId);
	}
	else if(m.isType("DisableCharacterMessage"))
	{
		const GenericValueTypeMessage<std::pair<std::pair<StationId, NetworkId>, std::string> > msg(ri);

		LOG("LoginServer", ("DisableCharacter %d, %s request from %s\n", msg.getValue().first.first, msg.getValue().first.second.getValueString().c_str(), msg.getValue().second.c_str()));

		DatabaseConnection::getInstance().enableCharacter(msg.getValue().first.first, msg.getValue().first.second, msg.getValue().second, false, m_clusterId);
	}
	else if(m.isType("DeleteFailedTransfer"))
	{
		GenericValueTypeMessage<TransferCharacterData> deleteCharacter(ri);
		LOG("CustomerService", ("CharacterTransfer: LoginServer received request to delete a character for a failed transfer. %s", deleteCharacter.getValue().toString().c_str()));
		LoginServer::getInstance().deleteCharacter(m_clusterId, deleteCharacter.getValue().getDestinationCharacterId(), deleteCharacter.getValue().getDestinationStationId());
	}
	else if(m.isType("RequestTransferClosePseudoClientConnection"))
	{
		GenericValueTypeMessage<std::pair<std::string, unsigned int> > const request(ri);
		GenericValueTypeMessage<unsigned int> const closeRequest("TransferClosePseudoClientConnection", request.getValue().second);
		LoginServer::getInstance().sendToCluster(LoginServer::getInstance().getClusterIDByName(request.getValue().first), closeRequest);
	}
	else if(m.isType("CSToolResponse" ) )
	{
		CSToolResponse response(ri);
		// find the connection, if it still exists
		CSToolConnection * con = CSToolConnection::getCSToolConnectionByToolId( response.getToolId() );
		
		// send the response
		if( con )
		{
			std::string message;
			message = m_clusterName + ":" + response.getResult();
			
			if( message[ message.length() -1 ] == '\n' && message[ message.length() - 2 ] != '\r' )
			{
				message[ message.length() - 1 ] = '\r';
				message += '\n';
			}
			
			con->sendToTool( message );
		}
	}
	else if(m.isType("ConGenericMessage"))
	{
		ConGenericMessage con(ri);
		parseCommand(con.getMsg(), con.getMsgId());
	}
	else if(m.isType("LoginToggleCompletedTutorial"))
	{
		GenericValueTypeMessage< std::pair<unsigned int, bool> > const request(ri);
		std::pair<unsigned int, bool> values = request.getValue();
		DatabaseConnection::getInstance().toggleCompletedTutorial(values.first, values.second);
	}
	
	else if(m.isType("AllCluserGlobalChannel"))
	{
		typedef std::pair<std::pair<std::string,std::string>, bool> PayloadType;
		GenericValueTypeMessage<PayloadType> msg(ri);

		PayloadType const & payload = msg.getValue();
		std::string const & channelName = payload.first.first;
		std::string const & messageText = payload.first.second;
		bool const & isRemove = payload.second;

		LOG("CustomerService", ("BroadcastVoiceChannel: LoginServer sending AllCluserGlobalChannel to all clusters chan(%s) text(%s) remove(%d)",
			channelName.c_str(), messageText.c_str(), (isRemove?1:0) ));

		LoginServer::getInstance().sendToAllClusters(msg);
	}

	else if(m.isType("GcwScoreStatRaw"))
	{
		GenericValueTypeMessage<std::pair<std::string, std::pair<std::map<std::string, std::pair<int64, int64> >, std::map<std::string, std::pair<int64, int64> > > > > const msg(ri);
		LoginServer::getInstance().sendToAllClusters(msg, NULL, 0, msg.getValue().first.c_str());
	}

	else if(m.isType("GcwScoreStatPct"))
	{
		GenericValueTypeMessage<std::pair<std::string, std::pair<std::map<std::string, int>, std::map<std::string, int> > > > const msg(ri);
		LoginServer::getInstance().sendToAllClusters(msg, NULL, 0, msg.getValue().first.c_str());
	}
}

//-----------------------------------------------------------------------

bool CentralServerConnection::sendCharacterListResponse(unsigned int stationId, const AvatarList & avatarList, const TransferCharacterData & characterData)
{
	bool result = false;
	// find proper central connection
	std::map<std::string, CentralServerConnection *>::iterator f = s_centralConnectionsByName.find(characterData.getSourceGalaxy());
	if(f != s_centralConnectionsByName.end())
	{
		// cull characters not associated with this server
		CentralServerConnection * connection = f->second;
		AvatarList::const_iterator i;
		AvatarList replyCharacters;
		for(i = avatarList.begin(); i != avatarList.end(); ++i)
		{
			if(i->m_clusterId == connection->m_clusterId)
				replyCharacters.push_back(*i);
		}
		TransferReplyCharacterList reply(characterData.getTrack(), stationId, replyCharacters);
		connection->send(reply, true);
		result = true;
	}
	else
	{
		LOG("Connection", ("Could not find a connection server to send character list response to for character %d\n", stationId));
	}
	return result;
}

//-----------------------------------------------------------------------

void CentralServerConnection::sendToCentralServer(const std::string & galaxyName, const GameNetworkMessage & message)
{
	std::map<std::string, CentralServerConnection *>::iterator f = s_centralConnectionsByName.find(galaxyName);
	if(f != s_centralConnectionsByName.end())
	{
		CentralServerConnection * connection = f->second;
		connection->send(message, true);
	}
}

//-----------------------------------------------------------------------

void CentralServerConnection::setClusterName(const std::string & newClusterName)
{
		DEBUG_REPORT_LOG( true, ( "setClusterName %s\n", newClusterName.c_str() ) );
		
	m_clusterName = newClusterName;
	s_centralConnectionsByName[m_clusterName] = this;
}

//-----------------------------------------------------------------------

void CentralServerConnection::setClusterId(uint32 clusterId)
{
	m_clusterId = clusterId;
}

//-----------------------------------------------------------------------

void CentralServerConnection::parseCommand(const std::string & cmd, int track)
{
//	int i = s_track;
//	s_resultsMap[i] = this;
//	++s_track;
	std::string result;
	/*CommandParser::ErrorType err =*/ ConsoleManager::processString(cmd, track, result);
	ConGenericMessage con(result, track);
	send(con, true);
/*	if(err == CommandParser::ERR_CMD_NOT_FOUND)
	{
		onCommandComplete(result, track);
	}
	else if(!result.empty() && err == CommandParser::ERR_SUCCESS)
	{
		onCommandComplete(result, track);
	}
*/
}

//-----------------------------------------------------------------------
