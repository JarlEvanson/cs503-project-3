# Project 3 - Custom Interpreter

## Project Description

The project provides an implementation of Blackjack in Haskell.

This implementations implements the following features:
- Betting
- Unlimited Hands
- Insurance Bets
- Doubling Down

## Organization

The code is organized into a single directory (`src`). `main.hs` exists to
stub out the code and enable use of all other code for testing in a simple
manner. `cards.hs` provides the definition of cards and various functions
for generating, manipulating, and summing cards and hands. `util.hs` provides
some useful utility functions that were not directly relevant to program
execution. `gameplay.hs` contains all of the code that directs the course
of gameplay, deals with extracting random cards, prompting players, and
calculating bets.

All test files are located in `tests` and provide various integration tests
that contain input to a run of the program and their expected results. The
test-runner simply compares the expected results of a run with the actual results
to detect success or failure.

## Build Process

### Prerequisites

In order to build the implementation of Blackjack, the following program must
be installed: `ghc`.

### Building

All build commands should be done with the current directory being the base of
this project.

To build the implementation of Blackjack, run:
```bash
ghc -o blackjack src/main.hs src/cards.hs src/util.hs src/gameplay.hs
```

To build the test runner, run:
```bash
ghc -o test-runner src/test-runner.hs src/cards.hs src/util.hs src/gameplay.hs
```

### Running and Testing

The program can be run by running the produced executable starting with the name
`blackjack` while the test runner can be run using the produced executable with
the name `test-runner`.
