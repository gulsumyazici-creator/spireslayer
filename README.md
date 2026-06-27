# Spireslayer - CMPE 230 Assignment 1

This project is a command-line interpreter written in **C** for the CMPE 230 Systems Programming course.
It simulates the bookkeeping of a roguelike deckbuilder-style run inspired by **Slay the Spire**.

The program reads one command per line, updates the current game state, and prints the required output according to the assignment specification.

## About the Project

In this project, the player controls the state of **Ironclad** through text commands.

The interpreter keeps track of:

* gold,
* current HP and max HP,
* current floor and room,
* cards and upgraded cards,
* relics,
* potions,
* enemy effectiveness notes,
* defeated enemy counts,
* exhaust-tagged cards.

Each input line is either a state-changing command, a read-only query, or the `Exit` command.

Malformed commands print:

```text
INVALID
```

## Features

* Command-line interpreter written in C
* Strict input parsing and command validation
* Persistent game state
* Gold, HP, floor, and room tracking
* Deck management with base and upgraded card copies
* Exhaust card handling
* Unique relic ownership
* Potion belt with capacity limit
* Enemy effectiveness codex
* Fight logic with victory and defeat outcomes
* Read-only query commands
* Makefile-based build system

## Project Structure

```text
.
├── README.md
├── Makefile
└── src
    └── main.c
```

## Main File

### `src/main.c`

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

The implementation uses plain C arrays and structs to store the game state.

## How to Build

Compile the project using the included Makefile:

```bash
make
```

This creates an executable named:

```text
spireslayer
```

## How to Run

After building, run:

```bash
./spireslayer
```

Then enter commands line by line.

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

## Clean Build Files

To remove the compiled executable:

```bash
make clean
```

## Notes

* The program is case-sensitive.
* Input commands must match the expected grammar.
* Invalid syntax prints `INVALID`.
* Semantic errors, such as not having enough gold or a missing card, print their specific error messages.
* The program keeps running until the `Exit` command is entered.

## Course Context

This project was implemented for **CMPE 230 - Systems Programming**.
The main goal was to practice C programming, parsing, command-line interpreter design, state management, arrays, structs, and string handling.

