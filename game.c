// Hangman with Raylib + OS Concepts Integration (Fullscreen + Q&A Mode)
#include "raylib.h" // Raylib for graphics
#include <stdio.h>
#include <string.h> 
#include <stdbool.h> 
#include <pthread.h> // For threads and mutexes
#include <stdlib.h> // For general utilities (rand, exit)
#include <unistd.h> // For sleep and fork
#include <sys/wait.h> // For wait (used with fork)
#include <time.h> 

// Constants for game limits
#define MAX_TRIES 6 // Maximum number of incorrect attempts allowed
#define MAX_LETTERS 26 // Maximum number of letters in English alphabet
#define WORD_FILE "words.txt" // File containing question|answer pairs
#define LOG_FILE "game_log.txt" // Log file to store game resultss

// Global game state variables
char guessedLetters[MAX_LETTERS] = {0}; // Letters the user has guessed so far
int tries = 0; // Number of incorrect attempts
char displayWord[100] = ""; // Visual representation of the word with underscores and revealed letters
bool guessed[26] = { false }; // Boolean array to track which letters have been guessed
char WORD[100] = ""; // The actual word to be guessed
char question[256] = ""; // The question/prompt for the word
bool gameEnded = false; // Tracks if the game is over
bool timeUp = false; // Set to true by timer thread after 60 seconds
int timeLeft = 30; // Time for user to guess the word

pthread_mutex_t lock; // Mutex for synchronizing access to shared variables (OS Concept: Synchronization)
pthread_t timerThread; // Timer thread declaration

// Load a random question-answer pair from file
void LoadRandomWord() {
    FILE *file = fopen(WORD_FILE, "r"); // Open the word file for reading
    if (!file) { // If file can't be opened, load default question/word
        strcpy(WORD, "elephant"); // Default word
        strcpy(question, "Large gray animal?"); // Default question
        return;
    }

    char lines[50][256]; // Temporary array to store up to 50 lines from file
    int count = 0; // Line counter

    while (fgets(lines[count], sizeof(lines[count]), file) && count < 50) { // Read each line until 50 or end of file
        lines[count][strcspn(lines[count], "\n")] = 0; // Strip newline from each line
        count++; // Increment line count
    }
    fclose(file); // Close the file after reading

    if (count == 0) { // If no lines were read (empty file)
        strcpy(WORD, "elephant");
        strcpy(question, "Large gray animal?");
        return;
    }

    srand(time(NULL)); // Seed the random number generator (used for random selection)
    int randIndex = rand() % count; // Randomly select one of the lines

    char *delimiter = strchr(lines[randIndex], '|'); // Look for '|' delimiter separating question and word
    if (delimiter) { // If delimiter found
        *delimiter = '\0'; // Replace delimiter with null to split string
        strcpy(question, lines[randIndex]); // Copy question part
        strcpy(WORD, delimiter + 1); // Copy word part
    } else { // If format is incorrect
        strcpy(WORD, "elephant");
        strcpy(question, "Invalid format in words.txt");
    }
}

// Update the word display based on guessed letters
void UpdateWordDisplay() {
    int len = strlen(WORD); // Get the length of the word
    int idx = 0; // Index for displayWord

    for (int i = 0; i < len; i++) { // Loop through each character of the word
        if (guessed[WORD[i] - 'a']) // If letter was guessed
            displayWord[idx++] = WORD[i]; // Add letter to display
        else
            displayWord[idx++] = '_'; // Add underscore if not guessed
        displayWord[idx++] = ' '; // Add space after each character/underscore
    }
    displayWord[idx] = '\0'; // Null terminate the string
}

// Check if the player has guessed the entire word
bool IsWordComplete() {
    for (int i = 0; i < strlen(WORD); i++) { // Check each letter of the word
        if (!guessed[WORD[i] - 'a']) return false; // If any letter isn't guessed, word isn't complete
    }
    return true; // All letters guessed
}

// Log result using process creation (fork) - OS Concept: Process creation + Inter-process communication
void StartLoggerProcess(const char *result) {
    pid_t pid = fork(); // Create a new process (OS Concept: Process creation)
    if (pid == 0) { // Child process
        FILE *log = fopen(LOG_FILE, "a"); // Open log file for appending (OS Concept: Inter-process communication via file)
        if (log) {
            fprintf(log, "Result: %s | Word: %s | Question: %s\n", result, WORD, question); // Write game result
            fclose(log); // Close log file
        }
        exit(0); // Terminate child process
    }
}

