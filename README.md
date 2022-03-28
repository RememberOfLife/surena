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


## issues


## todos
* typedef player(uint8_t) and move(uint64_t)
  * provide common values, e.g. player_none(0x00) and player_random(0xFF)
  * add nullmove globally, likely use ~0
* engine search timeout should be uint32_t
* engine needs compat. check for games

## ideas
* state import/export
  * import state should also take an argument what length of bytes to import (i.e. for binary exports that don't have a string zero terminator)
  * export state should probably take a format specifier, same for import, so different formats can be used
* game should expose:
  * amount of players playing this game right now (also construct variable player count games with a player count)
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
* handle imperfect information games, introductions of hidden information
  * look at information sets some more, for now just use:
    * move; actual thing that happened (i.e. roll_d6(1, 2, ..) or draw_facedown(1, 2, ..))
    * action; class of moves (i.e. draw_facedown(unknown) or roll_d6(unknown) or draw_faceup(unknown))
  * e.g. a player places a card facedown or draw a card without showing the others
    * also, there is always the chance that some information can be inferred about what the player just did by others
  * faceup randomness represented by all options of the possible moves being made the player 0xFF
  * facedown hidden information / randomness, the game has to be able to:
    * show what the possible moves are that could happen for the player to move
      * enabled side-by-side-replaying of games, i.e. register what happened on a physical board with the digital copy for player we know hidden info about
    * accept an action instead of a move to signal the game that the player moved (i.e. observe player drawing facedown, but we don't know what they drew)
      * not all moves/actions here will actually introduce hidden information (i.e. maybe the player can do multiple things in their turn)
  * hidden info init?
   * e.g. 2 decks where people draw cards, left one facedown, right one face up
     * indiscrete game offers moves: draw_facedown (unknown), draw_facedown (#1, #2, ...), draw_faceup (#1, #2, ...)
     * game offers method move->action: would convert
       * draw_facedown(unknown)=>draw_facedown(unknown)
       * draw_facedown(#1, #2, ...)=>draw_facedown(unknown)
       * draw_faceup(#1, #2, ...)=>draw_faceup(unknown) <- even though this is never offered
     * this is a feature (maybe two, do a little think), complete_info maybe
     * if a dice is rolled, the discretized board replaces its move gen by just the straight result from the roll
       * also less data moved if the gen moves func from the game changes after discretization
       * always return move in gen moves, if you want then do action(move)
       * if online, send the server the action, get back the move

### integration workflow
* ==> games with simultaneous moves
  * game unions all possible moves from all outstanding players to move simultaneously, returns that as the valid_moves_list
  * when a player moves, their move is stored by the backend game into its accumulation buffer
    * when the last remaining player, of all those who move simultaneously, makes their move, the game processes the accumulation buffer and proceeds
  * move gen should be able to receive that it should only output moves for a certain players perspective, if simultaneous moves are about to happen
    * otherwise the same simultaneous move can be made with multiple orderings of the specific players moves
* ==> feature flags (struct with bitfield?) will include:
  * perfect information
  * random moves
  * hidden information
  * simultaneous moves (e.g. 2 players move at once, the order in which they decide on their move does not matter)
    * ordered simultaneous moves (e.g. 2 players move at once, but the moves available DO change if one player makes a move)
    * SM may be repr. by all moves being SM-once and then offer an "is-commutative" flag from every SM move gamestate
    * for normal, human vs human use case, just resend when denied for older from-state, no commutative moves
  * big moves (i.e. game requests space for moves >64bit, will get handled via requested size byte buffers)
  * supports id (zobrist state hash, requires that e.g. id()&0xFFFF is just as "unique" as a dedicated id16 would be)
  * supports evaluation (is f32 fine?)
  * supports random playouts (required because some games might never terminate, or will take unreasonably long to do so)
  * supports discretization, is this actually required?
  * binary state export (maybe make this a universal tagged thing and implementation specific, i.e. games can expose what format they support)
