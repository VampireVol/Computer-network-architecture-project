[Readme in Russian](./readme_ru.md)

# Description
This is client-server project for game-servers
Based on bgfx, glfw, imgui for rendering and enet for networking
The project contains two client-server games, a lobby client, a lobby server and an agent for running game servers on a dedicated server.
- The first agario-like game controls an orb that can absorb the orbs of other players or bots that are smaller than it. Inputs are sent to the server from the players, and there are movements on it and packets with new positions are sent.
- The second game about the ability to control the car in the form of capsules. The main functionality implemented in this game:
-- All simulation takes place on the server, all data is transmitted as snapshots
-- Implemented interpolation between snapshots in case of packet loss on the client
-- Implemented local simulation for client-managed objects and rerolling physics when there are differences between received values from the server
- Lobby client - an application for creating and placing rooms that can be created for two games. With it, the player can receive information from the lobby server about the created rooms, who connected and connected to the session that has already started, each room has its own set of attributes depending on the type of game.

Video demonstration you can see here: https://disk.yandex.ru/i/QbswJCAiPKrv7A

### List of modifiers for servers

- For agario
-- aiSize - number of bots in the game 0-32
-- minStartRadius - minimum size of spheres at start 0.0-3.0
-- maxStartRadius - maximum size of spheres at start 0.0-3.0
-- weightLoss - weight loss of the entity eaten by another entity 0.0-1.0
-- speedModif - speed modifier 0.1-10.0
- For cars
-- forward_accel - forward acceleration 0.0-30.0
-- break_accel - reverse acceleration 0.0-30.0
-- speed_rotation - rotation speed 0.0-1.0

## Architecture
On each dedicated servers deploy Agent, they can launch game servers when lobby-server send signal for creation. All clients firstly connected to the lobby-server, where they can send info about game rooms, then one player press 'start' lobby-server send messeage to Agent and they send info about connection. The agent creates a game server and then sends connection information. This information is then sent from the lobby server to the lobby client. The lobby client launches the client of the selected game with the selected options. Next, the game client contacts the game server and you can play!
![Architecture](./images/architecture.png)


# How build clinets with Visual Studio
Requerements for clients is Visual Studio 2017+ and Cmake
1. Clone 3rdParty submodules
```
git submodules init
git submodules update
```
2. build bgfx
```
cd 3rdParty/bgfx/
../bx/tools/bin/windows/genie --with-examples vs2017
```
3. build glfw
```
cd ../glfw/
cmake .
```
4. For glfw project set Multi-threaded Debug (/MTd)
![project properties](./images/glfw-prop.png)
5. Download [Enet 1.3.17](http://enet.bespin.org/Downloads.html) and copy folder to 3rdParty
6. Use .sln for build clients for lobby\agario\cars

# How build servers in Ubuntu or WSL
1. Download and unpack [Enet 1.3.17](http://enet.bespin.org/Downloads.html)
2. Install lib with command:
```
./configure && make && sudo make install
```
3. Make dirs for builds
```
mkdir lobby/bin
mkdir agario/bin
mkdir cars/bin
```
4. Build and how execute lobby-server and agent. Also create in /lobby/bin/ create ServersList.txt if you push agent to dedicated server
```
g++ lobby/agent.cpp lobby/protocol.cpp -o lobby/bin/agent -lenet
g++ lobby/lobby-server.cpp lobby/protocol.cpp -o lobby/bin/lobby-server -lenet

./lobby/bin/agent
(cd lobby/bin/ && ./lobby-server)
```
5. Build and how execute server for agario and cars
```
g++ -std=c++17 agario/server.cpp agario/protocol.cpp -o agario/bin/server -lenet
g++ cars/server.cpp cars/protocol.cpp cars/entity.cpp -o cars/bin/server -lenet

./agario/bin/server
./cars/bin/server
```
Also you can execute servers with params
```
./agario/bin/server [hostName] [port] [aiSize] [minStartRadius] [maxStartRadius] [weightLoss] [speedModif]
./cars/bin/server [hostName] [port] [forward_accel] [break_accel] [speed_rotation]
```
In lobby/bin/ServersList.txt you can set host names for agents deployed on dedicated servers.
For lobby-client near with lobby-client.exe in LobbyHostName.txt you can put host name to lobby-server. In default used localhost. 