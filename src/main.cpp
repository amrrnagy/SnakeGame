// =============================================================================
// main.cpp — Entry point.
//
// Responsibilities:
//   1. Seed the random number generator.
//   2. Display a welcome screen.
//   3. Construct and run the Game.
// =============================================================================

#include "Game.h"
#include <iostream>
#include <cstdlib>   // srand()
#include <ctime>     // time()

int main() {
    // Seed once at program start — used by Food::spawn()
    srand(static_cast<unsigned int>(time(nullptr)));

    std::cout << "╔════════════════════════════════════╗\n";
    std::cout << "║    CSE333 Snake Game — Spring 2026  ║\n";
    std::cout << "╚════════════════════════════════════╝\n\n";
    std::cout << "  Controls:\n";
    std::cout << "    W / A / S / D  — Move\n";
    std::cout << "    P              — Pause / Resume\n";
    std::cout << "    Q              — Quit\n\n";
    std::cout << "  Eat * to grow and score points.\n";
    std::cout << "  Avoid # walls, + obstacles, and yourself!\n\n";
    std::cout << "  Press ENTER to start...\n";
    std::cin.ignore();

    Game game;
    game.run();

    return 0;
}
