#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rosalia/semver.h"
#include "rosalia/serialization.h"
#include "rosalia/timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint64_t SURENA_GAME_API_VERSION = 25;

typedef uint32_t error_code;

// general purpose error codes
enum ERR {
    ERR_OK = 0,
    // ERR_NOK, //TODO might want this to show that input was in fact valid, but the result is just false, i.e. legal move and engine game support
    ERR_STATE_UNRECOVERABLE,
    ERR_STATE_CORRUPTED,
    ERR_OUT_OF_MEMORY,
    ERR_FEATURE_UNSUPPORTED,
    ERR_MISSING_HIDDEN_STATE,
    ERR_INVALID_INPUT,
    ERR_INVALID_PLAYER,
    ERR_INVALID_MOVE,
    ERR_INVALID_OPTIONS,
    ERR_INVALID_LEGACY,
    ERR_INVALID_STATE,
    ERR_UNSTABLE_POSITION,
    ERR_SYNC_COUNTER_MISMATCH,
    ERR_SYNC_COUNTER_IMPOSSIBLE_REORDER,
    ERR_RETRY, // retrying the same call again may yet still work
    ERR_CUSTOM_ANY, // unspecified custom error, check get_last_error for a detailed string
    ERR_ENUM_DEFAULT_OFFSET, // not an error, start game method specific error enums at this offset
};

// returns not_general if the err is not a general error
const char* get_general_error_string(error_code err, const char* fallback);
// instead of returning an error code, one can return rerrorf which automatically manages fmt string buffer allocation for the error string
// call rerrorf with fmt=NULL to free (*pbuf)
error_code rerror(char** pbuf, error_code ec, const char* str, const char* str_end);
error_code rerrorf(char** pbuf, error_code ec, const char* fmt, ...);
error_code rerrorvf(char** pbuf, error_code ec, const char* fmt, va_list args);

// anywhere a rng seed is use, SEED_NONE represents not using the rng
static const uint64_t SEED_NONE = 0;

// moves represent state transitions on the game board (and its internal state)
// actions represent sets of moves, i.e. sets of concrete moves (action instances / informed moves)
// every action can be encoded as a move
// a move that encodes an action is part of exactly that action set
// every concrete move can be encoded as a move
// every concrete move can be reduced to an action, i.e. a move
// e.g. flipping a coin is an action (i.e. flip the coin) whereupon the PLAYER_RAND decides the outcome of the flip
// e.g. laying down a hidden hand card facedown (i.e. keeping it hidden) is informed to other players through the action of playing *some card* from hand facedown, but the player doing it chooses one of the concrete moves specifying which card to play (as they can see their hand)
//TODO better name for "concrete_move"? maybe action instance / informed move

typedef uint64_t move_code;
static const move_code MOVE_NONE = UINT64_MAX;

typedef union move_data_u {
    union {
        move_code code; // FEATURE: !big_moves ; use MOVE_NONE for empty
        size_t len; // FEATURE: big_moves ; if data is NULL and this is 0 then the big move is empty
    } cl;

    uint8_t* data; // FEATURE: big_moves ; MUST always be NULL for non big moves or empty big moves
} move_data; // unindexed move realization, switch by big_move feature flag

custom_serializer_t ls_move_data_serializer;

typedef struct move_data_sync_s {
    move_data md; // see unindexed move move_data
    uint64_t sync_ctr; // sync ctr from where this move is intended to be "made" (on the game) //TODO might rename to move_ctr
} move_data_sync;

extern const serialization_layout sl_move_data_sync[];

static const uint64_t SYNC_CTR_DEFAULT = 0; // this is already a valid sync ctr value (usually first for a fresh game)

//TODO !maybe! force every move to also include a player + move_to_player in the game_methods

typedef uint8_t player_id;
static const player_id PLAYER_NONE = 0x00;
static const player_id PLAYER_RAND = 0xFF;

