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
..\bx\tools\bin\windows\genie --with-examples vs2017
```
3. build glfw
```
cd ../glfw/
cmake .
```
4. For glfw project set Multi-threaded Debug (/MTd)
![project properties](./images/glfw-prop.jpg)
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
4. Build and how execute lobby-server and agent
```
g++ lobby/agent.cpp lobby/protocol.cpp -o lobby/bin/agent -lenet
g++ lobby/lobby-server.cpp lobby/protocol.cpp -o lobby/bin/lobby-server -lenet

./lobby/bin/agent
./lobby/bin/lobby-server
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
./agario/bin/server [port] [aiSize] [minStartRadius] [maxStartRadius] [weightLoss] [speedModif]
./cars/bin/server [port] [forward_accel] [break_accel] [speed_rotation]
```