// Timer thread that ends game after 30 seconds - OS Concept: Threading + Synchronization
void *TimerThread(void *arg) {
    for (int i = 0; i < 30; i++) {
        sleep(1);
        pthread_mutex_lock(&lock);
        timeLeft--;
        pthread_mutex_unlock(&lock);
    }
    pthread_mutex_lock(&lock); // Lock mutex before modifying shared variable 
    timeUp = true; // Indicate time is up
    pthread_mutex_unlock(&lock); // Unlock mutex
    return NULL; // Exit thread
}

void DrawWrappedText(const char *text, int x, int y, int maxWidth, int fontSize, Color color) {
    char buffer[1024]; // Temp buffer for a single line
    int len = strlen(text); // Get total length of the input text
    int lineStart = 0; // Index where the current line starts

    while (lineStart < len) {
        int lineLength = 0; // Length of current line in characters
        int lastSpace = -1; // Position of last space found 
        int i = lineStart; 

        // Try to fill the line buffer until width exceeds maxWidth
        while (i < len) {
            buffer[lineLength] = text[i];
            buffer[lineLength + 1] = '\0';

            int width = MeasureText(buffer, fontSize);
            if (width > maxWidth) break;
            if (text[i] == ' ') lastSpace = i;
            i++;
            lineLength++;
        }

        // Handle line break correctly based on whether a space was found 
        int actualLength;
        if (i < len && lastSpace > lineStart) {
            actualLength = lastSpace - lineStart;
        } else {
            actualLength = lineLength;
        }

        // Copy calculated line into buffer and null-terminate
        strncpy(buffer, &text[lineStart], actualLength);
        buffer[actualLength] = '\0';
        // Draw the current line of words 
        DrawText(buffer, x, y, fontSize, color);
        y += fontSize + 5; // Move to next line
        
        // Move to the start of the next line
        if (i < len && lastSpace > lineStart) {
            lineStart = lastSpace + 1;
            // Skipp additional spaces
            while (text[lineStart] == ' ' && lineStart < len) lineStart++;
        } else {
            lineStart += actualLength;
        }
    }
}

