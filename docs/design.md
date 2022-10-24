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

### game methods
Goals served by the game methods:
* enforce game rules (illegal moves can not be played)
* provide uniform access to common game actions (e.g. get moves and make moves until game is done)
* ability to replay physical games with and without hidden information and randomness
* api designed for easy use with general purpose game playing AI

//TODO

## examples
Examples for the more advanced features of the game methods api.

### options
//TODO

### public randomness (flip coin)
//TODO

### hidden randomness (flip coin in secret)
//TODO

### simultaneous moves (with/without moved indicator for opponent)
//TODO

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
