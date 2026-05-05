# Snake Game
## Project Description
A fully playable console Snake Game written in standard C++17. The snake moves around a bordered grid, eating food to grow longer and score points. The game ends when the snake collides with a wall, obstacle, or itself. The difficulty increases automatically as the player scores more points.

---

## Features
- [x] Snake movement (W/A/S/D keys)
- [x] Food spawning (random empty cell selection)
- [x] Snake growth on food collection
- [x] Collision detection — walls, self, obstacles
- [x] Scoring system (10 × level per food eaten)
- [x] Levels with increasing speed (level up every 5 foods)
- [x] Obstacle tiles appear at level 3 and above
- [x] Pause / Resume (P key)
- [x] Persistent high score saved to `data/highscore.txt`
- [x] Restart without exiting (R at Game Over screen)

---

## Data Structures Used and Justification

### 1. `std::deque<Position>` — Snake Body
The snake body is stored as a **double-ended queue (deque)**.

| Operation | Complexity | Why |
|-----------|-----------|-----|
| Add new head (`push_front`) | **O(1)** | deque provides O(1) access at both ends |
| Remove old tail (`pop_back`) | **O(1)** | same reason |
| Self-collision check | O(n) | linear scan through all segments |

**Why not vector?** `push_front` on a vector is O(n) because all elements must shift.  
**Why not linked list?** A singly-linked list has no O(1) `pop_back`; you need a doubly-linked list (essentially what deque provides, with better cache locality).

### 2. `std::vector<std::vector<Cell>>` — Board Grid (2-D Array)
The board stores the game state as a **2-D grid** indexed `grid_[row][col]`.

| Operation | Complexity |
|-----------|-----------|
| Read/write any cell | **O(1)** |
| Full redraw per frame | O(W × H) = O(600) — constant for fixed board |
| Wall collision check | **O(1)** |

**Why not a flat 1-D array?** Same asymptotic complexity, but 2-D indexing is clearer and less error-prone for students.

### 3. `std::vector<Position>` — Food Candidate List (inside `Food::spawn`)
When spawning food we collect all empty cells into a temporary vector, then pick one randomly.

| Operation | Complexity |
|-----------|-----------|
| Build candidate list | O(W × H) |
| Random selection | **O(1)** |

**Why not loop until we find an empty cell?** That could loop forever on a nearly-full board.

---

## How to Compile and Run

### Linux / macOS
```bash
# Requires g++ with C++17 support
make
./snake_game
```

### Windows (MinGW / MSYS2)
```bash
g++ -std=c++17 -Wall -I include/ -o snake_game.exe src/*.cpp
snake_game.exe
```

### Windows (Visual Studio)
1. Create a new Empty C++ Project.
2. Add all files from `src/` and `include/`.
3. Set C++ Standard to C++17 in Project Properties.
4. Build and run.

---

## Example Output
```
 Score:    50   Level: 2   High Score:   80

##############################
#                            #
#   ooo                      #
#   O                        #
#              *             #
#                            #
##############################

 Controls: W/A/S/D  |  P = Pause  |  Q = Quit
```

---

## AI Usage Declaration

| Tool | Used For | Outcome |
|------|----------|---------|
| Claude (Anthropic) | Initial architecture brainstorming & boilerplate generation | Used as a starting point; restructured class relationships ourselves |
| Claude (Anthropic) | Explaining `std::deque` trade-offs vs. vector/list | Helped us understand and justify our DSA choice |
| Claude (Anthropic) | Generating cross-platform input handling snippet (conio.h / termios) | We tested it, found the arrow-key prefix byte was wrong on our machines, and fixed the `\xe0` prefix case ourselves |
| Claude (Anthropic) | Suggesting test cases for boundary collisions | Some suggested cases used, others we wrote from scratch |

**What we modified or rejected:** The AI initially suggested using a raw C array for the board. We replaced it with `vector<vector<Cell>>` because it is safer (no manual memory management) and more idiomatic C++17.

**Example where AI output was not fully correct:** The original AI-generated `getKey()` used `key == 224` as an integer comparison, which triggered a signed/unsigned comparison warning and behaved incorrectly on some compilers. We changed the condition to compare with the correct hex literal `'\xe0'` and added a cast.

**What we understood and implemented ourselves:** The complete game loop logic, the level-up threshold formula, the obstacle spawning system, and all collision detection logic were designed and written by our group.

---

## Project Structure
```
snake_game/
├── include/
│   ├── Common.h         — shared types, constants, Position alias
│   ├── Snake.h          — deque-based snake class
│   ├── Food.h           — food spawning
│   ├── Board.h          — 2-D grid + renderer
│   ├── ScoreManager.h   — file-based high score
│   └── Game.h           — main game engine
├── src/
│   ├── main.cpp         — entry point
│   ├── Snake.cpp
│   ├── Food.cpp
│   ├── Board.cpp
│   ├── ScoreManager.cpp
│   └── Game.cpp
├── data/
│   └── highscore.txt    — auto-created; stores single integer
├── Makefile
└── README.md
```