typedef struct game_feature_flags_s {

    bool error_strings : 1;

    // options are passed together with the creation of the game data
    bool options : 1;

    // this is a binary serialization of the game, must be absolutely accurate representation of the game state
    bool serializable : 1;

    bool legacy : 1;

    // if a board has a seed then if any random moves happen there will only ever be one move available (the rigged one)
    // a board can be seeded at any time using discretize
    // when a non seeded board offers random moves then all possible moves are available (according to the gathered info)
    // same result moves are treated as the same, i.e. 2 dice rolled at once will only offer 12 moves for their sum
    // probability distribution of the random moves is offered in get_concrete_move_probabilities
    bool random_moves : 1;

    bool hidden_information : 1;

    //TODO extra flag for sync data?

    // remember: the game owner must guarantee total move order on all moves!
    bool simultaneous_moves : 1;

    //TODO extra flag for sync_ctr?

    // in a game that uses this feature flag, ALL moves are big moves
    // big moves use move_data.big, otherwise use move_data.code
    bool big_moves : 1;

    bool move_ordering : 1;

    bool scores : 1;

    bool id : 1;

    bool eval : 1;

    bool playout : 1;

    bool print : 1;

    // bool time : 1;

} game_feature_flags;

typedef struct sync_data_s {
    // end pointers points to the last byte/elem not included in the segment
    uint8_t player_c;
    uint8_t* players;
    blob b;
} sync_data;

typedef enum __attribute__((__packed__)) GAME_INIT_SOURCE_TYPE_E {
    GAME_INIT_SOURCE_TYPE_DEFAULT = 0, // create a default game with the default options and default initial state
    GAME_INIT_SOURCE_TYPE_STANDARD, // create a game from some options, legacy and initial state
    GAME_INIT_SOURCE_TYPE_SERIALIZED, // (re)create a game from a serialization buffer
    GAME_INIT_SOURCE_TYPE_COUNT,
    GAME_INIT_SOURCE_TYPE_SIZE_MAX = UINT8_MAX,
} GAME_INIT_SOURCE_TYPE;

typedef struct game_init_standard_s {
    const char* opts; // FEATURE: options ; may be NULL to use default
    const char* legacy; // FEATURE: legacy ; may be NULL to use none
    const char* state; // may be null to use default
} game_init_standard;

typedef struct game_init_serialized_s {
    // beware that the data given through serialized is UNTRUSTED and shoudl be thouroughly checked for consistency
    blob b;
} game_init_serialized;

typedef struct game_init_s {
    GAME_INIT_SOURCE_TYPE source_type;

    union {
        game_init_standard standard; // use options, legacy, initial_state
        game_init_serialized serialized; // FEATURE: serialize ; use the given byte buffer to create the game data, NULL buffers are invalid
    } source;
} game_init;

void game_init_create_standard(game_init* init_info, const char* opts, const char* legacy, const char* state);

void game_init_create_serialized(game_init* init_info, blob b);

extern const serialization_layout sl_game_init_info[];

typedef struct timectlstage_s timectlstage; //TODO better name?

typedef struct game_s game; // forward declare the game for the game methods

///////
// game method functions, usage comments are by the typedefs where the arguments are

// client-server multiplay move protocol:
// * client: sync board and play board
//   * play move on play board and send to server (it will only become valid on the sync board once the server sends it back)
//   * everything that arrives from server is made on sync board, if play board is desynced then reset it using the sync board
// * server: on move from client
//   * check move is legal -> convert move to action -> make move -> get sync data if any
//   * send to every player (in this order!!): sync data for all players this client serves + the action made (except the player who made the action, they get the true move)
// * client: on (sync data +) move from server
//   * import sync data to sync board
//   * make move on sync board
//   * (if req'd rollback and) update play board to be up to date with sync board
// * to send just sync data to other players without noticing, make a move that maps to the null move action, it gets dropped by the server
//   * //TODO alternative to this is to keep a sync ctr offset for every client, maybe do this in the future

// FEATURE: error_strings
// returns the error string complementing the most recent occured error (i.e. only available if != ERR_OK)
// returns NULL if there is no error string available for this error
// the string is still owned by the game method backend, do not free it
typedef const char* get_last_error_gf_t(game* self);

// construct and initialize a new game specific data object into self
// only one game can be created into a self at any one time (destroy it again to reuse with create)
// every create MUST ALWAYS be matched with a call to destroy
// !!! even if create fails, the game has to be destroyed before retrying create or releasing the self
// get_last_error will, if supported, be valid to check even if create fails (which is why destroy is required)
// the init_info provides details on what source is used, if any, and details about that source
// after creation, if successful, the game is always left in a valid and ready to use state
// the init_info is only read by the game, it is still owned externally, and no references are created during create
typedef error_code create_gf_t(game* self, game_init* init_info);

