## Project Description:

This project demonstrates two implementations of a multiplayer guessing game:

1. **Process-Based Version** (`game_process_group9.c`)  
   - Two players try to guess each other's location on a grid.
   - Uses `fork()` to create child processes.
   - Shared memory (POSIX) is used to communicate between processes.
   - The game is played for a maximum of 3 rounds or until a player correctly guesses the other's location.
   - Winner is declared based on correct guess or the lowest total distance.

2. **Thread-Based Version** (`game_thread_group9.c`)  
   - Multiple players (up to 10) guess other players' positions on a shared grid.
   - Each player is represented by a thread (`pthread`).
   - Shared memory is again used for shared data.
   - Each thread makes guesses, calculates distances, and the winner is the one with the closest final guess.

---

## Compilation Instructions:

To compile the process-based version:
```bash
gcc -o process_game game_process_group9.c -lrt
```

To compile the thread-based version:
```bash
gcc -o thread_game game_thread_group9.c -lpthread -lrt
```

---

## Running the Programs:

For process-based game:
```bash
./process_game 8
```
Where `8` is the size of the square board (must be ≤ 10).

For thread-based game:
```bash
./thread_game 8 4
```
Where `8` is the board size and `4` is the number of players (between 3 and 10).

---

## Notes:

- Shared memory (`shm_open`, `mmap`) is used in both implementations.
- Make sure to have the required permissions and memory cleanup after crashes.
- The board is visualized in the terminal with players represented by numbers.
- Game logic uses Manhattan distance for guess evaluation.
