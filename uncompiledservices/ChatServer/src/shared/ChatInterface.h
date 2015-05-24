
// ChatInterface.h
// Copyright 2000-02, Sony Online Entertainment Inc., all rights reserved. 
// Author: Justin Randall

#ifndef	_INCLUDED_ChatInterface_H
#define	_INCLUDED_ChatInterface_H

//-----------------------------------------------------------------------

#include "ChatAPI/ChatAPI.h"
#include "ChatServerAvatarOwner.h"
#include "ChatServerRoomOwner.h"
#include <deque>
#include <hash_map>
#include <map>
#include <set>
#include <string>
#include "sharedNetworkMessages/ChatRoomData.h"
#include "sharedFoundation/NetworkId.h"

class ChatPersistentMessageToClient;
class ConnectionServerConnection;
class GameNetworkMessage;

namespace Archive 
{
	class ByteStream;
}

void makeAvatarId(const ChatAvatar & avatar, ChatAvatarId & id);

using namespace ChatSystem;

class ChatInterface : public ChatAPI
{
public:
	ChatInterface  (const std::string & strGameCode,
					const std::string & strGatewayServerIP,
					const unsigned short sGatewayServerPort,
					const std::string & registrarHost,
					const unsigned short registrarPort
					);

	virtual ~ChatInterface  (); //lint !e1510 base class 'ChatAPI' has no destructor // jrandall - this is beyond my control

	const NetworkId & getNetworkIdByAvatarId(const ChatAvatarId &id);
	ChatServerAvatarOwner *getAvatarOwner(const ChatAvatar *avatar);
	void          disconnectPlayer  (const ChatAvatarId & avatarId);
	const std::hash_map<std::string, ChatServerRoomOwner> &  getRoomList  () const;

	const ChatServerRoomOwner *getRoomOwner(const std::string &roomName);
	ChatServerRoomOwner *getRoomOwner(unsigned roomId);
	const ChatServerRoomOwner *  getRoomByName       (const std::string & roomName) const;

	void          checkQueuedLogins();

	void          sendQueuedHeadersToClient();
	void          clearQueuedHeadersForAvatar(const ChatAvatarId & avatarId);
	void          addQueuedHeaderForAvatar(const ChatAvatarId & avatarId, const ChatPersistentMessageToClient * header, unsigned long sendTime);

	void          sendMessageToAvatar  (const ChatAvatar & avatar, const GameNetworkMessage & message);
	void          sendMessageToAvatar  (const ChatAvatarId & avatar, const GameNetworkMessage & message);
	void          requestRoomList   (const NetworkId & id, ConnectionServerConnection * connection);
	void deferChatMessageFor  (const NetworkId & id, const Archive::ByteStream & bs);
	void          sendMessageToAllAvatars  (const GameNetworkMessage & message);
	bool          sendMessageToPendingAvatar(const ChatAvatarId &id, const GameNetworkMessage &message);

	std::string  getChatName(const NetworkId &id);

	void          queryRoom         (const NetworkId & id, ConnectionServerConnection * connection, const unsigned int sequenceId, const std::string & roomName);
	void          updateRoomForThisChatAPI(const ChatRoom *room, const ChatAvatar *additionalAvatar);
	void          updateRooms();

	void          ConnectPlayer(const unsigned int suid, const std::string & characterName, const NetworkId & networkId);
	void          DestroyAvatar(const std::string & characterName);

	virtual void OnConnect();
	virtual void OnDisconnect();

	virtual void OnFailoverBegin();
	virtual void OnFailoverComplete();

