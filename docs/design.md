# surena API design
The surena project provides powerful APIs and utilities for creating all kinds of board games.  
All manner of games can be described by the general purpose game methods provided in [`game.h`](../includes/surena/game.h). This includes, but is not limited to, games containing:
* setup options
* randomness
* simultaneous moves
* hidden information
* scores (reward based results)
* legacies carrying over to future games
* (planned) custom and dynamic timecontrol stages and staging
* (planned) teams
* (planned) non-trivial draw/resign voting

To facilitate general usage with game agnostic tooling (e.g. visualization / AI) many of the more advanced features of the game method api are guarded behind feature flags. Additionally the game methods can optionally be enriched with features aiding in automatic processing like state ids, board evals and further internal methods.  
For more information on the provided engine methods API see [`engine.h`](../includes/surena/engine.h).

## the game

### terminology
|term|description|example|
|---|---|---|
|random moves / randomness|Any type of event where the outcome is decided from a fixed set of results, by chance, offers random moves. This outcome may or may not be visible to all players.|Roll some dice in the open.|
|simultaneous moves|Any state from which more than one player can move offers simultaneous moves. These moves may or may not be commutative.|Chosen actions are revealed by both players at once.|
|hidden information|Any game where there exists any valid state in which any information is conceiled from any set of players contains hidden information.|A deck of cards shuffled at the start.||
|`player_id`|An 8-bit integer representing the id of a player participating in a game. For a game with N players, the ids are always numbered 1 to N. A maximum of 254 player ids can be assigned.||
|`PLAYER_NONE`|Special player id representing either none or all players. Never assigned to participants.||
|`PLAYER_RAND`|Special player id representing random chance. Non-discretized games use this to offer moves that decide the outcome of random events.||
|move|Moves represent state transitions on the game board and its internal state. A move that encodes an action is part of exactly that action set.|Moves as the union on actions and concrete moves.|
|action|Actions represent sets of moves, i.e. sets of concrete moves (action instances / informed moves). Every action can be encoded as a move.|Draw *some* card from a hidden pile. Roll the die (irrespective of outcome).|
|concrete move|For every action, the concrete moves it encompasses determine the outcome of the action move. Every concrete move can be encoded as a move or reduced to an action, i.e. a move.|Roll a specific number with a die.|
|options|Any information that the game requires to be available at time of creation, and which can not be changed for lifetime of the game.|Board sizes, player counts.|
|state|A set of facts that represents the current setup of the "board".|String equivalent of a screenshot of the board (and all players hands).|
|serialization|Comprehensive representation of the entire game as it exists. This includes options, legacy, state *and* any internal data that influences the game. I.e. a deserialized game has to behave *exactly* like the one it was serialized from would.|Raw binary stream.|
|legacy|Carry over information from a previous game of this type.|Parts of a card deck get replaced/changed/added/removed over the course of multiple games.|
|scores|Scoring mechanism, possibly used to determine single game results (winners). Accumulate over multiple games for multi-game results.|Victory/Penalty points.|

### game methods
Goals served by the game methods:
* enforce game rules (illegal moves can not be played)
* provide uniform access to common game actions (e.g. get moves and make moves until game is done)
* ability to replay (physical) games with and without hidden information and randomness
* api designed for easy use with general purpose game playing AI

//TODO

## examples
Examples for the more advanced features of the game methods api.

### options
//TODO

### public randomness
Two modes of operation:
* Undiscretized (Open)
  * Random moves are decided by a move from PLAYER_RAND.
* Discretized (Closed)
  * Random move variety is eliminated because the game has a discretization seed. PLAYER_RAND still moves, but there is only one move left.

For replaying (physical) games, open games are used. Make the appropriate randomly selected result move as `PLAYER_RAND`.

#### example 1 (flip coin)
In this example a coin is flipped to decide which player goes first. However, the same working principle can be applied to all kinds of public randomness, like rolling dice at the start of a turn.

After creation the game is undiscretized by default. Because the coin flip is the first thing that happens in the game, `players_to_move` returns `{PLAYER_RAND}`. After discretization with a seed, all randomness is removed from the game, the results are then pre-determined.
* **Open**:
  Moves with `get_concrete_moves`: `{COIN_HEAD, COIN_TAIL}`.
* **Closed**:
  Moves with `get_concrete_moves`: `{COIN_TAIL}`.   (because the discretization collapsed to tail in this example).  
  Note that the random player still has to move, although only a single move is available for them. This assures that random events are still separated semantically from the rest of the gameplay. E.g. a possible frontend can separate and animate all moves.
When any move has been made the game proceeds as it normally would, using the move to determine which random result happened.

#### example 2 (roll dice)
In this example we assume a game where, among other things, a player may choose to roll a dice (the result of which is then somehow used in gameplay). We assume at least 1 player, with the id`P1`.

In both open and closed games we start with: `players_to_move` returns `{P1}` and `get_concrete_moves` returns `{MOVE_1, ..., ROLL_DIE, ..., MOVE_N}`.
When player `P1` makes the move `ROLL_DIE`, in both open and closed games, the next `players_to_move` are `{PLAYER_RAND}`. Just like the coin, the results for `get_concrete_moves` differ only slightly:
* **Open**: `{DIE_1, DIE_2, DIE_3, DIE_4, DIE_5, DIE_6}`
* **Closed**: `{DIE_4}`

### hidden randomness
This is the easiest form of hidden information.  
In this example a deck of cards is shuffled at the beginning of the game and players draw from it both faceup and facedown (into their hand).

#### example 1 (shuffle a deck)
//TODO

#### example 2 (draw card faceup from shuffled deck)
//TODO

#### example 3 (draw card facedown from shuffled deck)
//TODO

#### example 4 (flip coin in secret)
//TODO want this?

### simultaneous moves
A more complicated form of hidden information, but very still mostly straight forward.  
ABC //TODO (with/without moved indicator for opponent)
//TODO types (no)sync,unordered,ordered,want_discard,reissue,never_discard

#### example 1 (gated)
//TODO all SM players have to move before game continues, no interplay between moves

#### example 2 (synced)
//TODO sync ctr is needed b/c moves might interplay

### revealing hidden information (e.g. play card from secret hand)
//TODO

### transforming hidden information (e.g. play card from secret hand *facedown*)
//TODO

### scores
//TODO

### teams
//TODO

### legacy
//TODO

### timecontrol
//TODO

### draw/resign
//TODO

## utilities
For implementation convenience, some general purpose APIs are provided:
* **base64**: encode/decode bytes to base64 and back
* **fast_prng**: small and fast seeded pseudo random number generator
* **noise**: easy to use noise functions to generate pseudo random numbers without the need to remember anything
* **raw_stream**: very basic primitive / string / blob stream writer to quickly put data in a buffer and get it out again
* **serialization**: layout based descriptive serializer for zero/copy/(de-)serialize/destroy on dynamic objects