// deconstruct and release any (complex) game specific data, if it has been created already
// same for options specific data, if it exists
// even if this fails, there will never be an error string available (should generally only fail on corrupt games anyway)
typedef error_code destroy_gf_t(game* self);

// fills clone_target with a deep clone of the self game state
// undefined behaviour if self == clone_target
typedef error_code clone_gf_t(game* self, game* clone_target);

// deep clone the game state of other into self
// other is restricted to already created games using the same options, otherwise undefined behaviour
// undefined behaviour if self == other
typedef error_code copy_from_gf_t(game* self, game* other);

// returns true iff self and other are in a behaviourally identical state (concerning the game methods)
// e.g. this includes move counters (and 3fold repition histories) in e.g. chess, but not any exchangable backend data structures
typedef error_code compare_gf_t(game* self, game* other, bool* ret_equal);

// FEATURE: options
// write this games options to a universal options string and returns a read only pointer to it
// returns the length of the options string written, 0 if failure, excluding null character
// player specifies the perspective player from which this is done (relevant for feature: hidden_information) (PLAYER_NONE is omniscient pov)
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code export_options_gf_t(game* self, player_id player, size_t* ret_size, const char** ret_str);

// returns the number of players participating in this game
typedef error_code player_count_gf_t(game* self, uint8_t* ret_count);

// writes the game state to a universal state string and returns a read only pointer to it
// returns the length of the state string written, 0 if failure, excluding null character
// player specifies the perspective player from which this is done (relevant for feature: hidden_information) (PLAYER_NONE is omniscient pov)
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code export_state_gf_t(game* self, player_id player, size_t* ret_size, const char** ret_str);

// load the game state from the given string, beware this may be called on running games
// if str is NULL then the initial position is loaded
// errors while parsing are handled (NOTE: avoid crashes)
// when this fails self is left unmodified from its previous state
typedef error_code import_state_gf_t(game* self, const char* str);

// FEATURE: serializable
// writes the game state and options to a game specific raw byte representation that is absolutely accurate to the state of the game and returns a read only pointer to it
// player specifies the perspective player from which this is done (relevant for feature: hidden_information) (PLAYER_NONE is omniscient pov)
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code serialize_gf_t(game* self, player_id player, const blob** ret_blob);

// writes the player ids to move from this state and returns a read only pointer to them
// writes PLAYER_RAND if the current move branch is decided by randomness
// writes no ids if the game is over
// returns the number of ids written
// written player ids are ordered from lowest to highest
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code players_to_move_gf_t(game* self, uint8_t* ret_count, const player_id** ret_players);

// writes the available moves for the player from this position and returns a read only pointer to them
// writes no moves if the game is over or the player is not to move
// if the game uses moves at this position, which can not be feasibly listed it can return ERR_UNSTABLE_POSITION to signal this
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_concrete_moves_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// FEATURE: random_moves
// writes the probabilities [0,1] of each avilable move in get_concrete_moves and returns a read only pointer to them
// order is the same as get_concrete_moves
// writes no moves if the available moves are not random moves
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_concrete_move_probabilities_gf_t(game* self, player_id player, uint32_t* ret_count, const float** ret_move_probabilities);

// FEATURE: move_ordering
// writes the available moves for the player from this position and returns a read only pointer to them
// writes no moves if the game is over
// moves must be at least of count get_moves(NULL)
// moves written are ordered according to the game method, from perceived strongest to weakest
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_concrete_moves_ordered_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// writes the available action moves for the player from this position and returns a read only pointer to them
// writes no action moves if there are no action moves available
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_actions_gf_t(game* self, player_id player, uint32_t* ret_count, const move_data** ret_moves);

