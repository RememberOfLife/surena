# surena

General purpose board game backend and some AI to go with it.
* Some [supported](#support) games out of the box
* Use game [plugins](#plugins) to play arbitrary games

Don't forget to clone submodules too by using:  
`git clone --recurse-submodules `

## usage
`surena [options]`  
available options:
* `--game <basegame[.variant]>` play the **basegame**, if specified use the **variant** (standard variants are not specified)
* `--game-plugin <file>` play the the first provided game method of the **file** game plugin by using the game plugin loader
* `--game-options <string>` don't create a default game, but use the options **string** instead
* `--game-state <string>` after creating the game, import the given state **string**
* `--version`

## support
|game|status|
|---|---|
|Chess|DONE|
|Havannah|DONE|
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
