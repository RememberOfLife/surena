# surena

General purpose board game backend and some AI to go with it.
* Some [supported](#support) games out of the box
* Use game [plugins](#plugins) to play arbitrary games

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
* fix copy clone compare in all game impls here
  * make sure to handle the error string correctly
* use rerrorf in the game impls to clarify errors, and free it
* move history, add plugin functionality for e.g. (universal -> pgn -> universal)
  * want similar translation layer for (de)serialization?
* use readline in main https://stackoverflow.com/questions/2600528/c-readline-function
* add uci wrapper engine for general purpose chess engines (same for tak)
* document workflow for non perfect information games
* stop clang-format newline after structs unions etc.. if already within a struct, union, class, function!
* fill more feature flags for the example games provided in this project

### ideas
* how to do legacy game legacy information properly? just pass around increasing options strings encoding the legacy?
* => team games with team rewards
  * game has to provide player-team association
* => games with rewards more complicated than just win/loss
  * get_scoring instead of or adjacent to get_results
* => some games may need to have access to some sort of time control for their functionality (munchkin and upcount time control games, poker time chips)
  * move timectl to game, managed outside but game provides a method to call back on any significant updates (e.g. ran out of time and game can update the timectl itself after a move was made)
* draw and resign should be handled outside of the game, but for team games the game should be able to provide some input on the voting mechanism
* for simultaneous moves the engine has to merge extra moves into the tree even if there are already moves on a node
  * possibly generate_all_moves_for_all_discretization method for the game class
  * order moves by uint64_t value and then splice possible moves into the actual children
* would there be use for some sort of utility library for handling common game objects