int main() {
    InitWindow(0, 0, "Hangman with Raylib + Q&A + OS Concepts"); // Create fullscreen Raylib window
    SetWindowState(FLAG_FULLSCREEN_MODE); // Set window to fullscreen mode
    int screenWidth = GetScreenWidth(); // Get screen width for positioning
    int screenHeight = GetScreenHeight(); // Get screen height for positioning
    SetTargetFPS(60); // Set FPS to 60 for smooth rendering

    LoadRandomWord(); // Load a question-answer pair from file
    UpdateWordDisplay(); // Generate initial word display with underscores
    pthread_mutex_init(&lock, NULL); // Initialize mutex (OS Concept: Synchronization)
    pthread_create(&timerThread, NULL, TimerThread, NULL); // Start timer thread (OS Concept: Threading)

    while (!WindowShouldClose()) { // Main game loop until window closes
        pthread_mutex_lock(&lock); // Lock mutex for thread-safe checks
        bool lose = tries >= MAX_TRIES || timeUp; // Lose if max tries or timer runs out
        bool win = IsWordComplete(); // Check for win condition
        pthread_mutex_unlock(&lock); // Unlock mutex

        // Stop the timer when the word is guessed correctly
        if (win && !gameEnded) {
            pthread_cancel(timerThread); // Cancel timer thread if word is guessed correctly
            StartLoggerProcess("Win"); // Log win result using child process
            gameEnded = true; // Mark game as ended
        }

        if (!gameEnded && (win || lose)) { // If game just ended
            StartLoggerProcess(win ? "Win" : "Lose"); // Log result using child process (OS Concept: Process + IPC)
            gameEnded = true; // Mark game as ended
        }

        if (!gameEnded) { // If game is still ongoing
            int key = GetCharPressed(); // Get key pressed by user
            if (key >= 'a' && key <= 'z') { // Ensure key is lowercase letter
                if (!guessed[key - 'a']) { // If letter hasn't been guessed before
                    guessed[key - 'a'] = true; // Mark letter as guessed

                    int len = strlen(guessedLetters); // Find position to insert letter in guessedLetters
                    guessedLetters[len] = (char)key; // Store guessed letter
                    guessedLetters[len + 1] = '\0'; // Null terminate string

                    if (!strchr(WORD, key)) tries++; // If letter not in word, increment tries
                    UpdateWordDisplay(); // Refresh word display
                }
            }
        }

        BeginDrawing(); // Start rendering frame
        ClearBackground(RAYWHITE); // Set background to white

        // Display game title centered
        DrawText("Hangman Game (Q&A Edition)", screenWidth / 2 - MeasureText("Hangman Game (Q&A Edition)", 40) / 2, 30, 40, DARKBLUE);
        // Show question centered
        char fullQuestion[300];
        sprintf(fullQuestion, "Question: %s", question);
        int textWidth = screenWidth - 100; // Leave margins to wrapped the question
        DrawWrappedText(fullQuestion, 50, 100, textWidth, 30, DARKGRAY);


        // Drawing hangman base
        int baseX = screenWidth / 2 - 100; // X base position
        int baseY = 250; // Y base position
        int scale = 2; // Scale for drawing hangman

        DrawLine(baseX, baseY + 300, baseX + 100 * scale, baseY + 300, BLACK); // Ground line
        DrawLine(baseX + 50 * scale, baseY + 300, baseX + 50 * scale, baseY + 5 * scale, BLACK); // Vertical pole
        DrawLine(baseX + 50 * scale, baseY + 5 * scale, baseX + 100 * scale, baseY + 5 * scale, BLACK); // Top beam
        DrawLine(baseX + 100 * scale, baseY + 30 * scale, baseX + 100 * scale, baseY + 5 * scale, BLACK); // Rope

        // Draw hangman parts based on tries
        if (tries > 0) DrawCircle(baseX + 100 * scale, baseY + 30 * scale, 10 * scale, BLACK); // Head
        if (tries > 1) DrawLine(baseX + 100 * scale, baseY + 80 * scale, baseX + 100 * scale, baseY + 30 * scale, BLACK); // Body
        if (tries > 2) DrawLine(baseX + 100 * scale, baseY + 80 * scale, baseX + 80 * scale, baseY + 120 * scale, BLACK); // Left arm
        if (tries > 3) DrawLine(baseX + 100 * scale, baseY + 80 * scale, baseX + 120 * scale, baseY + 120 * scale, BLACK); // Right arm
        if (tries > 4) DrawLine(baseX + 100 * scale, baseY + 50 * scale, baseX + 80 * scale, baseY + 120, BLACK); // Left leg
        if (tries > 5) DrawLine(baseX + 100 * scale, baseY + 50 * scale, baseX + 120 * scale, baseY + 120, BLACK); // Right leg

        // Show result or instruction
        if (win)
            DrawText("🎉 You won! 🎉", screenWidth / 2 - MeasureText("🎉 You won! 🎉", 30) / 2, screenHeight / 2 + 100, 30, GREEN);
        else if (lose)
            DrawText(TextFormat("💀 You lost! The word was: %s", WORD), screenWidth / 2 - MeasureText(TextFormat("💀 You lost! The word was: %s", WORD), 30) / 2, screenHeight / 2 + 100, 30, MAROON);
        else
            DrawText("Type a letter (a-z) to guess", screenWidth / 2 - MeasureText("Type a letter (a-z) to guess", 24) / 2, screenHeight - 250, 25, GRAY);

        // Display tries and guessed word
        DrawText(TextFormat("Tries: %d/%d", tries, MAX_TRIES), screenWidth / 2 - MeasureText(TextFormat("Tries: %d/%d", tries, MAX_TRIES), 25) / 2, screenHeight - 375, 25, RED);
        DrawText(TextFormat("Word: %s", displayWord), screenWidth / 2 - MeasureText(TextFormat("Word: %s", displayWord), 30) / 2, screenHeight - 300, 30, BLACK);

        // Show guessed letters
        DrawText(TextFormat("Guessed Letters: %s", guessedLetters), screenWidth / 2 - MeasureText(TextFormat("Guessed Letters: %s", guessedLetters), 25) / 2, screenHeight / 2 + 150, 25, DARKGRAY);

        // Draw the timer at the top of the screen and show remaining time
        if (!gameEnded) { // Only draw the timer if the game is still going on
            DrawText(TextFormat("Time Left: %d", timeLeft), 60, 40, 30, (timeLeft <= 10 ? RED : DARKGREEN));
        }

        // Display exit button if game ended
        if (gameEnded) {
            Rectangle btn = { screenWidth / 2 - 100, screenHeight - 200, 200, 50 }; // Exit button rectangle
            DrawRectangleRec(btn, DARKGRAY); // Draw button background
            DrawText("Exit Game", btn.x + 40, btn.y + 10, 30, RAYWHITE); // Button label

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btn)) { // On button click
                CloseWindow(); // Close window
                exit(0); // Exit program
            }
        }

        EndDrawing(); // End frame drawing
    }

    if (pthread_cancel(timerThread) == 0) {
        pthread_join(timerThread, NULL);
    }

    pthread_join(timerThread, NULL); // Wait for timer thread to finish
    pthread_mutex_destroy(&lock); // Destroy mutex before exiting
    CloseWindow(); // Close the Raylib window
    return 0; // Return from main
}
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

