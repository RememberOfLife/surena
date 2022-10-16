# surena API design
The surena project provides powerful APIs and utilities for creating all kinds of board games.  
All manner of games can be described by the general purpose game methods provided in [`game.h`](../includes/surena/game.h). This includes, but is not limited to, games containing:
* setup options
* randomness
* simultaneous moves
* hidden information
* (planned) scores (reward based results)
* (planned) teams
* (planned) legacies carrying over to future games
* (planned) custom and dynamic timecontrol stages and staging
* (planned) non-trivial draw/resign voting

To facilitate general usage with game agnostic tooling (e.g. visualization / AI) many of the more advanced features of the game method api are guarded behind feature flags. Additionally the game methods can optionally be enriched with features aiding in automatic processing like state ids, board evals and further internal methods.  
For more information on the provided engine methods API see [`engine.h`](../includes/surena/engine.h).

## the game methods
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
