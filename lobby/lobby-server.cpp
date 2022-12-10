#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include "protocol.h"

const int AGENT_PORT = 10007;
const int LOBBY_SERVER_PORT = 10887;
const int MAX_PEERS = 32;

static uint16_t curRoomId = 1;
static uint16_t curUserId = 1;
static uint16_t curAgentIdx = 0;

static std::vector<User> users;
static std::vector<Room> rooms;
static std::vector<AgarSettings> agarSettings;
static std::vector<CarsSettings> carsSettings;
static std::vector<ENetPeer*> agents;
static Room emptyR;
static User emptyU;
static AgarSettings emptyA;
static CarsSettings emptyC;

struct UserInfo
{
  uint16_t id;
};

uint16_t get_user_id()
{
  return curUserId++;
}

uint16_t get_room_id()
{
  return curRoomId++;
}

User& find_user(uint16_t id)
{
  for (auto &user : users)
  {
    if (user.id == id)
      return user;
  }
  printf("Can't find user gg");
  return emptyU;
}

Room& find_room(uint16_t id)
{
  for (auto &room : rooms)
  {
    if (room.id == id)
      return room;
  }
  printf("Can't find room gg");
  return emptyR;
}

const AgarSettings& find_agar_settings(uint16_t id)
{
  for (const auto &settings : agarSettings)
  {
    if (settings.id == id)
      return settings;
  }
  printf("Can't find agar settings gg");
  return emptyA;
}

const CarsSettings& find_cars_settings(uint16_t id)
{
  for (const auto &settings : carsSettings)
  {
    if (settings.id == id)
      return settings;
  }
  printf("Can't find cars settings gg");
  return emptyC;
}

void send_user_list(uint16_t roomId, ENetPeer *peers, ENetPeer *sender)
{
  printf("try send list\n");
  std::vector<User> sendUsers;
  for (const auto &user : users)
  {
    if (user.roomId == roomId)
      sendUsers.push_back(user);
  }
  printf("send users list, size: %ld", sendUsers.size());
  if (sender)
  {
    auto room = find_room(roomId);
    if (room.type == 0)
    {
      send_room_info_agar(sender, room, find_agar_settings(roomId), sendUsers);
    }
    else
    {
      send_room_info_cars(sender, room, find_cars_settings(roomId), sendUsers);
    }
  }

  std::vector<ENetPeer *> connectedPeers;
  for (int i = 0; i < MAX_PEERS; ++i)
  {
    if (peers[i].state == ENET_PEER_STATE_CONNECTED)
    {
      connectedPeers.push_back(&peers[i]);
    }
  }
  for (const auto &user : sendUsers)
  {
    for (int i = 0; i < connectedPeers.size(); ++i)
    {
      const UserInfo *data = (UserInfo *) connectedPeers[i]->data;
      if (!data)
        continue;
      if (sender)
      {
        const UserInfo *dataSender = (UserInfo *) sender->data;
        if (user.id == data->id && dataSender->id != data->id)
          send_users_list(connectedPeers[i], sendUsers);
      }
      else
        if (user.id == data->id)
          send_users_list(connectedPeers[i], sendUsers);
    }
  }
}

void send_connection_info(ENetPeer *peers, const ConnectionInfo &info, uint16_t roomId)
{
  std::vector<ENetPeer *> connectedPeers;
  for (int i = 0; i < MAX_PEERS; ++i)
  {
    if (peers[i].state == ENET_PEER_STATE_CONNECTED)
    {
      connectedPeers.push_back(&peers[i]);
      UserInfo *data = (UserInfo *) peers[i].data;
      if (data)
        printf("sending port to: Peer %d, id %d\n", i, data->id);
    }
  }
  for (const auto &peer : connectedPeers)
  {
    const UserInfo *data = (UserInfo *) peer->data;
    if (!data)
      continue;
    User &user = find_user(data->id);
    if (user.roomId == roomId)
      send_server_connection_info(peer, info);
  }
}

