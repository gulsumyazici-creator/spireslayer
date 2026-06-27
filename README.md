# Spireslayer - CMPE 230 Assignment 1

This project is a command-line interpreter written in **C** for the CMPE 230 Systems Programming course.
It simulates the bookkeeping of a roguelike deckbuilder-style run inspired by **Slay the Spire**.

The program reads commands line by line, updates the current game state, and prints the required output according to the assignment specification.

## About the Project

In this project, the player controls the state of **Ironclad** through text commands.
The interpreter keeps track of gold, health, floor progress, cards, relics, potions, combat notes, defeated enemies, and exhaust cards.

Each input line is either:

* a state-changing command,
* a read-only query,
* or the exit command.

Malformed commands are handled by printing `INVALID`.

## Features

* Command-line interpreter written in C
* Line-by-line input parsing
* Strict command validation
* Persistent game state
* Gold, HP, floor, and room tracking
* Deck management with base and upgraded cards
* Exhaust card handling
* Relic ownership system
* Potion belt with capacity limit
* Enemy effectiveness codex
* Fight logic with victory/defeat outcomes
* Query commands for checking current state

## Project Structure

```text
.
├── README.md
├── main.c
```

## Main Components

### `main.c`

Contains the full implementation of the interpreter.

The program includes:

* tokenization of input lines,
* positive integer validation,
* entity name validation,
* reserved keyword checking,
* command dispatching,
* state-changing command handlers,
* read-only query handlers,
* fight and inventory logic.

## Supported State

The interpreter keeps track of:

* gold amount,
* current HP and max HP,
* current floor,
* current room,
* deck contents,
* upgraded cards,
* relics,
* potions,
* exhaust-tagged cards,
* enemy effectiveness notes,
* defeated enemy counts.

## Example Commands

```text
Ironclad gains 50 gold
Ironclad gains card Bash
Ironclad upgrades card Bash
Ironclad gains relic Burning Blood
Ironclad gains potion Fire Potion
Ironclad enters Monster room
Ironclad learns card Bash is effective against Gremlin Nob
Ironclad fights Gremlin Nob
Deck ?
Health ?
Total gold ?
Exit
```

## How to Compile

Use `gcc` to compile the project:

```bash
gcc main.c -o spireslayer
```

## How to Run

After compiling, run:

```bash
./spireslayer
```

On Windows, you can run:

```bash
spireslayer.exe
```

## Notes

* The program is case-sensitive.
* Input commands must match the expected grammar.
* Invalid syntax prints `INVALID`.
* Semantic errors, such as not having enough gold or a missing card, print their specific error messages.
* The program continues running until the `Exit` command is entered.

## Course Context

This project was implemented for **CMPE 230 - Systems Programming**.
The main goal was to practice C programming, parsing, state management, arrays, string handling, and command-line interpreter design.
