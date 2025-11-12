# Cars From Hell - Apocalypse Edition

> "Welcome to the Wasteland, driver."
---
it started as a course assignment and ended as a full on chaos. idk, c'est la vie ig.

## Overview

So, you survived. Bad news. The world you remember? It's gone. This is what's left: a scorched, demonic scrapheap where the only thing that matters is the V8 under your hood and the spikes on your grille.

Your car is a beast. Your job is to survive. Good luck—believe me, you'll need it.

## What You're Up Against

### 🔥 The World Hates You
We've built levels, each one worse than the last. You'll also get to enjoy random meteor showers, earthquakes, and lightning storms. Fun, right?

### 🛠️ Evolve or Die
Death is just a pit stop. If you (somehow) survive a level, you'll hit the Upgrade Shop. Spend your blood-money (we call it "score") to beef up your health, acceleration, turning, and those all-important ability cooldowns.

### 💥 Your Bag of Tricks
Your car is more than just a car. It's a weapon. You've got:
- **Shield**: Protection when you need it
- **Blink**: Short-range teleport (costs health!)
- **Shockwave**: Area damage (costs more health!)
- **Fly**: Because your car can fly (my personal favorite)

Use them wisely.

### 👽 The Locals Aren't Friendly
Make sure to not overstay your welcome.

## Controls

### Driving & Combat

| Key | Action |
|-----|--------|
| **↑ / ↓** | Accelerate / Brake |
| **← / →** | Turn Left / Right |
| **Spacebar** | TURBO (fast and free to use) |
| **S** | Shield |
| **D** | Fly (why not F, you ask? Well... it's my game) |
| **B** | Blink (costs 5 HP, don't blink into a wall) |
| **V** | Shockwave (costs 10 HP, clears out the trash) |
| **ESC** | Rage Quit |

### Menus

| Key | Action |
|-----|--------|
| **Enter** | "Yes, I'm ready for more pain" (Continue/Start Level) |
| **1-5** | Buy upgrades in the Upgrade Shop |

## Your "Job"

### Levels 1-14
See that green beacon? Get to it. Take too long, and the game will make you regret it... and dock some points for the trouble. Survive, and you get to go shopping.

### Level 15
...like f I'll tell you about it.

## The Wasteland is Unforgiving...

Is the game too difficult? Is the apocalypse just too apocalyptic?

Perhaps. But the wasteland is full of secrets. The game engine is built on chaos, and chaos can be exploited. There are loopholes, strategies, and "easter eggs" that the developers (that's me) left behind.

If you find yourself truly desperate... look for them.

---

## Requirements

<details>
<summary>Click to expand system requirements and dependencies</summary>

### System Requirements
- OpenGL-capable graphics card
- C++ compiler with C++11 support or later
- Minimum 2GB RAM
- 100MB free disk space

### Dependencies
This game requires the following libraries to compile and run:

- **OpenGL**: Core graphics library
- **GLUT (OpenGL Utility Toolkit)**: Window management and input handling
  - On Linux: `freeglut3-dev`
  - On Windows: FreeGLUT or compatible GLUT implementation
  - On macOS: GLUT framework (included with Xcode)
- **GLU (OpenGL Utility Library)**: Additional OpenGL utilities
- **Standard C Libraries**: `stdlib.h`, `stdio.h`, `time.h`, `stdbool.h`, `math.h`

### Compilation
**Linux/macOS:**
```bash
g++ -o cars_from_hell main.cpp -lGL -lGLU -lglut -lm
```

**Windows (MinGW):**
```bash
g++ -o cars_from_hell.exe main.cpp -lopengl32 -lglu32 -lfreeglut -lm
```

### Running the Game
```bash
./cars_from_hell        # Linux/macOS
cars_from_hell.exe      # Windows
```

</details>

---

## License

This game is licensed under the **Boost Software License - Version 1.0 - August 17th, 2003**

---

## Developer

Created by **Zeyad A. A. Muhammed**

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=flat&logo=linkedin)](https://www.linkedin.com/in/ziad-mohamed-2a956b282)

---

**Good luck out there, driver. You're going to need it.**