// returns whether or not this move would be legal to make on the current state
// for non SM games (or those that are totally ordered) equivalent to the fallback check of: move in list of get_moves?
// should be optimized by the game method if possible
// moves do not have to be valid
typedef error_code is_legal_move_gf_t(game* self, player_id player, move_data_sync move);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// returns the action representing the information set transformation of the (concrete) LEGAL move (action instance)
// if move is already an action then it is directly returned unaltered
// if this returns the null move action, the action should not be sent to ther clients and the sync_ctr in the game no incremented after making the move
// use e.g. on server, send out only the action to client which are not controlling the player that sent the move
typedef error_code move_to_action_gf_t(game* self, player_id player, move_data_sync move, move_data_sync* ret_action);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// convenience wrapper
// returns true if the move represents an action for this player
typedef error_code is_action_gf_t(game* self, player_id player, move_data_sync move, bool* ret_is_action);

// make the legal move on the game state as player
// undefined behaviour if move is not within the get_moves list for this player (MAY CRASH)
// undefined behaviour if player is not within players_to_move (MAY CRASH)
// after a move is made, if it's action was not the null move (if applicable), the sync_ctr in the game must be incremented!
typedef error_code make_move_gf_t(game* self, player_id player, move_data_sync move);

// writes the result (winning) players and returns a read only pointer to them
// writes no ids if the game is not over yet or there are no result players
// returns the number of ids written
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_results_gf_t(game* self, uint8_t* ret_count, const player_id** ret_players);

// FEATURE: legacy
// while the game is running exports the used legacy string at creation, or after the game is finished the resulting legacy to be used for the next game and returns a read only pointer to it
// player specifies the perspective player from which this is done (relevant for feature: hidden_information) (PLAYER_NONE is omniscient pov)
// use PLAYER_NONE to export all the hidden info for everything
// NOTE: this does not include the used options, save them separately for reuse together with this in a future game
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code export_legacy_gf_t(game* self, player_id player, size_t* ret_size, const char** ret_str);

// FEATURE: scores
// available after creation for the entire lifetime of the game and returns a read only pointer to it
// writes the scores of players, as accumulated during this game only, to scores, and the respective players to players
//TODO is int32 enough? need more complex? or float?
//TODO offer some default score?
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_scores_gf_t(game* self, size_t* ret_count, player_id* players, const int32_t** ret_scores);

// FEATURE: id
// state id, should be as conflict free as possible
// commutatively the same for equal board states
// optimally, any n bits of this should be functionally equivalent to a dedicated n-bit id
//TODO usefulness breaks down quickly for hidden info games? want perspective too?
typedef error_code id_gf_t(game* self, uint64_t* ret_id);

// FEATURE: eval
// evaluates the state comparatively against others
// higher evaluations correspond to a (game method) perceived better position for the player
// states with multiple players to move can be unstable, if their evaluations are worthless use ERR_UNSTABLE_POSITION to signal this
typedef error_code eval_gf_t(game* self, player_id player, float* ret_eval);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// seed the game and collapse the hidden information and all that was inferred via play
// the resulting game state assigns possible values to all previously unknown information
// all random moves from here on will be pre-rolled from this seed
typedef error_code discretize_gf_t(game* self, uint64_t seed);

// FEATURE: playout
// playout the game by performing random moves for all players until it is over
// the random moves selected are determined by the seed
// here SEED_NONE can not be used
typedef error_code playout_gf_t(game* self, uint64_t seed);

// FEATURE: random_moves || hidden_information || simultaneous_moves
// removes all but certain players hidden and public information from the internal state
// if PLAYER_RAND is not in players, then the seed (and all internal hidden information) is redacted as well
typedef error_code redact_keep_state_gf_t(game* self, uint8_t count, const player_id* players);

// FEATURE: hidden_information || simultaneous_moves
// one sync data always describes the data to be sent to the given player array to sync up their state
// multiple sync data structs incur multiple events, i.e. multiple import_sync_data calls
// use this after making a move to test what sync data should be sent to which player clients before the action
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code export_sync_data_gf_t(game* self, uint32_t* ret_count, const sync_data** ret_sync_data);

// FEATURE: hidden_information || simultaneous_moves
// import a sync data block received from another (more knowing) instance of this game (e.g. across the network)
typedef error_code import_sync_data_gf_t(game* self, blob b);

// returns the game method and state specific move data representing the move string at this position for this player
// if the move string does not represent a valid move this returns MOVE_NONE (or for big moves len==0 data==NULL) (NOTE: avoid crashes)
// if player is PLAYER_NONE assumes current player to move
// this should accept at least the string given out by get_move_str, but is allowed to accept more strings as well
typedef error_code get_move_data_gf_t(game* self, player_id player, const char* str, move_data_sync* ret_move);

