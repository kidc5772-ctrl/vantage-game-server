// Vantage Game Server - Authoritative multiplayer server
// Runs on Oracle Cloud, handles all game logic

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <cmath>
#include <vector>
#include <map>
#include <ctime>
#include <cstring>
#include <iostream>

const int SERVER_PORT = 7777;
const float GRAVITY = 1800.0f;
const int MAX_PLAYERS = 4;
const float TICK_RATE = 50.0f; // 50ms per tick (20 ticks/sec)

enum PacketType {
    PACKET_CONNECT = 1,
    PACKET_DISCONNECT = 2,
    PACKET_INPUT = 3,
    PACKET_GAME_STATE = 4,
    PACKET_PLAYER_JOINED = 5,
    PACKET_PLAYER_LEFT = 6,
    PACKET_CREATE_LOBBY = 7,
    PACKET_JOIN_LOBBY = 8,
    PACKET_LOBBY_UPDATE = 9,
    PACKET_START_GAME = 10
};

struct Vec2 {
    float x, y;
};

struct PlayerInput {
    bool moveLeft;
    bool moveRight;
    bool jump;
    bool shoot;
    float mouseAngle;
    Uint32 timestamp;
};

struct Player {
    Vec2 pos;
    Vec2 vel;
    float size;
    bool onGround;
    bool hasWeapon;
    float shootCooldown;
    bool alive;
    float health;
    bool connected;
    IPaddress address;
    Uint32 lastInputTime;
    char name[32];
    int playerIndex;
};

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    float lifetime;
    int ownerIndex;
};

struct WeaponPickup {
    Vec2 pos;
    float respawnTimer;
    bool active;
};

struct LobbyPlayer {
    char name[32];
    IPaddress address;
    bool ready;
    bool isHost;
};

struct Lobby {
    char code[5];
    std::vector<LobbyPlayer> players;
    bool inGame;
};

// Game state
std::map<int, Player> players; // playerIndex -> Player
std::vector<Bullet> bullets;
std::vector<WeaponPickup> weapons;
std::map<std::string, Lobby> lobbies; // lobbyCode -> Lobby
UDPsocket serverSocket;
UDPpacket* inPacket;
UDPpacket* outPacket;
int nextPlayerIndex = 0;

// Simple tile collision (we'll sync level data from clients)
std::vector<SDL_FRect> platforms;