void message_receive(const ENetEvent &event, ENetPeer *peers)
{
  printf("catch!\n");
  switch (get_packet_type(event.packet))
  {
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_LOGIN:
    {
      User user;
      deserialize_login(event.packet, user);
      user.id = get_user_id();
      users.push_back(user);
      event.peer->data = new UserInfo({ user.id });
      send_user_id(event.peer, user.id);
      for (int i = 0; i < MAX_PEERS; ++i)
      {
        if (peers[i].state == ENET_PEER_STATE_CONNECTED)
        {
          UserInfo *data = (UserInfo *) peers[i].data;
          if (data)
            printf("Peer %d, id %d\n", i, data->id);
        }
      }
      break;
    }
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_JOIN_ROOM:
    {
      uint16_t roomId;
      uint16_t userId;
      deserialize_join(event.packet, userId, roomId);
      auto &user = find_user(userId);
      user.roomId = roomId;
      auto &room = find_room(roomId);
      room.curPlayers++;
      if (room.running == 1)
      {
        if (room.type == 0)
        {
          send_room_info_agar(event.peer, room, find_agar_settings(roomId), {});
        }
        else
        {
          send_room_info_cars(event.peer, room, find_cars_settings(roomId), {});
        }
        send_server_connection_info(event.peer, room.serverInfo);
      }
      else
      {
        send_user_list(roomId, peers, event.peer);
      }
      break;
    }
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_LEAVE_ROOM:
    {
      uint16_t roomId;
      uint16_t userId;
      deserialize_leave(event.packet, userId, roomId);
      auto &user = find_user(userId);
      user.roomId = 0;
      auto &room = find_room(roomId);
      room.curPlayers--;
      if (!room.running)
      {
        send_user_list(roomId, peers, nullptr);
      }
      break;
    }
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_GET_ROOMS_LIST:
      send_rooms_list(event.peer, rooms);
      break;
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_START:
    {
      uint16_t roomId;
      deserialize_start(event.packet, roomId);
      //TODO chose next agent
      auto &room = find_room(roomId);
      if (room.type == 0)
      {
        auto settings = find_agar_settings(roomId);
        send_agario_server_settings(agents[curAgentIdx], settings);
      }
      else
      {
        auto settings = find_cars_settings(roomId);
        send_cars_server_settings(agents[curAgentIdx], settings);
      }
      ++curAgentIdx;
      if (curAgentIdx == agents.size())
        curAgentIdx = 0;
      break;
    }
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_CREATE_AGAR_ROOM:
    {
      Room room;
      AgarSettings settings;
      deserialize_create_agar_room(event.packet, room, settings);
      room.id = get_room_id();
      const UserInfo *data = (UserInfo *) event.peer->data;
      User &user = find_user(data->id);
      user.roomId = room.id;
      settings.id = room.id;
      room.curPlayers++;
      rooms.push_back(room);
      agarSettings.push_back(settings);
      send_room_id(event.peer, room.id);
      send_user_list(room.id, peers, event.peer);
      break;
    }
    case E_LOBBY_CLIENT_TO_LOBBY_SERVER_CREATE_CARS_ROOM:
    {
      Room room;
      CarsSettings settings;
      deserialize_create_cars_room(event.packet, room, settings);
      room.id = get_room_id();
      const UserInfo *data = (UserInfo *) event.peer->data;
      User &user = find_user(data->id);
      user.roomId = room.id;
      settings.id = room.id;
      room.curPlayers++;
      rooms.push_back(room);
      carsSettings.push_back(settings);
      send_room_id(event.peer, room.id);
      send_user_list(room.id, peers, event.peer);
      break;
    }
    case E_AGENT_TO_LOBBY_SERVER_SERVER_CREATED:
    {
      ServerInfo info;
      char hostName[32] {""};
      enet_address_get_host(&event.peer->address, hostName, 32);
      deserialize_server_created(event.packet, info);
      Room &room = find_room(info.id);
      room.running = 1;
      strcpy(room.serverInfo.hostName, hostName);
      room.serverInfo.port = info.port;
      printf("server created id: %d port: %d\n", info.id, info.port);
      send_connection_info(peers, room.serverInfo, info.id);
    }
  }
}

int main(int argc, const char **argv)
{
  printf("Lobby-server started\n");
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = LOBBY_SERVER_PORT;

  ENetHost *server = enet_host_create(&address, MAX_PEERS, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  std::fstream in("ServersList.txt", std::fstream::in);
  std::string agentString;
  if (in.is_open())
  {
    while (getline(in, agentString))
    {
      ENetAddress agentAddress;
      enet_address_set_host(&agentAddress, agentString.c_str());
      agentAddress.port = AGENT_PORT;
      ENetPeer *agent = enet_host_connect(server, &agentAddress, 2, 0);
      agents.push_back(agent);
      printf("put agent to list with host name %s\n", agentString.c_str());
    }
  }
  in.close();

  if (agents.empty())
  {
    printf("Can't open ServerList.txt or they empty. Set try connect to agent with default localhost");
    ENetAddress agentAddress;
    enet_address_set_host(&agentAddress, "localhost");
    agentAddress.port = AGENT_PORT;
    ENetPeer *agent = enet_host_connect(server, &agentAddress, 2, 0);
    agents.push_back(agent);
  }

  if (!agents[0])
  {
    printf("Cannot connect to agent\n");
    return 1;
  }

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        message_receive(event, server->peers);
        printf("Packet received '%s'\n", event.packet->data);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        event.peer->data = nullptr;
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