	virtual void OnSendRoomMessage(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveRoomMessage(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom, const ChatUnicodeString &msg, const ChatUnicodeString &oob, unsigned messageID);

	virtual void OnAddFriend(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatUnicodeString &destName, const ChatUnicodeString &destAddress, void *user);
	virtual void OnRemoveFriend(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatUnicodeString &destName, const ChatUnicodeString &destAddress, void *user);
	virtual void OnFriendStatus(unsigned track, unsigned result, const ChatAvatar *srcAvatar, unsigned listLength, const ChatFriendStatus *friendList, void *user);
	virtual void OnReceiveFriendLogin(const ChatAvatar *srcAvatar, const ChatUnicodeString &srcAddress, const ChatAvatar *destAvatar);
	virtual void OnReceiveFriendLogout(const ChatAvatar *srcAvatar, const ChatUnicodeString &srcAddress, const ChatAvatar *destAvatar);

	virtual void OnAddIgnore(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatUnicodeString &destName, const ChatUnicodeString &destAddress, void *user);
	virtual void OnRemoveIgnore(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatUnicodeString &destName, const ChatUnicodeString &destAddress, void *user);
	virtual void OnIgnoreStatus(unsigned track, unsigned result, const ChatAvatar *srcAvatar, unsigned listLength, const ChatIgnoreStatus *ignoreList, void *user);

	virtual void OnLoginAvatar(unsigned track, unsigned result, const ChatAvatar *newAvatar, void *user);
	virtual void OnLogoutAvatar(unsigned track, unsigned result, const ChatAvatar *oldAvatar, void *user);
	virtual void OnDestroyAvatar(unsigned track, unsigned result, const ChatAvatar *oldAvatar, void *user);
	virtual void OnGetAnyAvatar(unsigned track, unsigned result, const ChatAvatar *foundAvatar, bool loggedIn, void *user);

	virtual void OnCreateRoom(unsigned track,unsigned result, const ChatRoom *newRoom, void *user);
	virtual void OnDestroyRoom(unsigned track, unsigned result, void *user);
	virtual void OnReceiveDestroyRoom(const ChatAvatar *srcAvatar, const ChatRoom *destRoom);

	virtual void OnEnterRoom(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveEnterRoom(const ChatAvatar *srcAvatar, const ChatRoom *destRoom);

	virtual void OnAddInvite(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveAddInviteRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveAddInviteAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnRemoveInvite(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveRemoveInviteRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveRemoveInviteAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnLeaveRoom(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveLeaveRoom(const ChatAvatar *srcAvatar, const ChatRoom *destRoom);

	virtual void OnKickAvatar(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveKickRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveKickAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnGetPersistentMessage(unsigned track, unsigned result, ChatAvatar *destAvatar, const PersistentHeader *header, const ChatUnicodeString &msg, const ChatUnicodeString &oob, void *user);
	virtual void OnSendInstantMessage(unsigned track, unsigned result, const ChatAvatar *srcAvatar, void *user);
	virtual void OnReceiveInstantMessage(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &msg, const ChatUnicodeString &oob);

	virtual void OnSendPersistentMessage(unsigned track, unsigned result, const ChatAvatar *srcAvatar, void *user);
	virtual void OnGetRoomSummaries(unsigned track, unsigned result, unsigned numFoundRooms, RoomSummary *foundRooms, void *user);
	virtual void OnGetRoom(unsigned track, unsigned result, const ChatRoom *room, void *user);

	virtual void OnAddModerator(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveAddModeratorRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveAddModeratorAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnRemoveModerator(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveRemoveModeratorRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveRemoveModeratorAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnGetPersistentHeaders(unsigned track, unsigned result, ChatAvatar *destAvatar, unsigned listLength, const PersistentHeader *list, void *user);

	virtual void OnReceiveForcedLogout(const ChatAvatar *oldAvatar);

	virtual void OnReceivePersistentMessage(const ChatAvatar *destAvatar, const PersistentHeader *header);

	virtual void OnAddBan(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveAddBanRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveAddBanAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnRemoveBan(unsigned track, unsigned result, const ChatAvatar *srcAvatar, const ChatRoom *destRoom, void *user);
	virtual void OnReceiveRemoveBanRoom(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatRoom *destRoom);
	virtual void OnReceiveRemoveBanAvatar(const ChatAvatar *srcAvatar, const ChatAvatar *destAvatar, const ChatUnicodeString &roomName, const ChatUnicodeString &roomAddress);

	virtual void OnUpdatePersistentMessages(unsigned track, unsigned result, const ChatAvatar *targetAvatar, void *user);
	virtual void OnReceiveUnregisterRoomReady(const ChatRoom *destRoom);
	virtual void OnTransferAvatar(unsigned track, unsigned result, unsigned oldUserID, unsigned newUserID, const ChatUnicodeString &oldName, const ChatUnicodeString &newName, const ChatUnicodeString &oldAddress, const ChatUnicodeString &newAddress, void *user);

private:
	std::map<ChatAvatarId, ChatServerAvatarOwner *>  avatarMap;
	std::hash_map<std::string, NetworkId>  pendingAvatars;
	std::set<NetworkId> pendingRoomQueries;
	int                 roomQueriesThisFrame;
	std::hash_map<std::string, ChatServerRoomOwner> roomList;
	std::hash_map<NetworkId, std::vector<Archive::ByteStream>, NetworkId::Hash >  deferredChatMessages;
	std::map<ChatAvatarId, std::pair<unsigned long, std::deque<const ChatPersistentMessageToClient *> > > queuedHeaders;
	std::map<unsigned, std::pair<std::pair<ChatUnicodeString, ChatUnicodeString>, int> > trackingRequestGetAnyAvatarForDestroy;
};

#if 0

//-----------------------------------------------------------------------

class ChatInterface : public ChatAPI
{
public:
	ChatInterface  (const std::string & strGameCode,
	                const std::string & strGatewayServerIP,
	                const unsigned short sGatewayServerPort,
	                const std::string & strBackupGatewayServerIP,
	                const unsigned short sBackupGatewayServerPort
					);

	virtual ~ChatInterface  (); //lint !e1510 base class 'ChatAPI' has no destructor // jrandall - this is beyond my control


	void checkRoomExpirations ();
	void ConnectPlayer        (const unsigned int suid, const std::string & characterName, const NetworkId & networkId);
	void deferChatMessageFor  (const NetworkId & id, const Archive::ByteStream & bs);
	void destroyRoomByName    (const std::string & roomName);

	const std::hash_map<std::string, ChatServerRoomOwner> &  getRoomList  () const;
	const ChatServerAvatarOwner *findAvatar(const ChatAvatarId & avatarId);

	void leaveRoom            (const ChatAvatarId & avatarId, const std::string & roomName);

	//--- ChatAPI virtual overrides
	virtual void  OnConnectAvatar                    (const std::string & track, const unsigned int resultCode, const ChatAvatar & avatar);
	virtual void  OnAddFriend                        (const unsigned int sequence, const std::string & track, const unsigned int nResultCode, const ChatAvatar & requestingAvatar, const ChatAvatar & friendAvatar);
	virtual void  OnRemoveFriend                        (const unsigned int sequence, const std::string & track, const unsigned int nResultCode, const ChatAvatar & requestingAvatar, const ChatAvatar & friendAvatar);
	virtual void  OnAddIgnore                        (const unsigned int sequence, const std::string & track, const unsigned int nResultCode, const ChatAvatar & requestingAvatar, const ChatAvatar & ignoreAvatar);
	virtual void  OnRemoveIgnore                        (const unsigned int sequence, const std::string & track, const unsigned int nResultCode, const ChatAvatar & requestingAvatar, const ChatAvatar & ignoreAvatar);
	virtual void  OnAddRoomModerator                 (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatRoom & room, const unsigned int nRequestingAvatarUID, const ChatAvatar & moderator);
	virtual void  OnCreateRoom                       (const unsigned int sequence, const std::string & track,  const unsigned int nResultCode,  const ChatRoom & room);
	virtual void  OnDestroyRoom                      (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatRoom & room, const std::string & strCharacterName, const std::string & strGameServerName, const std::string & strGameCode);
	virtual void  OnDisconnectAvatar                 (const std::string & track, const unsigned int nResultCode, const ChatAvatar & toAvatar);
	virtual void  OnDisconnectAvatarNotify           (const ChatAvatar & toAvatar);
	virtual void  OnEnterRoom                        (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatRoom & room, const ChatAvatar & avatar);
	virtual void  OnLeaveRoom                        (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatRoom & room, const ChatAvatar & avatar);
	virtual void  OnReceiveInstantMessage            (const ChatAvatar & toAvatar,  const ChatAvatar & fromAvatar,  const Unicode::String & ustrMessage, const Unicode::String & oob);
	virtual void  OnReceivePersistentMessage         (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatPersistentMessage & message);
    virtual void  OnReceivePersistentMessageHeader   (const ChatPersistentMessageHeader & header);
	virtual void  OnReceivePersistentMessageHeaders  (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatAvatar & toAvatar, const std::vector<ChatPersistentMessageHeader> & headers);
	virtual void  OnReceiveRoomMessage               (const unsigned int nSequenceID, const ChatRoom & room, const std::string & strFromCharacterName, const std::string & strFromGameServerName, const std::string & strFromGameCode, const Unicode::String & ustrMessage, const Unicode::String &  outOfBand, unsigned messageID);
	virtual void  OnConnect                          (const unsigned int resultCode);
	virtual void  OnReceiveRooms                     (const std::string & track, const unsigned int nResultCode, const std::vector<ChatRoom*> & rooms);
	virtual void  OnRemoveRoomModerator              (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatRoom & room, const unsigned int nRequestingAvatarUID, const ChatAvatar & moderator);
	virtual void  OnSendInstantMessage               (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatAvatar & sendingAvatar);
	virtual void  OnSendPersistentMessage            (const unsigned int nSequenceID, const std::string & track, const unsigned int nResultCode, const ChatAvatar& sendingAvatar);  
	virtual void  OnSendRoomMessage                  (const unsigned int sequence, const std::string & track, const unsigned int resultCode);
	virtual void  OnUpdateFriendStatus               (const ChatAvatar & toAvatar, const ChatAvatar & fromAvatar, const bool bIsConnected);
	virtual void  OnReceiveRoomInvitation            (const std::string & strRoomName, const ChatAvatar & invitorAvatar, const ChatAvatar & inviteeAvatar);
	virtual void  OnSendRoomInvitation               (const unsigned int sequence, const std::string & track, const unsigned int resultCode, const ChatRoom & room, const bool isOnline, const ChatAvatar & invitor, const ChatAvatar & invitee);

	const ChatServerRoomOwner *  getRoomByName       (const std::string & roomName) const;

	void          disconnectPlayer  (const ChatAvatarId & avatarId);
	void          queryRoom         (const NetworkId & id, ConnectionServerConnection * connection, const unsigned int sequenceId, const std::string & roomName);
	void          queryRoom         (const NetworkId & id, ConnectionServerConnection * connection, const unsigned int sequenceId, const unsigned int roomId);
	void          requestRoomList   (const NetworkId & id, ConnectionServerConnection * connection);
	void          sendMessageToAllAvatars  (const GameNetworkMessage & message);
	bool          sendMessageToPendingAvatar(const ChatAvatarId &id, const GameNetworkMessage &message);

private:
	void          sendMessageToAvatar  (const ChatAvatar & avatar, const GameNetworkMessage & message);
	void          sendMessageToAvatar  (const ChatAvatarId & avatar, const GameNetworkMessage & message);

private:
	ChatInterface & operator = (const ChatInterface & rhs);
	ChatInterface(const ChatInterface & source);
	std::hash_map<std::string, NetworkId>  pendingAvatars;
	std::hash_map<NetworkId, std::vector<Archive::ByteStream>, NetworkId::Hash >  deferredChatMessages;
	std::hash_map<std::string, ChatServerRoomOwner> roomList;
	std::map<ChatAvatarId, ChatServerAvatarOwner *>  avatarMap;
	time_t lastRoomCheck;

}; //lint !e1712 default constructor not defined for class

#endif

std::string toLower(const std::string & source);
std::string toUpper(const std::string & source);
void makeRoomData(const ChatRoom & room, ChatRoomData & roomData);
void makeRoomData(const RoomSummary & room, ChatRoomData & roomData);

//-----------------------------------------------------------------------

#endif	// _INCLUDED_ChatInterface_H