void initServer() {
    if (SDL_Init(0) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    if (SDLNet_Init() < 0) {
        std::cerr << "SDLNet_Init failed: " << SDLNet_GetError() << std::endl;
        exit(1);
    }
    
    serverSocket = SDLNet_UDP_Open(SERVER_PORT);
    if (!serverSocket) {
        std::cerr << "Failed to open port " << SERVER_PORT << ": " << SDLNet_GetError() << std::endl;
        exit(1);
    }
    
    inPacket = SDLNet_AllocPacket(1024);
    outPacket = SDLNet_AllocPacket(1024);
    
    std::cout << "Server started on port " << SERVER_PORT << std::endl;
}

int findPlayerByAddress(IPaddress addr) {
    for (auto& pair : players) {
        Player& p = pair.second;
        if (p.connected && p.address.host == addr.host && p.address.port == addr.port) {
            return pair.first;
        }
    }
    return -1;
}

void handleConnect(IPaddress addr, const char* playerName) {
    // Check if already connected
    int existing = findPlayerByAddress(addr);
    if (existing != -1) {
        std::cout << "Player already connected: " << playerName << std::endl;
        return;
    }
    
    // Check if room is full
    if (players.size() >= MAX_PLAYERS) {
        std::cout << "Server full, rejecting: " << playerName << std::endl;
        return;
    }
    
    // Add new player
    Player newPlayer;
    newPlayer.pos = {100.0f + nextPlayerIndex * 100.0f, 300.0f};
    newPlayer.vel = {0, 0};
    newPlayer.size = 30;
    newPlayer.onGround = false;
    newPlayer.hasWeapon = false;
    newPlayer.shootCooldown = 0;
    newPlayer.alive = true;
    newPlayer.health = 100.0f;
    newPlayer.connected = true;
    newPlayer.address = addr;
    newPlayer.lastInputTime = SDL_GetTicks();
    newPlayer.playerIndex = nextPlayerIndex;
    strncpy(newPlayer.name, playerName, 31);
    newPlayer.name[31] = '\0';
    
    players[nextPlayerIndex] = newPlayer;
    
    std::cout << "Player connected: " << playerName << " (index " << nextPlayerIndex << ")" << std::endl;
    
    // Send player joined notification to all clients
    outPacket->data[0] = PACKET_PLAYER_JOINED;
    memcpy(outPacket->data + 1, &nextPlayerIndex, sizeof(int));
    memcpy(outPacket->data + 5, playerName, 32);
    outPacket->len = 37;
    
    for (auto& pair : players) {
        if (pair.second.connected) {
            outPacket->address = pair.second.address;
            SDLNet_UDP_Send(serverSocket, -1, outPacket);
        }
    }
    
    nextPlayerIndex++;
}

void handleDisconnect(int playerIndex) {
    if (players.find(playerIndex) == players.end()) return;
    
    std::cout << "Player disconnected: " << players[playerIndex].name << std::endl;
    players[playerIndex].connected = false;
    players.erase(playerIndex);
    
    // Notify all clients
    outPacket->data[0] = PACKET_PLAYER_LEFT;
    memcpy(outPacket->data + 1, &playerIndex, sizeof(int));
    outPacket->len = 5;
    
    for (auto& pair : players) {
        if (pair.second.connected) {
            outPacket->address = pair.second.address;
            SDLNet_UDP_Send(serverSocket, -1, outPacket);
        }
    }
}

void broadcastLobbyUpdate(const std::string& lobbyCode) {
    if (lobbies.find(lobbyCode) == lobbies.end()) return;
    
    Lobby& lobby = lobbies[lobbyCode];
    
    outPacket->data[0] = PACKET_LOBBY_UPDATE;
    outPacket->data[1] = lobby.players.size();
    
    int offset = 2;
    for (const auto& player : lobby.players) {
        memcpy(outPacket->data + offset, player.name, 32);
        offset += 32;
        outPacket->data[offset++] = player.isHost ? 1 : 0;
        outPacket->data[offset++] = player.ready ? 1 : 0;
    }
    
    outPacket->len = offset;
    
    // Send to all players in lobby
    for (const auto& player : lobby.players) {
        outPacket->address = player.address;
        SDLNet_UDP_Send(serverSocket, -1, outPacket);
    }
    
    std::cout << "Broadcasted lobby update for " << lobbyCode << " (" << lobby.players.size() << " players)" << std::endl;
}

void handleCreateLobby(IPaddress addr, const char* lobbyCode, const char* playerName) {
    std::string code(lobbyCode, 4);
    
    // Check if lobby already exists
    if (lobbies.find(code) != lobbies.end()) {
        std::cout << "Lobby " << code << " already exists" << std::endl;
        return;
    }
    
    // Create new lobby
    Lobby newLobby;
    strncpy(newLobby.code, lobbyCode, 4);
    newLobby.code[4] = '\0';
    newLobby.inGame = false;
    
    // Add host player
    LobbyPlayer host;
    strncpy(host.name, playerName, 31);
    host.name[31] = '\0';
    host.address = addr;
    host.ready = true;
    host.isHost = true;
    newLobby.players.push_back(host);
    
    lobbies[code] = newLobby;
    
    std::cout << "Created lobby " << code << " by " << playerName << std::endl;
    
    // Send lobby update to host
    broadcastLobbyUpdate(code);
}

void handleJoinLobby(IPaddress addr, const char* lobbyCode, const char* playerName) {
    std::string code(lobbyCode, 4);
    
    // Check if lobby exists
    if (lobbies.find(code) == lobbies.end()) {
        std::cout << "Lobby " << code << " not found" << std::endl;
        return;
    }
    
    Lobby& lobby = lobbies[code];
    
    // Check if lobby is full
    if (lobby.players.size() >= 4) {
        std::cout << "Lobby " << code << " is full" << std::endl;
        return;
    }
    
    // Check if player already in lobby
    for (const auto& player : lobby.players) {
        if (player.address.host == addr.host && player.address.port == addr.port) {
            std::cout << "Player already in lobby " << code << std::endl;
            return;
        }
    }
    
    // Add player to lobby
    LobbyPlayer newPlayer;
    strncpy(newPlayer.name, playerName, 31);
    newPlayer.name[31] = '\0';
    newPlayer.address = addr;
    newPlayer.ready = true;
    newPlayer.isHost = false;
    lobby.players.push_back(newPlayer);
    
    std::cout << playerName << " joined lobby " << code << std::endl;
    
    // Broadcast update to all players in lobby
    broadcastLobbyUpdate(code);
}

void handleStartGame(const char* lobbyCode) {
    std::string code(lobbyCode, 4);
    
    // Check if lobby exists
    if (lobbies.find(code) == lobbies.end()) {
        std::cout << "Lobby " << code << " not found for start game" << std::endl;
        return;
    }
    
    Lobby& lobby = lobbies[code];
    
    // Check if enough players
    if (lobby.players.size() < 2) {
        std::cout << "Not enough players in lobby " << code << std::endl;
        return;
    }
    
    // Mark lobby as in game
    lobby.inGame = true;
    
    std::cout << "Starting game for lobby " << code << " with " << lobby.players.size() << " players" << std::endl;
    
    // Send start game packet to all players
    outPacket->data[0] = PACKET_START_GAME;
    memcpy(outPacket->data + 1, lobbyCode, 4);
    outPacket->len = 5;
    
    for (const auto& player : lobby.players) {
        outPacket->address = player.address;
        SDLNet_UDP_Send(serverSocket, -1, outPacket);
    }
}

void handlePlayerInput(int playerIndex, PlayerInput input) {
    if (players.find(playerIndex) == players.end()) return;
    
    Player& p = players[playerIndex];
    p.lastInputTime = SDL_GetTicks();
    
    // Store input for processing in game tick
    // For now, we'll process immediately (simplified)
    float dt = TICK_RATE / 1000.0f;
    
    if (!p.alive) return;
    
    // Movement (same as client code)
    float moveSpeed = 650.0f;
    float acceleration = 40000.0f;
    float airAcceleration = 28000.0f;
    float friction = 10000.0f;
    
    float currentAccel = p.onGround ? acceleration : airAcceleration;
    
    if (input.moveLeft) {
        p.vel.x -= currentAccel * dt;
    }
    if (input.moveRight) {
        p.vel.x += currentAccel * dt;
    }
    
    // Friction
    if (p.onGround && !input.moveLeft && !input.moveRight) {
        float frictionForce = friction * dt;
        if (fabs(p.vel.x) < frictionForce) {
            p.vel.x = 0;
        } else {
            p.vel.x -= (p.vel.x > 0 ? frictionForce : -frictionForce);
        }
    } else if (!p.onGround) {
        p.vel.x *= 0.96f;
    }
    
    // Clamp speed
    if (p.vel.x > moveSpeed) p.vel.x = moveSpeed;
    if (p.vel.x < -moveSpeed) p.vel.x = -moveSpeed;
    
    // Gravity
    p.vel.y += GRAVITY * dt;
    
    // Jump
    if (input.jump && p.onGround) {
        p.vel.y = -700.0f;
        p.onGround = false;
    }
    
    // Update position
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;
    
    // Simple ground collision (y > 600 = ground)
    if (p.pos.y > 600) {
        p.pos.y = 600;
        p.vel.y = 0;
        p.onGround = true;
    } else {
        p.onGround = false;
    }
    
    // Screen bounds
    if (p.pos.x < 15) p.pos.x = 15;
    if (p.pos.x > 1185) p.pos.x = 1185;
    
    // Shooting
    if (input.shoot && p.hasWeapon && p.shootCooldown <= 0) {
        // Create bullets
        float gunDist = 25.0f;
        float gunX = p.pos.x + cos(input.mouseAngle) * gunDist;
        float gunY = p.pos.y + sin(input.mouseAngle) * gunDist;
        
        for (int i = 0; i < 5; i++) {
            float angle = input.mouseAngle + (i - 2) * 0.15f;
            Bullet b;
            b.pos = {gunX, gunY};
            b.vel = {cos(angle) * 800.0f, sin(angle) * 800.0f};
            b.lifetime = 2.0f;
            b.ownerIndex = playerIndex;
            bullets.push_back(b);
        }
        
        // Recoil
        p.vel.x -= cos(input.mouseAngle) * 600.0f;
        p.vel.y -= sin(input.mouseAngle) * 600.0f;
        
        p.shootCooldown = 0.5f;
    }
    
    // Update cooldown
    if (p.shootCooldown > 0) {
        p.shootCooldown -= dt;
    }
}

void updateGame(float dt) {
    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->pos.x += it->vel.x * dt;
        it->pos.y += it->vel.y * dt;
        it->lifetime -= dt;
        
        // Check collision with players
        bool hitPlayer = false;
        for (auto& pair : players) {
            Player& p = pair.second;
            if (!p.alive || pair.first == it->ownerIndex) continue;
            
            float dx = p.pos.x - it->pos.x;
            float dy = p.pos.y - it->pos.y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist < p.size / 2) {
                p.health -= 20.0f;
                if (p.health <= 0) {
                    p.alive = false;
                    p.health = 0;
                }
                
                // Knockback
                float angle = atan2(it->vel.y, it->vel.x);
                p.vel.x += cos(angle) * 400.0f;
                p.vel.y += sin(angle) * 400.0f;
                
                hitPlayer = true;
                break;
            }
        }
        
        if (hitPlayer || it->lifetime <= 0 || it->pos.x < 0 || it->pos.x > 1200 || 
            it->pos.y < 0 || it->pos.y > 700) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
    
    // Check for disconnected players (timeout after 5 seconds)
    Uint32 now = SDL_GetTicks();
    for (auto it = players.begin(); it != players.end();) {
        if (it->second.connected && (now - it->second.lastInputTime) > 5000) {
            std::cout << "Player timeout: " << it->second.name << std::endl;
            handleDisconnect(it->first);
            it = players.begin(); // Restart iteration
        } else {
            ++it;
        }
    }
}

