# surena

General purpose board game backend and some AI to go with it.
* Some [supported](#support) games out of the box
* Use game [plugins](#plugins) to play arbitrary games

Don't forget to clone submodules too by using:  
`git clone --recurse-submodules https://github.com/RememberOfLife/surena.git`

## usage
`surena <version/repl>`

## support
|game|status|
|---|---|
|Chess|DONE|
|Havannah|DONE|
|Quasar|DONE|
|RockPaperScissors|DONE|
|TicTacToe|DONE|
|TicTacToe.Ultimate|DONE|
|TwixT.PP|DONE|

## plugins
The surena project provides powerful APIs and utilities for creating all kinds of board games.  
For more details regarding the various APIs available, see the [design](./docs/design.md) doc.

## backlog

### todos
* use rerrorf in the game impls to clarify errors, and free it
* move history, add plugin functionality for e.g. (universal -> pgn -> universal)
  * want similar translation layer for (de)serialization?
* use readline in main https://stackoverflow.com/questions/2600528/c-readline-function
* add uci wrapper engine for general purpose chess engines (same for tak)
* document workflow for non perfect information games
* stop clang-format newline after structs unions etc.. if already within a struct, union, class, function!
* fill more feature flags for the example games provided in this project
