# surena

General purpose board game backend and some AI to go with it.

## support
|game|status|
|---|---|
|Chess|DONE|
|Havannah|DONE|
|TicTacToe|DONE|
|TicTacToe.Ultimate|DONE|

### games todo

|game|flags|relative prio|
|---|---|---|
|6 Nimmt!|HI, SM, RM|+|
|Alhambra|RM|++|
|Azul|RM||
|Caesar|HI, RM|++|
|Connect4|PI||
|Go|PI|-|
|Oshi Sumo|SM|+|
|Quoridor|PI||
|Settlers of Catan|HI, RM||
|Skat|HI, RM|+|
|Tak|PI|++|
|TwixT PP|PI||
|Wizards|HI, RM||


## issues


## todos
* replace prng by squirrelnoise5
* engine search timeout should be uint32_t
* engine needs compat. check for games

## ideas
* state import/export
  * import state should also take an argument what length of bytes to import (i.e. for binary exports that don't have a string zero terminator)
  * export state should probably take a format specifier, same for import, so different formats can be used
* game should expose:
  * maximum amount of moves generated from any random position
  * maximum amount of players that can play this game
* clone method for the game:
  * option to choose if to discretize if available or not
  * only to copy for a certain privacy view (i.e. which player perspectives actual data to include)
* for simultaneous moves the engine has to merge extra moves into the tree even if there are already moves on a node
  * possibly generate_all_moves_for_all_discretization method for the game class
  * order moves by uint64_t value and then splice possible moves into the actual children
* do engines need some way to give ALL moves / actions evaluations?
  * i.e. for a game like havannah, rate all possible placements

## problems
* is the swap move for the pie rule a feature flag, or a dedicated move that is generated only on the first move for the second player (havannah needs it)
* what other generalized constraints on search start are required?
  * how can specialized constraints be offered? (e.g. depths or moves)
* need some sort of event queue to recv/send events to and from the engine
  * e.g. engine periodically pushes out info events with current bestmove and searchinfo

### integration workflow
* ==> games with simultaneous moves
  * game unions all possible moves from all outstanding players to move simultaneously, returns that as the valid_moves_list
  * when a player moves, their move is stored by the backend game into its accumulation buffer
    * when the last remaining player, of all those who move simultaneously, makes their move, the game processes the accumulation buffer and proceeds
  * move gen should be able to receive that it should only output moves for a certain players perspective, if simultaneous moves are about to happen
    * otherwise the same simultaneous move can be made with multiple orderings of the specific players moves
* ==> more feature flags?:
  * simultaneous moves (e.g. 2 players move at once, the order in which they decide on their move does not matter)
    * ordered simultaneous moves (e.g. 2 players move at once, but the moves available DO change if one player makes a move)
    * SM may be repr. by all moves being SM-once and then offer an "is-commutative" flag from every SM move gamestate
    * for normal, human vs human use case, just resend when denied for older from-state, no commutative moves
  * big moves (i.e. game requests space for moves >64bit, will get handled via requested size byte buffers)
  * binary state export (maybe make this a universal tagged thing and implementation specific, i.e. games can expose what format they support)