void broadcastGameState() {
    // Build game state packet
    outPacket->data[0] = PACKET_GAME_STATE;
    int offset = 1;
    
    // Number of players
    int numPlayers = players.size();
    memcpy(outPacket->data + offset, &numPlayers, sizeof(int));
    offset += sizeof(int);
    
    // Player data
    for (auto& pair : players) {
        Player& p = pair.second;
        memcpy(outPacket->data + offset, &pair.first, sizeof(int)); offset += sizeof(int);
        memcpy(outPacket->data + offset, &p.pos.x, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &p.pos.y, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &p.vel.x, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &p.vel.y, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &p.health, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &p.alive, sizeof(bool)); offset += sizeof(bool);
        memcpy(outPacket->data + offset, &p.hasWeapon, sizeof(bool)); offset += sizeof(bool);
    }
    
    // Number of bullets
    int numBullets = bullets.size();
    memcpy(outPacket->data + offset, &numBullets, sizeof(int));
    offset += sizeof(int);
    
    // Bullet data
    for (auto& b : bullets) {
        memcpy(outPacket->data + offset, &b.pos.x, sizeof(float)); offset += sizeof(float);
        memcpy(outPacket->data + offset, &b.pos.y, sizeof(float)); offset += sizeof(float);
    }
    
    outPacket->len = offset;
    
    // Send to all connected players
    for (auto& pair : players) {
        if (pair.second.connected) {
            outPacket->address = pair.second.address;
            SDLNet_UDP_Send(serverSocket, -1, outPacket);
        }
    }
}

