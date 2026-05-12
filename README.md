# Snake Game

## Group Members
1. Amr Ahmed Nagy - 2300162
2. Ali Ahmed Ayman - 2300346
3. Wesam Ayman AbdElfattah - 2300627
4. Omar Ahmed Mohamed - 2300164
5. Ziad Moried Mohamed - 2300135

---

## Project Description
A fully playable console and GUI Snake Game written in standard C++17 utilizing the Qt6 framework. The snake moves around a bordered grid, eating food to grow longer and score points. The game ends when the snake collides with an obstacle or itself. The difficulty increases automatically as the player scores more points, introducing obstacles and faster game speeds.

---

## Features
- [x] Dual interface: Playable via Command Line or a Neon Arcade Graphical User Interface (Qt6).
- [x] Snake movement (W/A/S/D keys) with wall-wrapping capabilities.
- [x] Food spawning (random empty cell selection).
- [x] Snake growth on food collection.
- [x] Collision detection — self and obstacles.
- [x] Scoring system (10 × level per food eaten, plus 50 × level for Bonus fruits).
- [x] Levels with increasing speed (level up every 5 foods).
- [x] Dynamic Obstacles that spawn upon leveling up.
- [x] Power-ups: Shield (protects against one obstacle) and Bonus Fruit (temporary high-score boost).
- [x] Sprint mechanic (Hold Shift to boost speed).
- [x] Pause / Resume (P key).
- [x] Persistent high score saved to `data/highscore.txt`.

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
| Obstacle collision check | **O(1)** |

**Why not a flat 1-D array?** Same asymptotic complexity, but 2-D indexing is clearer and highly efficient for tracking spatial coordinates.

### 3. `std::vector<Position>` — Dynamic Spawning (inside `Food::spawn`, `Shield`, `Bonus`)
When spawning items, we collect all empty cells into a temporary vector, then pick one randomly.

| Operation | Complexity |
|-----------|-----------|
| Build candidate list | O(W × H) |
| Random selection | **O(1)** |

**Why not loop until we find an empty cell?** That could loop forever on a nearly-full board and cause massive lag spikes.

---

## How to Compile and Run

### CMake (Recommended for Qt6 GUI)
```bash
mkdir build
cd build
cmake ..
cmake --build .
./SnakeGame

```

---

## AI Usage Declaration

| Tool | Used For | Outcome |
| --- | --- | --- |
| Claude (Anthropic) | Initial architecture brainstorming & boilerplate generation | Used as a starting point; restructured class relationships ourselves |
| Gemini (Google) | Refactoring GUI logic | Used to decouple complex Qt6 UI elements and build a clean State Machine. |
| Claude (Anthropic) | Explaining `std::deque` trade-offs vs. vector/list | Helped us understand and justify our DSA choice |

**What we modified or rejected:** The AI initially suggested using a raw C array for the board. We replaced it with `vector<vector<Cell>>` because it is safer (no manual memory management) and more idiomatic C++17. We also rejected a highly bloated particle-engine GUI suggestion in favor of a cleaner, minimal glowing UI to maintain focus on the core algorithms.

**Example where AI output was not fully correct:** The original AI-generated `getKey()` used `key == 224` as an integer comparison, which triggered a signed/unsigned comparison warning and behaved incorrectly on some compilers. We changed the condition to compare with the correct hex literal `'\xe0'` and added a cast.

**What we understood and implemented ourselves:** The complete game loop logic, the State Machine architecture, the level-up threshold formula, the dynamic obstacle spawning system, and the O(1) grid collision logic were fundamentally designed and managed by our group.