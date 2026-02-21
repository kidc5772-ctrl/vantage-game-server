# Vantage Multiplayer Implementation Plan

## Architecture Overview

```
┌─────────────┐         ┌──────────────────┐         ┌─────────────┐
│   Client 1  │◄───────►│  Oracle Cloud    │◄───────►│   Client 2  │
│  (game.exe) │   UDP   │  Game Server     │   UDP   │  (game.exe) │
└─────────────┘         │  (game_server)   │         └─────────────┘
      │                 └──────────────────┘               │
      │                          │                         │
      └──────────────────────────┼─────────────────────────┘
                                 │
                          ┌──────▼──────┐
                          │  Supabase   │
                          │   (Lobby)   │
                          └─────────────┘
```

## What We're Building

### 1. Game Server (game_server.cpp) ✅ CREATED
- Runs on Oracle Cloud (free forever)
- Authoritative game simulation
- Handles 2-4 players
- 20 ticks/second (50ms)
- UDP networking for low latency

### 2. Game Client (main.cpp) - TO MODIFY
- Connect to server via UDP
- Send player inputs
- Receive and render game state
- Predict own movement (client-side prediction)

### 3. Lobby System (Supabase) - OPTIONAL
- Create/join rooms
- Player list
- Server IP distribution
- Stats and leaderboards

## Current Status

✅ Server code created (game_server.cpp)
✅ Oracle Cloud setup guide created
✅ Build scripts created
⏳ Waiting for Oracle Cloud account approval
❌ Client networking code (next step)
❌ SDL2_net installation (needed)

## Next Steps

### Immediate (While Waiting for Oracle):

1. **Install SDL2_net library**
   - Download: https://github.com/libsdl-org/SDL_net/releases
   - Extract SDL2_net.dll to game folder
   - Copy headers to C:/mingw64/include/SDL2/

2. **Test server locally**
   ```bash
   build_server.bat
   game_server.exe
   ```

3. **Modify game client** (main.cpp)
   - Add UDP networking
   - Connect to server
   - Send inputs instead of processing locally
   - Receive and render server game state

### After Oracle Cloud Approval:

4. **Deploy to Oracle Cloud**
   - Follow ORACLE_CLOUD_SETUP.md
   - Upload game_server.cpp
   - Build and run on VM

5. **Test with friends**
   - Share server IP
   - Connect multiple clients
   - Test gameplay

6. **Add Supabase lobby** (optional)
   - Room creation/joining
   - Player matchmaking
   - Stats tracking

## Files Created

- `game_server.cpp` - Authoritative game server
- `ORACLE_CLOUD_SETUP.md` - Complete deployment guide
- `build_server.bat` - Windows build script
- `MULTIPLAYER_PLAN.md` - This file
- `supabase.h` / `supabase.cpp` - Lobby system (optional)
- `json.hpp` - JSON helper (for Supabase)

## Network Protocol

### Client → Server:
- `PACKET_CONNECT` - Join game with player name
- `PACKET_DISCONNECT` - Leave game
- `PACKET_INPUT` - Movement/shooting inputs (sent every frame)

### Server → Client:
- `PACKET_GAME_STATE` - Full game state (20 times/sec)
  - All player positions, velocities, health
  - All bullet positions
  - Weapon pickups
- `PACKET_PLAYER_JOINED` - New player notification
- `PACKET_PLAYER_LEFT` - Player disconnect notification

## Performance Targets

- **Latency:** 20-50ms (depends on distance to server)
- **Tick Rate:** 20 ticks/second (50ms per tick)
- **Bandwidth:** ~5-10 KB/sec per player
- **Players:** 2-4 simultaneous

## Cost

- **Oracle Cloud:** $0/month (free tier forever)
- **Supabase:** $0/month (free tier, 500MB database)
- **Total:** $0/month 🎉

## What You Need to Do Now

1. **Sign up for Oracle Cloud** (if not done)
   - https://www.oracle.com/cloud/free/
   - Wait for approval

2. **Download SDL2_net**
   - https://github.com/libsdl-org/SDL_net/releases
   - Get SDL2_net-2.2.0-win32-x64.zip
   - Extract SDL2_net.dll to your game folder

3. **Let me know when ready** and I'll:
   - Modify your game client for multiplayer
   - Help deploy to Oracle Cloud
   - Test with you

## Questions?

Ask me anything about:
- Oracle Cloud setup
- Networking code
- Server deployment
- Testing and debugging
