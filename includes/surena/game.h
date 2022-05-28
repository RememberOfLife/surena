#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "surena/util/semver.h"

static const uint64_t SURENA_GAME_API_VERSION = 6;

typedef uint32_t error_code;
// general purpose error codes
enum ERR {
    //TODO not_implemented
    ERR_OK = 0,
    ERR_STATE_UNRECOVERABLE,
    ERR_STATE_CORRUPTED,
    ERR_OUT_OF_MEMORY,
    ERR_FEATURE_UNSUPPORTED,
    ERR_STATE_UNINITIALIZED,
    ERR_INVALID_INPUT,
    ERR_INVALID_OPTIONS,
    ERR_UNSTABLE_POSITION,
    ERR_SYNC_CTR_MISMATCH,
    ERR_ENUM_DEFAULT_OFFSET, // not an error, start game method specific error enums at this offset
};
// returns NULL if the err is not a general error
const char* get_general_error_string(error_code err);

// anywhere a rng seed is use, SEED_NONE represents not using the rng
static const uint64_t SEED_NONE = 0;

// moves represent state transitions on the game board (and its internal state)
// actions represent sets of moves, i.e. sets of concrete moves (action instances / informed moves)
// every action can be encoded as a move
// a move that encodes an action is part of exactly that action set
// every concrete move can be encoded as a move
// every concrete move can be reduced to an action, i.e. a move
//TODO better name for "concrete_move"? maybe action instance / informed move
typedef uint64_t move_code;
static const move_code MOVE_NONE = UINT64_MAX;

typedef uint8_t player_id;
static const player_id PLAYER_NONE = 0x00;
static const player_id PLAYER_RAND = 0xFF;

typedef struct game_feature_flags_s {

    // bool perfect_information : 1; // needed?

    // if a board has a seed then if any random moves happen there will only ever be one move available (the rigged one)
    // a board can be seeded at any time using discretize
    // when a non seeded board offers random moves then all possible moves are available (according to the gathered info)
    // same result moves are treated as the same, i.e. 2 dice rolled at once will only offer 12 moves for their sum
    // probability distribution of the random moves is offered in get_concrete_move_probabilities
    bool random_moves : 1;

    //TODO document hidden info workflow with moves,concrete_moves,action_moves
    bool hidden_information : 1;

    //TODO document simultaneous move workflow for different kinds (no)sync,unordered,ordered,want_discard,reissue,never_discard
    bool simultaneous_moves : 1;

    // bool big_moves : 1; //TODO needs api support

    // bool supports_binary_state_export : 1; // here or as one of general purpose import/export format?

} game_feature_flags;

typedef struct buf_sizer_s {
    // options_str is valid after any options have been imported
    size_t options_str; // byte size
    // all below are valid after create
    size_t state_str; // byte size
    uint8_t player_count; // count, for n players, the player_ids are [1,n]
    uint8_t max_players_to_move; // player count
    uint32_t max_moves; // move count
    uint32_t max_actions; // move count
    uint8_t max_results; // player count
    size_t move_str; // byte size
    size_t print_str; // byte size
} buf_sizer;

typedef struct sync_data_s {
    void* data_start;
    void* data_end;
    uint8_t* player_start;
    uint8_t* player_end;
} sync_data;

typedef struct game_s game; // forward declare the game for the game methods

