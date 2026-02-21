# Vantage - Multiplayer Fighting Game

A fast-paced 2D multiplayer fighting game with square characters, weapons, and dynamic levels.

## Features

- 🎮 2-4 player online multiplayer
- 🔫 Shotgun combat with recoil physics
- 🗺️ Built-in level editor
- 🎯 Dynamic weapon spawning
- 💀 Spike traps and hazards
- 🏆 Last player standing wins
- 🎨 Custom level support (JSON format)

## Game Modes

- **Play Mode:** Fight against other players online
- **Level Editor:** Create custom maps with tiles, spikes, spawn points, and decorations
- **Local Testing:** Test your levels before sharing

## Controls

### Gameplay:
- **A/D or Arrow Keys:** Move left/right
- **Space/W:** Jump
- **Mouse:** Aim
- **Left Click:** Shoot (when you have a weapon)
- **R:** Respawn
- **ESC:** Pause menu

### Level Editor:
- **1:** Floor tile
- **2:** Spike
- **3:** Spawn point
- **4:** Wall (decoration)
- **5:** Chain (decoration)
- **6:** Spike + Wall combo
- **7:** Spawn + Wall combo
- **0:** Erase
- **S:** Save
- **Shift+S:** Save as
- **L:** Load level
- **C:** Clear level
- **E:** Play mode
- **T:** Toggle test mode

## Multiplayer Server

The game uses a dedicated authoritative server for fair gameplay.

### Server Hosting (Render.com - Free):
See [RENDER_SETUP.md](RENDER_SETUP.md) for deployment instructions.

### Server Features:
- Authoritative game simulation (anti-cheat)
- 20 tick rate (50ms updates)
- UDP networking for low latency
- Automatic player timeout detection
- Supports 2-4 simultaneous players

## Building

### Client (Windows):
```bash
g++ main.cpp -o game.exe -IC:/mingw64/include -L. -LC:/mingw64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lcomdlg32
```

### Server (Linux):
```bash
g++ game_server.cpp -o game_server -lSDL2 -lSDL2_net -lpthread
```

## Dependencies

- SDL2
- SDL2_image
- SDL2_net (for multiplayer)

## File Structure

```
├── main.cpp              # Game client
├── game_server.cpp       # Multiplayer server
├── level.json            # Default level
├── level2-5.json         # Additional levels
├── chain.png             # Chain decoration sprite
├── Dockerfile            # For Render.com deployment
├── render.yaml           # Render.com configuration
└── README.md             # This file
```

## Level Format

Levels are stored as JSON:
```json
{
  "width": 30,
  "height": 18,
  "tiles": [
    [0, 0, 1, 1, 0, ...],
    ...
  ]
}
```

Tile types:
- 0: Empty
- 1: Floor
- 2: Spike
- 3: Spawn point
- 4: Wall (decoration)
- 5: Chain (decoration)
- 6: Spike + Wall
- 7: Spawn + Wall

## Gameplay Mechanics

- **Health:** 100 HP per player
- **Damage:** 20 HP per bullet (5 pellets per shot)
- **Weapon Spawning:** First weapon at 1 second, then every 10 seconds
- **Max Weapons:** 10 active weapons on map
- **Respawn:** Random spawn point after death
- **Level Transition:** Winner announced, next level loads after 3 seconds

## Credits

Built with SDL2 and love ❤️

## License

MIT License - Feel free to modify and share!
