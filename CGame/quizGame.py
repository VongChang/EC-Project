# quiz_gui.py
# A GUI-based Kahoot-style quiz game using OS concepts in Python

# --- Standard Library Imports ---
import json
import time
import threading
import multiprocessing
from multiprocessing import Process, Queue, Lock
from tkinter import Tk, Label, Button, StringVar, Frame, PhotoImage, Scrollbar, Text
import random
import os

# --- Function to Load Quiz Questions from JSON file (File Management) ---
def load_questions(filename):
    with open(filename, 'r') as f:
        return json.load(f)

# --- Countdown Timer Thread (Threading + Timers) ---
# Runs a countdown for a specified duration and updates the GUI each second.
def start_timer(duration, update_func, done_event):
    for i in range(duration, -1, -1):
        if done_event.is_set():
            break
        update_func(i)
        time.sleep(1)
    if not done_event.is_set():
        done_event.set()

# --- Player Answer Process (Process Creation + IPC + Synchronization) ---
# This function is run in a separate process to simulate checking if a player's answer is correct.
def player_process(answer_index, correct_index, result_queue, lock):
    with lock:  # Synchronization (Mutex Lock)
        if answer_index == correct_index:
            result_queue.put(1)  # IPC via message queue
        else:
            result_queue.put(0)