typedef struct game_methods_s {
    // str buf functions return the character size excluding the null terminator
    // the sizer member of the game struct defines all the required sizes and counts for buffer using functions

    // the game methods functions work ONLY on the data supplied to it in the game
    // i.e. they are threadsafe across multiple games, but not within one game instance
    // use of lookup tables and similar constant read only game external structures is ok

    // except where explicitly permitted, methods should never cause a crash or undefined behaviour
    // when in doubt return an error code representing an unusable state and force deconstruction
    // should report fails from malloc and similar calls

    // functions pointers for functions marked with "FEATURE: ..." are only valid if the feature flag is set for the game method
    // function pointers for functions marked with "OPTIONAL SUPPORT" may be NULL if the function is not supported
    // functions with "NOTE: avoid crashes" should place extra care on proper input parsing
    // where applicable: size refers to bytes, count refers to a number of elements of a particular type

    // where not otherwise noted: interfaces that return their required size/count are stable for the games specific options

    // the concatenation of game_name+variant_name+impl_name+version uniquely identifies this game method
    // minimum length of 1 character each, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* game_name;
    const char* variant_name;
    const char* impl_name;
    const semver version;
    const game_feature_flags features; // these will never change depending on options

    // the game method specific internal method struct, NULL if not available
    // use the {base,variant,impl} name to make sure you know what this will be
    // e.g. these would expose get_cell and set_cell on a tictactoe board to enable rw-access to the state 
    const void* internal_methods;

    // returns the error string representing the error code
    // returns NULL if the error code is not a valid error on this game method
    // the string is still owned by the game method backend, do not free it
    const char* (*get_error_string)(error_code err);

    // OPTIONAL SUPPORT
    // import the internal options struct into the self
    // the game mallocs the internally kept options itself
    // if options_struct is NULL then the default options are loaded
    // importing any options while there are already options loaded is undefined behaviour
    error_code (*import_options_bin)(game* self, void* options_struct);

    // OPTIONAL SUPPORT
    // same as import_options_bin but imports the options from a supplied string
    // if str is NULL then the default options are loaded
    error_code (*import_options_str)(game* self, const char* str);

    // OPTIONAL SUPPORT
    // write this games options to a universal options string
    // returns the length of the options string written, 0 if failure, excluding null character
    error_code (*export_options_str)(game* self, size_t* ret_size, char* str);

    // construct and initialize a new game specific data object into self
    // options, if any, have to already be set on the supplied game
    // returns non ok if initialization failed because of invalid options
    // if this fails then no game data should be allocated
    error_code (*create)(game* self);

    // deconstruct and release any (complex) game specific data, if it has been created already
    // same for options specific data, if it exists
    error_code (*destroy)(game* self);

    // fills clone_target with a deep clone of the self game state
    // undefined behaviour is self == clone_target
    error_code (*clone)(game* self, game* clone_target);

    // deep clone the game state of other into self
    // other is restricted to already created games using the same options, otherwise undefined behaviour
    // undefined behaviour is self == other
    error_code (*copy_from)(game* self, game* other);

    // returns true iff self and other are perceived equal state (by the game method)
    // e.g. this includes move counters in chess, but not any exchangable backend data structures
    error_code (*compare)(game* self, game* other, bool* ret_equal);

    // load the game state from the given string, beware this may be called on running games
    // if str is NULL then the initial position is loaded
    // errors while parsing are handled (NOTE: avoid crashes)
    // when this fails the underlying object is left in an empty state
    error_code (*import_state)(game* self, const char* str);

    //=====
    // all functions below here may crash if used on a game that is:
    // (partially) uninitialized
    //=====

    // writes the game state to a universal state string
    // returns the length of the state string written, 0 if failure, excluding null character
    error_code (*export_state)(game* self, size_t* ret_size, char* str);

    // writes the player ids to move from this state
    // writes PLAYER_RAND if the current move branch is decided by randomness
    // writes no ids if the game is over
    // returns the number of ids written
    // written player ids are not explicitly ordered
    error_code (*players_to_move)(game* self, uint8_t* ret_count, player_id* players);

    // writes the available moves for the player from this position
    // writes no moves if the game is over or the player is not to move
    error_code (*get_concrete_moves)(game* self, player_id player, uint32_t* ret_count, move_code* moves);

    // FEATURE: random_moves
    // writes the probabilities [0,1] of each avilable move in get_concrete_moves
    // order is the same as get_concrete_moves
    // writes no moves if the available moves are not random moves
    error_code (*get_concrete_move_probabilities)(game* self, player_id player, uint32_t* ret_count, float* move_probabilities);

    // OPTIONAL SUPPORT
    // writes the available moves for the player from this position
    // writes no moves if the game is over
    // moves must be at least of count get_moves(NULL)
    // moves written are ordered according to the game method, from perceived strongest to weakest
    error_code (*get_concrete_moves_ordered)(game* self, player_id player, uint32_t* ret_count, move_code* moves);

    // FEATURE: random_moves || hidden_information || simultaneous_moves
    // writes the available action moves for the player from this position
    // writes no action moves if there are no action moves available
    error_code (*get_actions)(game* self, player_id player, uint32_t* ret_count, move_code* moves);

    // returns whether or not this move would be legal to make on the current state
    // equivalent to the fallback check of: move in list of get_moves?
    // should be optimized by the game method if possible
    // REAL commutative moves in a situation for simultaneous moves can be performed on dissimilar sync ctrs, at the games discretion
    // moves do not have to be valid
    error_code (*is_legal_move)(game* self, player_id player, move_code move, uint32_t sync_ctr);

    // FEATURE: random_moves || hidden_information || simultaneous_moves
    // returns the action representing the information set transformation of the (concrete) LEGAL move (action instance)
    // if move is already an action then it is directly returned unaltered
    error_code (*move_to_action)(game* self, move_code move, move_code* ret_action);

    // FEATURE: random_moves || hidden_information || simultaneous_moves
    // convenience wrapper
    // returns true if the move represents an action
    error_code (*is_action)(game* self, move_code move, bool* ret_is_action);

    // make the legal move on the game state as player
    // undefined behaviour if move is not within the get_moves list for this player (MAY CRASH)
    // undefined behaviour if player is not within players_to_move (MAY CRASH)
    error_code (*make_move)(game* self, player_id player, move_code move);

    // writes the result (winning) players
    // writes no ids if the game is not over yet or there are no result players
    // returns the number of ids written
    error_code (*get_results)(game* self, uint8_t* ret_count, player_id* players);

    // OPTIONAL SUPPORT
    // state id, should be as conflict free as possible
    // commutatively the same for equal board states
    // optimally, any n bits of this should be functionally equivalent to a dedicated n-bit id 
    error_code (*id)(game* self, uint64_t* ret_id);

    // OPTIONAL SUPPORT
    // evaluates the state comparatively against others
    // higher evaluations correspond to a (game method) perceived better position for the player
    // states with multiple players to move are inherently unstable, their evaluations are worthless
    error_code (*eval)(game* self, player_id player, float* ret_eval);

    // FEATURE: hidden_information
    // seed the game and collapse the hidden information and all that was inferred via play
    // the resulting game state assigns possible values to all previously unknown information
    // all random moves from here on will be pre-rolled from this seed
    error_code (*discretize)(game* self, uint64_t seed);

    // OPTIONAL SUPPORT
    // playout the game by performing random moves for all players until it is over
    // the random moves selected are determined by the seed
    error_code (*playout)(game* self, uint64_t seed);

    // FEATURE: hidden_information || simultaneous_moves
    // removes all but certain players hidden information from the internal state
    // if PLAYER_RAND is not in players, then the seed (and all internal hidden information) is redacted as well
    error_code (*redact_keep_state)(game* self, uint8_t count, player_id* players);

    // OPTIONAL SUPPORT
    // one sync data always describes the data to be sent to the given player array to sync up their state
    // multiple sync data struct incur multiple events, i.e. multiple import_sync_data calls
    // the returned sync data is valid until release_sync_data is called
    // undefined behaviour if release_sync_data was not called before the next export_sync_data usage
    error_code (*export_sync_data)(game* self, sync_data** sync_data_start, sync_data** sync_data_end); // everything owned by the game

    // OPTIONAL SUPPORT
    // release the sync data, should match every export_sync_data
    error_code (*release_sync_data)(game* self, sync_data* sync_data_start, sync_data* sync_data_end);

    // OPTIONAL SUPPORT
    // import a sync data block received from another (more knowing) instance of this game (e.g. across the network)
    error_code (*import_sync_data)(game* self, void* data_start, void* data_end);

    // returns the game method and state specific move code representing the move string at this position for this player
    // if the move string does not represent a valid move this returns MOVE_NONE (NOTE: avoid crashes)
    // if player is PLAYER_NONE the supplied move string has to be a universal move
    error_code (*get_move_code)(game* self, player_id player, const char* str, move_code* ret_move);

    // writes the state specific move string representing the game specific move code for this player
    // if player is PLAYER_NONE then a universal move string is given, a game is allowed to only support this mode of operation
    // returns number of characters written to string buffer on success, excluding null character
    error_code (*get_move_str)(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);

    // OPTIONAL SUPPORT
    // debug print the game state into the str_buf
    // returns the number of characters written to string buffer on success, excluding null character
    error_code (*debug_print)(game* self, size_t* ret_size, char* str_buf);

} game_methods;

struct game_s {
    uint32_t sync_ctr; // inc by one everytime a move is made, the game method does this itself
    uint32_t padding; //TODO use this properly for something
    void* options; // owned by the game method
    void* data; // owned by the game method
    buf_sizer sizer;
    const game_methods* methods;
};

#ifdef __cplusplus
}
#endif