int main(int argc, char* argv[]) {
    initServer();
    
    Uint32 lastTick = SDL_GetTicks();
    bool running = true;
    
    std::cout << "Waiting for players..." << std::endl;
    
    while (running) {
        // Receive packets
        while (SDLNet_UDP_Recv(serverSocket, inPacket) > 0) {
            PacketType type = (PacketType)inPacket->data[0];
            
            switch (type) {
                case PACKET_CONNECT: {
                    char playerName[32];
                    memcpy(playerName, inPacket->data + 1, 32);
                    handleConnect(inPacket->address, playerName);
                    break;
                }
                
                case PACKET_DISCONNECT: {
                    int playerIndex = findPlayerByAddress(inPacket->address);
                    if (playerIndex != -1) {
                        handleDisconnect(playerIndex);
                    }
                    break;
                }
                
                case PACKET_INPUT: {
                    int playerIndex = findPlayerByAddress(inPacket->address);
                    if (playerIndex != -1) {
                        PlayerInput input;
                        int offset = 1;
                        memcpy(&input.moveLeft, inPacket->data + offset, sizeof(bool)); offset += sizeof(bool);
                        memcpy(&input.moveRight, inPacket->data + offset, sizeof(bool)); offset += sizeof(bool);
                        memcpy(&input.jump, inPacket->data + offset, sizeof(bool)); offset += sizeof(bool);
                        memcpy(&input.shoot, inPacket->data + offset, sizeof(bool)); offset += sizeof(bool);
                        memcpy(&input.mouseAngle, inPacket->data + offset, sizeof(float)); offset += sizeof(float);
                        
                        handlePlayerInput(playerIndex, input);
                    }
                    break;
                }
                
                case PACKET_CREATE_LOBBY: {
                    char lobbyCode[5];
                    char playerName[32];
                    memcpy(lobbyCode, inPacket->data + 1, 4);
                    lobbyCode[4] = '\0';
                    memcpy(playerName, inPacket->data + 5, 32);
                    playerName[31] = '\0';
                    handleCreateLobby(inPacket->address, lobbyCode, playerName);
                    break;
                }
                
                case PACKET_JOIN_LOBBY: {
                    char lobbyCode[5];
                    char playerName[32];
                    memcpy(lobbyCode, inPacket->data + 1, 4);
                    lobbyCode[4] = '\0';
                    memcpy(playerName, inPacket->data + 5, 32);
                    playerName[31] = '\0';
                    handleJoinLobby(inPacket->address, lobbyCode, playerName);
                    break;
                }
                
                case PACKET_START_GAME: {
                    char lobbyCode[5];
                    memcpy(lobbyCode, inPacket->data + 1, 4);
                    lobbyCode[4] = '\0';
                    handleStartGame(lobbyCode);
                    break;
                }
                
                default:
                    break;
            }
        }
        
        // Game tick
        Uint32 now = SDL_GetTicks();
        if (now - lastTick >= TICK_RATE) {
            float dt = (now - lastTick) / 1000.0f;
            updateGame(dt);
            broadcastGameState();
            lastTick = now;
        }
        
        SDL_Delay(1); // Don't hog CPU
    }
    
    SDLNet_FreePacket(inPacket);
    SDLNet_FreePacket(outPacket);
    SDLNet_UDP_Close(serverSocket);
    SDLNet_Quit();
    SDL_Quit();
    
    return 0;
}
