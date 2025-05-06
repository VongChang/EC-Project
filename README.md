# EC-Project Test

/*
================================================================================
OPERATING SYSTEM CONCEPTS IMPLEMENTED IN THIS GAME (Hangman with Raylib)
================================================================================

1.THREADING + Singal and Timers 
   - Function: pthread_create(&timer, NULL, TimerThread, NULL);
   - Purpose: Runs a 30-second countdown timer in a separate thread without blocking the main game loop.
   - Benefit: Enables simultaneous gameplay and time tracking (multitasking).

2.SYNCHRONIZATION (pthread_mutex_t, pthread_mutex_lock/unlock)
   - Function: pthread_mutex_lock(&lock); ... pthread_mutex_unlock(&lock);
   - Purpose: Protects shared variables (tries, timeUp, etc.) from being accessed and modified by multiple threads at once.
   - Benefit: Prevents race conditions and ensures thread-safe updates to shared game state.

3.PROCESS CREATION (fork)
   - Function: pid_t pid = fork();
   - Purpose: Creates a child process whose only task is to handle writing the game result to a file after the game ends.
   - Benefit: By moving file writing into a child process, the main game remains responsive, showing how process creation can help offload tasks.

4.INTER-PROCESS COMMUNICATION (File-based)
   - Function: fprintf(log, "Result: %s | Word: %s | Question: %s\n", ...);
   - Purpose: The parent and child processes communicate indirectly by writing and accessing a shared log file (result_log.txt).
   - Benefit: Demonstrates IPC (Inter-Process Communication) using a shared file, a simple yet effective mechanism for data exchange between processes.

5.FILE MANAGEMENT (fopen, fgets, fclose, fprintf)
   - Functions:
        - fopen("words.txt", "r") - Load word/question pairs.
        - fgets(...), fclose() - Read and close the word file.
        - fprintf(...), fopen("result_log.txt", "a") - Write result to log file.
   - Purpose: Handles persistent storage of game content (questions/words) and player outcomes.
   - Benefit: Shows the use of file system operations for reading input and writing output, integrating basic file management into gameplay.
*/