// writes the game method and state specific move string representing the move data for this player and returns a read only pointer to it
// if player is PLAYER_NONE assumes current player to move
// returns number of characters written to string buffer on success, excluding null character
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code get_move_str_gf_t(game* self, player_id player, move_data_sync move, size_t* ret_size, const char** ret_str);

// FEATURE: print
// prints the game state into the str_buf and returns a read only pointer to it
// returns the number of characters written to string buffer on success, excluding null character
// player specifies the perspective player from which this is done (relevant for feature: hidden_information) (PLAYER_NONE is omniscient pov)
// the returned ptr is valid until the next call on this game, undefined behaviour if used after;  it is still owned by the game
typedef error_code print_gf_t(game* self, player_id player, size_t* ret_size, const char** ret_str);

// FEATURE: time
// informs the game that the game.time timestamp has moved
// the game may modify timectlstages, moves available, players to move and even end the game; so any caches of those are now potentially invalid
// if sleep_duration is zero (//TODO want helper?) the game does not request a wakeup
// otherwise, if nothing happened in between, time_ellapsed should be called at the latest after this duration
// typedef error_code time_ellapsed_gf_t(game* self, timestamp* sleep_duration);

typedef struct game_methods_s {
    // after any function was called on a game object, it must be destroyed before it can be safely released

    // the game methods functions work ONLY on the data supplied to it in the game
    // i.e. they are threadsafe across multiple games, but not within one game instance
    // use of lookup tables and similar constant read only game external structures is ok

    // except where explicitly permitted, methods should never cause a crash or undefined behaviour
    // when in doubt return an error code representing an unusable state and force deconstruction
    // also it is encouraged to report fails from malloc and similar calls

    // function pointers for functions marked with "FEATURE: ..." are valid if and only if the feature flag condition is met for the game method
    // functions with "NOTE: avoid crashes" should place extra care on proper input parsing
    // where applicable: size refers to bytes (incl. zero terminator for strings), count refers to a number of elements of a particular type

    // the concatenation of game_name+variant_name+impl_name+version uniquely identifies these game methods
    // minimum length of 1 character each, with allowed character set:
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" additionally '-' and '_' but not at the start or end
    const char* game_name;
    const char* variant_name;
    const char* impl_name;
    const semver version;
    const game_feature_flags features; // these will never change depending on options (e.g. if randomness only really happens due to a specific option, the whole game methods are always marked as containing randomness)

    // the game method specific internal method struct, NULL if not available
    // use the {base,variant,impl} name and version to make sure you know exactly what this will be
    // e.g. these would expose get_cell and set_cell on a tictactoe board to enable rw-access to the state
    const void* internal_methods;

    // all the member function pointers using the typedefs from above
    get_last_error_gf_t* get_last_error;
    create_gf_t* create;
    destroy_gf_t* destroy;
    clone_gf_t* clone;
    copy_from_gf_t* copy_from;
    compare_gf_t* compare;
    export_options_gf_t* export_options;
    player_count_gf_t* player_count;
    export_state_gf_t* export_state;
    import_state_gf_t* import_state;
    serialize_gf_t* serialize;
    players_to_move_gf_t* players_to_move;
    get_concrete_moves_gf_t* get_concrete_moves;
    get_concrete_move_probabilities_gf_t* get_concrete_move_probabilities;
    get_concrete_moves_ordered_gf_t* get_concrete_moves_ordered;
    get_actions_gf_t* get_actions;
    is_legal_move_gf_t* is_legal_move;
    move_to_action_gf_t* move_to_action;
    is_action_gf_t* is_action;
    make_move_gf_t* make_move;
    get_results_gf_t* get_results;
    export_legacy_gf_t* export_legacy;
    get_scores_gf_t* get_scores;
    id_gf_t* id;
    eval_gf_t* eval;
    discretize_gf_t* discretize;
    playout_gf_t* playout;
    redact_keep_state_gf_t* redact_keep_state;
    export_sync_data_gf_t* export_sync_data;
    import_sync_data_gf_t* import_sync_data;
    get_move_data_gf_t* get_move_data;
    get_move_str_gf_t* get_move_str;
    print_gf_t* print;

} game_methods;

