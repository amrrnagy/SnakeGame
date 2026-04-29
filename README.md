# Snake Game

A C++ implementation of the classic Snake game, built to demonstrate core data structures and algorithms from the course. The project supports both console mode and an optional Qt graphical interface.

## Features

- Snake movement controlled by arrow keys (WASD in console, arrows in Qt)
- Random food generation avoiding obstacles and snake body
- Snake growth on eating food
- Collision detection: wall, self, and obstacle collisions
- Score tracking
- Pause / Resume (Space)
- Game over and restart functionality (R)
- Optional Qt GUI with smooth rendering (bonus)

## Data Structures & Justification

| Data Structure | Where Used | Reason |
|----------------|------------|--------|
| `std::deque<Point>` | Snake body | O(1) push_front (head insertion) and pop_back (tail removal) for movement, random access for self‑collision scan O(n) |
| `std::unordered_set<Point>` | Board obstacles | O(1) average lookup for collision detection, sparse storage |
| Simple arrays (grid concept) | Board boundaries | O(1) boundary check using width/height |

## How to Compile & Run

### Console version (core logic)
```bash
mkdir build && cd build
cmake ..
make
./SnakeGameConsole