# --- Main GUI Class for the Quiz Game ---
class QuizGameGUI:
    def __init__(self, root, question_file):
        self.root = root
        self.root.title("OS Concepts Quiz Game")
        self.root.attributes('-fullscreen', True)

        # Load questions and initialize variables
        self.questions = load_questions(question_file)
        self.score = 0
        self.lock = Lock()  # Synchronization Lock
        self.result_queue = Queue()  # IPC: Queue for results
        self.current_question = 0
        self.selected_answer = None
        self.answers_given = []
        self.accepting_input = True
        self.timer_done = threading.Event()

        # GUI state variables
        self.question_var = StringVar()
        self.timer_var = StringVar()
        self.feedback_var = StringVar()
        self.image_label = None
        self.buttons = []

        self.setup_gui()
        self.show_question()

    # --- GUI Setup ---
    def setup_gui(self):
        Label(self.root, textvariable=self.timer_var, font=("Arial", 20)).pack(pady=10)
        Label(self.root, textvariable=self.question_var, wraplength=1000, font=("Arial", 24)).pack(pady=10)

        self.image_label = Label(self.root)
        self.image_label.pack(pady=10)

        Frame(self.root, height=100).pack(expand=True)

        self.button_frame = Frame(self.root)
        self.button_frame.pack(pady=20, fill="x", expand=True)

        for col in range(2):
            self.button_frame.grid_columnconfigure(col, weight=1)

        # Create 4 answer buttons
        for i in range(4):
            btn = Button(
                self.button_frame,
                text="",
                font=("Arial", 20),
                height=4,
                wraplength=500,
                command=lambda i=i: self.submit_answer(i)
            )
            row, col = divmod(i, 2)
            btn.grid(row=row, column=col, padx=20, pady=10, sticky="ew")
            self.buttons.append(btn)

        Label(self.root, textvariable=self.feedback_var, font=("Arial", 20)).pack(pady=20)
        Button(self.root, text="Exit", font=("Arial", 14), command=self.root.destroy).pack(pady=5)

    # --- Display the Current Question and Start Timer ---
    def show_question(self):
        if self.current_question >= len(self.questions):
            self.end_game()
            return

        q = self.questions[self.current_question]
        self.question_var.set(q['question'])  # Set question text

        # Load image if present (File Management)
        if 'image' in q:
            try:
                img = PhotoImage(file=q['image'])
                self.image_label.configure(image=img)
                self.image_label.image = img
            except:
                self.image_label.configure(image='')
                self.image_label.image = None
        else:
            self.image_label.configure(image='')
            self.image_label.image = None

        # Set button text and enable them
        for i, opt in enumerate(q['options']):
            self.buttons[i].config(text=opt, state='normal')

        self.feedback_var.set("")
        self.selected_answer = None
        self.accepting_input = True

        # Ensure previous timer thread is stopped
        if hasattr(self, 'timer_done') and not self.timer_done.is_set():
            self.timer_done.set()
        if hasattr(self, 'timer_thread') and self.timer_thread.is_alive():
            self.timer_thread.join()

        # Start 30-second countdown timer thread
        self.timer_done.clear()
        self.timer_thread = threading.Thread(target=start_timer, args=(30, self.update_timer, self.timer_done))
        self.timer_thread.start()

        # Check if time ran out after 31s
        self.root.after(31000, self.check_timer)

    # --- Update Timer Display ---
    def update_timer(self, value):
        self.timer_var.set(f"Time Left: {value} seconds")

    # --- Handle Timer Expiry ---
    def check_timer(self):
        if not self.timer_done.is_set():
            self.timer_done.set()
            self.submit_answer(-1)  # No answer

    # --- Handle Answer Submission ---
    def submit_answer(self, index):
        if not self.accepting_input:
            return
        self.accepting_input = False

        # Disable buttons
        for btn in self.buttons:
            btn.config(state='disabled')

        q = self.questions[self.current_question]
        self.selected_answer = index

        # Create a new process to check the answer (Process Creation + IPC + Synchronization)
        p = Process(target=player_process, args=(index, q['answer'], self.result_queue, self.lock))
        p.start()
        p.join()

        result = self.result_queue.get()

        # Show feedback and update score
        if result == 1:
            self.feedback_var.set("Correct! ✅")
            self.score += 1
            self.answers_given.append((q['question'], q['options'][index] if index != -1 else "No Answer", "Correct"))
        else:
            correct_text = q['options'][q['answer']]
            self.feedback_var.set(f"Incorrect ❌. Correct: {correct_text}")
            self.answers_given.append((q['question'], q['options'][index] if index != -1 else "No Answer", f"Correct: {correct_text}"))

        self.current_question += 1
        self.root.after(3000, self.show_question)

    # --- End the Game and Show Summary ---
    def end_game(self):
        for widget in self.root.winfo_children():
            widget.destroy()

        Label(self.root, text=f"Game Over! Final Score: {self.score} / {len(self.questions)}", font=("Arial", 28)).pack(pady=20)
        Label(self.root, text="Review of Your Answers:", font=("Arial", 22)).pack(pady=10)

        text_frame = Frame(self.root)
        text_frame.pack(fill="both", expand=True, padx=20, pady=10)

        scrollbar = Scrollbar(text_frame)
        scrollbar.pack(side="right", fill="y")

        review_box = Text(text_frame, wrap="word", yscrollcommand=scrollbar.set, font=("Arial", 14))
        for i, (q, ans, feedback) in enumerate(self.answers_given):
            review_box.insert("end", f"Q{i+1}: {q}\nYour Answer: {ans}\nResult: {feedback}\n\n")

        review_box.pack(fill="both", expand=True)
        scrollbar.config(command=review_box.yview)

        Button(self.root, text="Exit", font=("Arial", 14), command=self.root.destroy).pack(pady=10)

# --- Entry Point ---
if __name__ == '__main__':
    multiprocessing.freeze_support()  # For Windows compatibility
    root = Tk()
    app = QuizGameGUI(root, "questions.json")
    root.mainloop()

# -----------------------------------------------------------------------------------
# ✔ OS REQUIREMENTS USED & THEIR FUNCTION IN THE PROGRAM:

# 1. Process creation (multiprocessing.Process)
#    - Used in `player_process()` to simulate answer checking in a separate process.

# 2. Inter-process communication (Queue)
#    - Used to pass the result (correct/incorrect) from the process back to the main GUI.

# 3. Threading (threading.Thread)
#    - Used to run the countdown timer concurrently without blocking the main GUI.

# 4. Synchronization (Lock)
#    - Used inside the `player_process()` to safely access shared resources (if needed).

# 5. Timers / Signal handling
#    - Timer implemented using threads and `root.after()` to trigger timeout behavior.

# 6. File Management
#    - Loads questions from a JSON file; also optionally loads image files per question.