struct game_s {
    const game_methods* methods;
    void* data1; // owned by the game method
    void* data2; // owned by the game method
    uint64_t sync_ctr;

    // FEATURE: time
    // representation of the "current time" at function invocation, use to determine relative durations
    // the only allowed changes are monotonic increases (incl. no change)
    // timestamp time;

    // FEATURE: time
    // every participating player X has their stage at timectlstages[X], [0] is reserved
    // stages can change entirely after every game function call
    // if the game does not support / use timectl then this can be managed externally //TODO timectl helper api
    // timectlstage* timectlstages;
};

typedef enum TIMECTLSTAGE_TYPE_E {
    TIMECTLSTAGE_TYPE_NONE = 0,
    TIMECTLSTAGE_TYPE_TIME, // standard static time limit
    TIMECTLSTAGE_TYPE_BONUS, // add a persistent bonus AFTER every move
    TIMECTLSTAGE_TYPE_DELAY, // transient delay at the start of a move BEFORE the clock starts counting down, unused delay time is lost
    TIMECTLSTAGE_TYPE_BYO, // available time resets every move, timing out transfers to the next time control, can also be used for correspondence
    TIMECTLSTAGE_TYPE_UPCOUNT, // time counts upwards
    TIMECTLSTAGE_TYPE_COUNT,
} TIMECTLSTAGE_TYPE;

struct timectlstage_s {
    timectlstage* next_stage; //TODO want ALL stages? even past ones?
    TIMECTLSTAGE_TYPE type;
    bool discard_time; // if true: sets the remaining time to 0 before applying this time control
    bool chain; // for byo, if true: this and the previous time controls CHAIN up until a BYO will share a move counter for advancing to the end of the BYO chain
    uint32_t stage_time; // ms time available in this stage
    uint32_t stage_mod; // bonus/delay
    uint32_t stage_moves; // if > 0: moves to next time control
    uint32_t time; // ms left/accumulated for this player
    uint32_t moves; // already played moves with this time control
    const char* desc; // description
};

// game function wrap, use this instead of directly calling the game methods
const char* game_gname(game* self); // game name
const char* game_vname(game* self); // variant name
const char* game_iname(game* self); // impl name
const semver game_version(game* self);
const game_feature_flags game_ff(game* self);
get_last_error_gf_t game_get_last_error;
create_gf_t game_create;
destroy_gf_t game_destroy;
clone_gf_t game_clone;
copy_from_gf_t game_copy_from;
compare_gf_t game_compare;
export_options_gf_t game_export_options;
player_count_gf_t game_player_count;
export_state_gf_t game_export_state;
import_state_gf_t game_import_state;
serialize_gf_t game_serialize;
players_to_move_gf_t game_players_to_move;
get_concrete_moves_gf_t game_get_concrete_moves;
get_concrete_move_probabilities_gf_t game_get_concrete_move_probabilities;
get_concrete_moves_ordered_gf_t game_get_concrete_moves_ordered;
get_actions_gf_t game_get_actions;
is_legal_move_gf_t game_is_legal_move;
move_to_action_gf_t game_move_to_action;
is_action_gf_t game_is_action;
make_move_gf_t game_make_move;
get_results_gf_t game_get_results;
export_legacy_gf_t game_export_legacy;
get_scores_gf_t game_get_scores;
id_gf_t game_id;
eval_gf_t game_eval;
discretize_gf_t game_discretize;
playout_gf_t game_playout;
redact_keep_state_gf_t game_redact_keep_state;
export_sync_data_gf_t game_export_sync_data;
import_sync_data_gf_t game_import_sync_data;
get_move_data_gf_t game_get_move_data;
get_move_str_gf_t game_get_move_str;
print_gf_t game_print;
// extra utility for game funcs
bool game_e_move_is_none(game* self, move_data move);
bool game_e_move_sync_is_none(game* self, move_data_sync move);
move_data game_e_move_copy(game* self, move_data move);
move_data_sync game_e_move_sync_copy(game* self, move_data_sync move);
move_data_sync game_e_move_make_sync(game* self, move_data move);

error_code grerrorf(game* self, error_code ec, const char* fmt, ...); // game internal rerrorf: if your error string is self->data2 use this as a shorthand

#ifdef __cplusplus
}
#endif
