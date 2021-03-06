#include <array>
#include <bitset>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "surena/util/noise.hpp"
#include "surena/util/semver.h"
#include "surena/game_gftypes.h"
#include "surena/game.h"

#include "surena/games/havannah.h"

namespace {
    
    // general purpose helpers for opts, data, errors

    error_code _rerrorf(game* self, error_code ec, const char* fmt, ...)
    {
        if (self->data2 == NULL) {
            self->data2 = malloc(1024); //TODO correct size from where?
        }
        va_list args;
        va_start(args, fmt);
        vsprintf((char*)self->data2, fmt, args);
        va_end(args);
        return ec;
    }
    
    typedef havannah_options opts_repr;

    struct data_repr {
        opts_repr opts;

        int remaining_tiles;
        HAVANNAH_PLAYER current_player;
        HAVANNAH_PLAYER winning_player;
        uint8_t next_graph_id;
        //TODO just allocate map and vector completely
        std::unordered_map<uint8_t, havannah_graph> graph_map;
        // assuming a flat topped board this is [letter * num]
        // such that num goes vertically on the left downwards
        // and letter goes ascending horizontally towards the right
        std::vector<std::vector<havannah_tile>> gameboard;
    };

    opts_repr& _get_opts(game* self)
    {
        return ((data_repr*)(self->data1))->opts;
    }

    data_repr& _get_repr(game* self)
    {
        return *((data_repr*)(self->data1));
    }

    // forward declare everything to allow for inlining at least in this unit
    const char* _get_last_error(game* self);
    error_code _create_with_opts_str(game* self, const char* str);
    error_code _create_with_opts_bin(game* self, void* options_struct);
    error_code _create_default(game* self);
    error_code _export_options_str(game* self, size_t* ret_size, char* str);
    GF_UNUSED(get_options_bin_ref);
    error_code _destroy(game* self);
    error_code _clone(game* self, game* clone_target);
    error_code _copy_from(game* self, game* other);
    error_code _compare(game* self, game* other, bool* ret_equal);
    error_code _import_state(game* self, const char* str);
    error_code _export_state(game* self, size_t* ret_size, char* str);
    error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players);
    error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves);
    GF_UNUSED(get_concrete_move_probabilities);
    GF_UNUSED(get_concrete_moves_ordered);
    GF_UNUSED(get_actions);
    error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync);
    GF_UNUSED(move_to_action);
    GF_UNUSED(is_action);
    error_code _make_move(game* self, player_id player, move_code move);
    error_code _get_results(game* self, uint8_t* ret_count, player_id* players);
    GF_UNUSED(get_sync_counter);
    GF_UNUSED(id);
    GF_UNUSED(eval);
    GF_UNUSED(discretize);
    error_code _playout(game* self, uint64_t seed);
    GF_UNUSED(redact_keep_state);
    GF_UNUSED(export_sync_data);
    GF_UNUSED(release_sync_data);
    GF_UNUSED(import_sync_data);
    error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move);
    error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf);
    error_code _debug_print(game* self, size_t* ret_size, char* str_buf);

    error_code _get_cell(game* self, int x, int y, HAVANNAH_PLAYER* p);
    error_code _set_cell(game* self, int x, int y, HAVANNAH_PLAYER p, bool* wins);
    error_code _get_size(game* self, int* size);

    // implementation

    const char* _get_last_error(game* self)
    {
        return (char*)self->data2; // in this scheme opts are saved together with the state in data1, and data2 is the last error string
    }

    error_code _create_with_opts_str(game* self, const char* str)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        
        opts_repr& opts = _get_opts(self);
        opts.size = 8;
        if (str != NULL) {
            int ec = sscanf(str, "%u", &opts.size);
            if (ec != 1) {
                free(self->data1);
                self->data1 = NULL;
                return ERR_INVALID_INPUT;
            }
        }
        opts.board_sizer = 2 * opts.size - 1;

        self->sizer = (buf_sizer){
            .options_str = 16,
            .state_str = (size_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 ) + opts.board_sizer + 5),
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 )),
            .max_results = 1,
            .move_str = 4,
            .print_str = 1000,
        };
        return ERR_OK;
    }

    error_code _create_with_opts_bin(game* self, void* options_struct)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }
        
        opts_repr& opts = _get_opts(self);
        opts.size = 8;
        if (options_struct != NULL) {
            opts = *(opts_repr*)options_struct;
        }
        opts.board_sizer = 2 * opts.size - 1;

        self->sizer = (buf_sizer){
            .options_str = 16,
            .state_str = (size_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 ) + opts.board_sizer + 5),
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 )),
            .max_results = 1,
            .move_str = 4,
            .print_str = 1000,
        };
        return ERR_OK;
    }

    error_code _create_default(game* self)
    {
        self->data1 = new(malloc(sizeof(data_repr))) data_repr();
        if (self->data1 == NULL) {
            return ERR_OUT_OF_MEMORY;
        }

        opts_repr& opts = _get_opts(self);
        opts.size = 8;
        opts.board_sizer = 2 * opts.size - 1;

        self->sizer = (buf_sizer){
            .options_str = 16,
            .state_str = (size_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 ) + opts.board_sizer + 5),
            .player_count = 2,
            .max_players_to_move = 1,
            .max_moves = (uint32_t)(3 * opts.size * opts.size - ( 3 * opts.size - 1 )),
            .max_results = 1,
            .move_str = 4,
            .print_str = 1000,
        };
        return ERR_OK;
    }
    
    error_code _export_options_str(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        *ret_size = sprintf(str, "%u", opts.size);
        return ERR_OK;
    }

    error_code _destroy(game* self)
    {
        delete (data_repr*)self->data1;
        // free(self->data); // not required in the vector+map version
        self->data1 = NULL;
        return ERR_OK;
    }

    error_code _clone(game* self, game* clone_target)
    {
        if (clone_target == NULL) {
            return ERR_INVALID_INPUT;
        }
        clone_target->methods = self->methods;
        opts_repr& opts = _get_opts(self);
        error_code ec = clone_target->methods->create_with_opts_bin(clone_target, &opts);
        if (ec != ERR_OK) {
            return ec;
        }
        *(data_repr*)clone_target->data1 = *(data_repr*)self->data1;
        return ERR_OK;
    }
    
    error_code _copy_from(game* self, game* other)
    {
        *(data_repr*)self->data1 = *(data_repr*)other->data1;
        return ERR_OK;
    }

    error_code _compare(game* self, game* other, bool* ret_equal)
    {
        //BUG this doesnt actually work with the vector and map
        // *ret_equal = (memcmp(self->data1, other->data1, sizeof(data_repr)) == 0);
        // return ERR_OK;
        //TODO
        return ERR_STATE_CORRUPTED;
    }

    error_code _import_state(game* self, const char* str)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        data.remaining_tiles = (opts.board_sizer * opts.board_sizer) - (opts.size * (opts.size - 1));
        data.current_player = HAVANNAH_PLAYER_WHITE;
        data.winning_player = HAVANNAH_PLAYER_INVALID;
        data.graph_map.clear();
        data.next_graph_id = 1;
        data.gameboard.clear();
        data.gameboard.reserve(opts.board_sizer);
        for (int iy = 0; iy < opts.board_sizer; iy++) {
            std::vector<havannah_tile> tile_row_vector{};
            tile_row_vector.resize(opts.board_sizer, havannah_tile{HAVANNAH_PLAYER_INVALID, 0});
            data.gameboard.push_back(std::move(tile_row_vector));
            for (int ix = 0; ix < opts.board_sizer; ix++) {
                if ((ix - iy < opts.size) && (iy - ix < opts.size)) { // magic formula for only enabling valid cells of the board
                    data.gameboard[iy][ix].color = HAVANNAH_PLAYER_NONE;
                }
            }
        }
        if (str == NULL) {
            return ERR_OK;
        }
        // load from diy havannah format, "board p_cur p_res"
        int y = 0;
        int x = 0;
        // get cell fillings
        bool advance_segment = false;
        while (!advance_segment) {
            switch (*str) {
                case 'O': {
                    if (!((x - y < opts.size) && (y - x < opts.size))) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    _set_cell(self, x++, y, HAVANNAH_PLAYER_WHITE, NULL);
                } break;
                case 'X': {
                    if (!((x - y < opts.size) && (y - x < opts.size))) {
                        // out of bounds board
                        return ERR_INVALID_INPUT;
                    }
                    _set_cell(self, x++, y, HAVANNAH_PLAYER_BLACK, NULL);
                } break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': { // empty cells
                    uint32_t dacc = 0;
                    while (true) {
                        if (*str == '\0') {
                            return ERR_INVALID_INPUT;
                        }
                        if ((*str) < '0' || (*str) > '9') {
                            str--; // loop increments itself later on, we only peek the value here
                            break;
                        }
                        dacc *= 10;
                        uint32_t ladd = (*str)-'0';
                        if (ladd > opts.board_sizer) {
                            return ERR_INVALID_INPUT;
                        }
                        dacc += ladd;
                        str++;
                    }
                    for (int place_empty = 0; place_empty < dacc; place_empty++) {
                        if (!((x - y < opts.size) && (y - x < opts.size))) {
                            // out of bounds board
                            return ERR_INVALID_INPUT;
                        }
                        _set_cell(self, x++, y, HAVANNAH_PLAYER_NONE, NULL);
                    }
                } break;
                case '/': { // advance to next
                    y++;
                    // set the operational x as the actual x coord from in the sizer, NOT just counting existing cells from the left
                    x = (y >= opts.size ? y-opts.size+1 : 0);
                } break;
                case ' ': { // advance to next segment
                    advance_segment = true;
                } break;
                default: {
                    // failure, ran out of str to use or got invalid character
                    return ERR_INVALID_INPUT;
                } break;
            }
            str++;
        }
        // current player
        switch (*str) {
            case '-': {
                data.current_player = HAVANNAH_PLAYER_NONE;
            } break;
            case 'O': {
                data.current_player = HAVANNAH_PLAYER_WHITE;
            } break;
            case 'X': {
                data.current_player = HAVANNAH_PLAYER_BLACK;
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        str++;
        if (*str != ' ') {
            // failure, ran out of str to use or got invalid character
            return ERR_INVALID_INPUT;
        }
        str++;
        // result player
        switch (*str) {
            case '-': {
                data.winning_player = HAVANNAH_PLAYER_NONE;
            } break;
            case 'O': {
                data.winning_player = HAVANNAH_PLAYER_WHITE;
            } break;
            case 'X': {
                data.winning_player = HAVANNAH_PLAYER_BLACK;
            } break;
            default: {
                // failure, ran out of str to use or got invalid character
                return ERR_INVALID_INPUT;
            } break;
        }
        return ERR_OK;
    }

    error_code _export_state(game* self, size_t* ret_size, char* str)
    {
        if (str == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        const char* ostr = str;
        HAVANNAH_PLAYER cell_player;
        for (int y = 0; y < opts.board_sizer; y++) {
            int empty_cells = 0;
            for (int x = 0; x < opts.board_sizer; x++) {
                if (!((x - y < opts.size) && (y - x < opts.size))) {
                    continue;
                }
                _get_cell(self, x, y, &cell_player);
                if (cell_player == PLAYER_NONE) {
                    empty_cells++;
                } else {
                    // if the current cell isnt empty, print its representation, before that print empty cells, if any
                    if (empty_cells > 0) {
                        str += sprintf(str, "%d", empty_cells);
                        empty_cells = 0;
                    }
                    str += sprintf(str, "%c", (cell_player == HAVANNAH_PLAYER_WHITE ? 'O' : 'X'));
                }
            }
            if (empty_cells > 0) {
                str += sprintf(str, "%d", empty_cells);
            }
            if (y < opts.board_sizer - 1) {
                str += sprintf(str, "/");
            }
        }
        // current player
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm_count == 0) {
            ptm = HAVANNAH_PLAYER_NONE;
        }
        switch (ptm) {
            case HAVANNAH_PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case HAVANNAH_PLAYER_WHITE: {
                str += sprintf(str, " O");
            } break;
            case HAVANNAH_PLAYER_BLACK: {
                str += sprintf(str, " X");
            } break;
        }
        // result player
        player_id res;
        uint8_t res_count;
        _get_results(self, &res_count, &res);
        if (res_count == 0) {
            res = HAVANNAH_PLAYER_NONE;
        }
        switch (res) {
            case HAVANNAH_PLAYER_NONE: {
                str += sprintf(str, " -");
            } break;
            case HAVANNAH_PLAYER_WHITE: {
                str += sprintf(str, " O");
            } break;
            case HAVANNAH_PLAYER_BLACK: {
                str += sprintf(str, " X");
            } break;
        }
        *ret_size = str-ostr;
        return ERR_OK;
    }

    error_code _players_to_move(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = _get_repr(self);
        if (data.current_player == PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.current_player;
        return ERR_OK;
    }

    error_code _get_concrete_moves(game* self, player_id player, uint32_t* ret_count, move_code* moves)
    {
        if (moves == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        uint32_t move_cnt = 0;
        for (int iy = 0; iy < opts.board_sizer; iy++) {
            for (int ix = 0; ix < opts.board_sizer; ix++) {
                if (data.gameboard[iy][ix].color == HAVANNAH_PLAYER_NONE) {
                    // add the free tile to the return vector
                    moves[move_cnt++] = (ix << 8) | iy;
                }
            }
        }
        *ret_count = move_cnt;
        return ERR_OK;
    }

    error_code _is_legal_move(game* self, player_id player, move_code move, sync_counter sync)
    {
        if (move == MOVE_NONE) {
            return ERR_INVALID_INPUT;
        }
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        if (ptm != player) {
            return ERR_INVALID_INPUT;
        }
        data_repr& data = _get_repr(self);
        int ix = (move >> 8) & 0xFF;
        int iy = move & 0xFF;
        if (data.gameboard[iy][ix].color != HAVANNAH_PLAYER_NONE) {
            return ERR_INVALID_INPUT;
        }
        return ERR_OK;
    }

    error_code _make_move(game* self, player_id player, move_code move)
    {
        data_repr& data = _get_repr(self);
        
        int tx = (move >> 8) & 0xFF;
        int ty = move & 0xFF;

        bool wins;
        // set cell updates graph structures in the backend, and informs us if this move is winning for the current player
        _set_cell(self, tx, ty, data.current_player, &wins);

        // if current player is winner set appropriate state
        if (wins) {
            data.winning_player = data.current_player;
            data.current_player = HAVANNAH_PLAYER_NONE;
        } else if (--data.remaining_tiles == 0) {
            data.winning_player = HAVANNAH_PLAYER_NONE;
            data.current_player = HAVANNAH_PLAYER_NONE;
        } else {
            // only really switch colors if the game is still going
            data.current_player = data.current_player == HAVANNAH_PLAYER_WHITE ? HAVANNAH_PLAYER_BLACK : HAVANNAH_PLAYER_WHITE;
        }

        /*/ debug printing
        std::cout << "adj. graph count: " << std::to_string(adjacentGraphCount) << "\n";
        std::cout << "connected graphs: [" << std::to_string(adjacentGraphs[0]) << "] [" << std::to_string(adjacentGraphs[1]) << "] [" << std::to_string(adjacentGraphs[2]) << "]\n";
        std::cout << "started empty: " << std::to_string(emptyStart) << "\n";
        std::cout << "current graph id: " << std::to_string(currentGraphId) << "\n";
        std::cout << "tile contributing features: B[" << std::bitset<6>(contributionBorder) << "] C[" << std::bitset<6>(contributionCorner) << "]\n";
        std::cout << "current graph features: B[" << std::bitset<6>(graphMap[currentGraphId].connectedBorders) << "] C[" << std::bitset<6>(graphMap[currentGraphId].connectedCorners) << "]\n";*/

        return ERR_OK;
    }

    error_code _get_results(game* self, uint8_t* ret_count, player_id* players)
    {
        if (players == NULL) {
            return ERR_INVALID_INPUT;
        }
        *ret_count = 1;
        data_repr& data = _get_repr(self);
        if (data.current_player != HAVANNAH_PLAYER_NONE) {
            *ret_count = 0;
            return ERR_OK;
        }
        *players = data.winning_player;
        return ERR_OK;
    }

    error_code _playout(game* self, uint64_t seed)
    {
        uint32_t ctr = 0;
        move_code* moves;
        uint32_t moves_count;
        _get_concrete_moves(self, HAVANNAH_PLAYER_NONE, &moves_count, moves);
        moves = (move_code*)malloc(moves_count * sizeof(move_code));
        player_id ptm;
        uint8_t ptm_count;
        _players_to_move(self, &ptm_count, &ptm);
        while (ptm_count > 0) {
            _get_concrete_moves(self, ptm, &moves_count, moves);
            _make_move(self, ptm ,moves[squirrelnoise5(ctr++, seed)%moves_count]);
            _players_to_move(self, &ptm_count, &ptm);
        }
        free(moves);
        return ERR_OK;
    }

    error_code _get_move_code(game* self, player_id player, const char* str, move_code* ret_move)
    {
        if (strlen(str) >= 1 && str[0] == '-') {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        if (strlen(str) < 2) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        int x = (str[0]-'a');
        uint8_t y8;
        int ec = sscanf(str+1, "%hhu", &y8);
        if (ec != 1) {
            return ERR_INVALID_INPUT;
        }
        int y = y8;
        HAVANNAH_PLAYER cell_player;
        _get_cell(self, x, y, &cell_player);
        if (cell_player == HAVANNAH_PLAYER_INVALID) {
            *ret_move = MOVE_NONE;
            return ERR_INVALID_INPUT;
        }
        *ret_move = (x << 8) | y;
        return ERR_OK;
    }

    error_code _get_move_str(game* self, player_id player, move_code move, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        if (move == MOVE_NONE) {
            *ret_size = sprintf(str_buf, "-");
            return ERR_OK;
        }
        int x = (move >> 8) & 0xFF;
        int y = move & 0xFF;
        *ret_size = sprintf(str_buf, "%c%c", 'a' + x, '0' + y);
        return ERR_OK;
    }

    error_code _debug_print(game* self, size_t* ret_size, char* str_buf)
    {
        if (str_buf == NULL) {
            return ERR_INVALID_INPUT;
        }
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        const char* ostr = str_buf;
        switch (1) { //TODO proper switch in options
            case 0:
                // square printing of the full gameboard matrix
                for (int iy = 0; iy < opts.board_sizer; iy++) {
                    for (int ix = 0; ix < opts.board_sizer; ix++) {
                        str_buf += sprintf(str_buf, "%c", HAVANNAH_PLAYER_CHARS[data.gameboard[iy][ix].color]);
                    }
                    str_buf += sprintf(str_buf, "\n");
                }
                break;
            case 1:
                // horizontally formatted printing
                /*
                 x x
                x x x
                 x x
                */
                for (int iy = 0; iy < opts.board_sizer; iy++) {
                    int padding_count = (iy <= opts.size-1) ? (opts.size-1)-iy: iy-(opts.size-1);
                    for (int ip = 0; ip < padding_count; ip++) {
                        str_buf += sprintf(str_buf, " ");
                    }
                    for (int ix = 0; ix < opts.board_sizer; ix++) {
                        if ((ix - iy < opts.size) && (iy - ix < opts.size)) {
                            str_buf += sprintf(str_buf, " %c", HAVANNAH_PLAYER_CHARS[data.gameboard[iy][ix].color]);
                        }
                    }
                    str_buf += sprintf(str_buf, "\n");
                }
                break;
            case 2:
                // vertically formatted printing
                /*
                    x
                x       x
                    x
                x       x
                    x
                */
                // from: https://www.techiedelight.com/print-matrix-diagonally-positive-slope/#comment-3071
                for (int sum = 0; sum <= 2*(opts.board_sizer-1); sum++) {
                    int r_end = std::min(sum, opts.board_sizer-1);
                    int r = sum - std::min(sum, opts.board_sizer-1);
                    int paddingCount = (sum < opts.board_sizer) ? (opts.board_sizer-r_end)-1: r;
                    for (int ip = 0; ip < paddingCount; ip++) {
                        str_buf += sprintf(str_buf, "    ");
                    }
                    while(r <= r_end) {
                        if (data.gameboard[r][sum-r].color == HAVANNAH_PLAYER_INVALID) {
                            str_buf += sprintf(str_buf, "        ");
                            r++;
                            continue;
                        }
                        str_buf += sprintf(str_buf, "%c       ", HAVANNAH_PLAYER_CHARS[data.gameboard[r][sum-r].color]);
                        r++;
                    }
                    str_buf += sprintf(str_buf, "\n");
                }
                break;
        }
        *ret_size = str_buf-ostr;
        return ERR_OK;
    }

    //=====
    // game internal methods

    error_code _get_cell(game* self, int x, int y, HAVANNAH_PLAYER* p)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);
        if (x < 0 || y < 0 || x >= opts.board_sizer || y >= opts.board_sizer) {
            *p = HAVANNAH_PLAYER_INVALID;
        } else {
            *p = data.gameboard[y][x].color;
        }
        return ERR_OK;
    }

    error_code _set_cell(game* self, int x, int y, HAVANNAH_PLAYER p, bool* wins)
    {
        opts_repr& opts = _get_opts(self);
        data_repr& data = _get_repr(self);

        bool winner = false; // if this is true by the end, player p wins with this move
        uint8_t contribution_border = 0b00111111; // begin on the north-west corner and it's left border, going counter-clockwise
        uint8_t contribution_corner = 0b00111111; 
        std::array<uint8_t, 3> adjacent_graphs = {0, 0, 0}; // there can only be a maximum of 3 graphs with gaps in between
        int adjacent_graph_count = 0;
        bool empty_start = true; // true if the first neighbor tile is non-same color
        uint8_t current_graph_id = 0; // 0 represents a gap, any value flags an ongoing streak which has to be saved before resetting the flag to 0 for a gap

        // discover north-west neighbor
        if (y > 0 && x > 0) {
            // exists
            if (data.gameboard[y-1][x-1].color == p) {
                current_graph_id = data.gameboard[y-1][x-1].parent_graph_id;
                empty_start = false;
            }/* else {
                // not a same colored piece, save previous streak and set gap
                if (currentGraphId != 0) {
                    adjacentGraphs[adjacentGraphCount++] = currentGraphId;
                }
                currentGraphId = 0;
            }*/
            contribution_border &= 0b00011110;
            contribution_corner &= 0b00001110;
        } else {
            // invalid, narrow possible contribution features and save previous streak
            contribution_border &= 0b00100001;
            contribution_corner &= 0b00110001;
            /*if (currentGraphId != 0) {
                adjacentGraphs[adjacentGraphCount++] = currentGraphId;
            }
            currentGraphId = 0;*/
        }

        // discover west neighbor
        if (x > 0 && (y - x) < opts.size-1) {
            if (data.gameboard[y][x-1].color == p) {
                current_graph_id = data.gameboard[y][x-1].parent_graph_id;
            } else {
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            contribution_border &= 0b00001111;
            contribution_corner &= 0b00000111;
        } else {
            contribution_border &= 0b00110000;
            contribution_corner &= 0b00111000;
            if (current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }
            current_graph_id = 0;
        }

        // discover south-west neighbor
        if (y < opts.board_sizer-1 && (y - x < opts.size-1)) {
            if (data.gameboard[y+1][x].color == p) {
                current_graph_id = data.gameboard[y+1][x].parent_graph_id;
            } else {
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            contribution_border &= 0b00100111;
            contribution_corner &= 0b00100011;
        } else {
            contribution_border &= 0b00011000;
            contribution_corner &= 0b00011100;
            if (current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }
            current_graph_id = 0;
        }
        
        // discover south-east neighbor
        if (y < opts.board_sizer-1 && x < opts.board_sizer-1) {
            if (data.gameboard[y+1][x+1].color == p) {
                current_graph_id = data.gameboard[y+1][x+1].parent_graph_id;
            } else {
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            contribution_border &= 0b00110011;
            contribution_corner &= 0b00110001;
        } else {
            contribution_border &= 0b00001100;
            contribution_corner &= 0b00001110;
            if (current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }
            current_graph_id = 0;
        }

        // discover east neighbor
        if (x < opts.board_sizer-1 && (x - y < opts.size-1)) {
            if (data.gameboard[y][x+1].color == p) {
                current_graph_id = data.gameboard[y][x+1].parent_graph_id;
            } else {
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            contribution_border &= 0b00111001;
            contribution_corner &= 0b00111000;
        } else {
            contribution_border &= 0b00000110;
            contribution_corner &= 0b00000111;
            if (current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }
            current_graph_id = 0;
        }

        // discover north-east neighbor
        if (y > 0 && (x - y < opts.size-1)) {
            if (data.gameboard[y-1][x].color == p) {
                current_graph_id = data.gameboard[y-1][x].parent_graph_id;
            } else {
                if (current_graph_id != 0) {
                    adjacent_graphs[adjacent_graph_count++] = current_graph_id;
                }
                current_graph_id = 0;
            }
            contribution_border &= 0b00111100;
            contribution_corner &= 0b00011100;
        } else {
            contribution_border &= 0b00000011;
            contribution_corner &= 0b00100011;
            if (current_graph_id != 0) {
                adjacent_graphs[adjacent_graph_count++] = current_graph_id;
            }
            current_graph_id = 0;
        }

        // simulate a gap, to properly save last streak if requird by empty start
        // non-empty start will just discard the last streak which would be a dupe of itself anyway
        if (empty_start && current_graph_id != 0) {
            adjacent_graphs[adjacent_graph_count++] = current_graph_id;
        }

        //##### evaluate neighbor scanning results to update state

        // resolve all existing adjacent graphs to their true parentGraphId
        for (int i = 0; i < adjacent_graph_count; i++) {
            while (adjacent_graphs[i] != data.graph_map[adjacent_graphs[i]].parent_graph_id) {
                //TODO any good way to keep the indirections down, without looping over the whole graphMap when reparenting?
                adjacent_graphs[i] = data.graph_map[adjacent_graphs[i]].parent_graph_id;
            }
        }
        
        // set current graph to the first discovered, if any
        if (adjacent_graph_count > 0) {
            current_graph_id = adjacent_graphs[0];
        }

        // check for ring creation amongst adjacent graphs
        if (
            (adjacent_graphs[0] != 0 && adjacent_graphs[1] != 0 && adjacent_graphs[0] == adjacent_graphs[1]) ||
            (adjacent_graphs[0] != 0 && adjacent_graphs[2] != 0 && adjacent_graphs[0] == adjacent_graphs[2]) ||
            (adjacent_graphs[2] != 0 && adjacent_graphs[1] != 0 && adjacent_graphs[2] == adjacent_graphs[1])
        ) {
            // ring detected, game has been won by currentPlayer
            winner = true;
        }

        // no adjacent graphs, create a new one
        if (adjacent_graph_count == 0) {
                current_graph_id = data.next_graph_id++;
                data.graph_map[current_graph_id].parent_graph_id = current_graph_id;
        }

        // 2 or 3 adjacent graphs, all get reparented and contribute their features
        for (int i = 1; i < adjacent_graph_count; i++) {
                data.graph_map[adjacent_graphs[i]].parent_graph_id = current_graph_id;
                data.graph_map[current_graph_id].connected_borders |= data.graph_map[adjacent_graphs[i]].connected_borders;
                data.graph_map[current_graph_id].connected_corners |= data.graph_map[adjacent_graphs[i]].connected_corners;
        }

        // contribute target tile features
        if (contribution_border == 0b00000000) {
            data.graph_map[current_graph_id].connected_corners |= contribution_corner;
        } else {
            data.graph_map[current_graph_id].connected_borders |= contribution_border;
        }

        // check if the current graph has amassed a winning number of borders or corners
        if (std::bitset<6>(data.graph_map[current_graph_id].connected_borders).count() >= 3 || std::bitset<6>(data.graph_map[current_graph_id].connected_corners).count() >= 2) {
            // fork or bridge detected, game has been won by currentPlayer
            winner = true;
        }

        // perform actual move
        data.gameboard[y][x].color = p;
        data.gameboard[y][x].parent_graph_id = current_graph_id;

        if (wins) {
            *wins = winner;
        }

        return ERR_OK;
    }

    error_code _get_size(game* self, int* size)
    {
        opts_repr& opts = _get_opts(self);
        *size = opts.size;
        return ERR_OK;
    }

}

const char HAVANNAH_PLAYER_CHARS[4] = {'.', 'O', 'X', '-'}; // none, white, black, invalid

static const havannah_internal_methods havannah_gbe_internal_methods{
    .get_cell = _get_cell,
    .set_cell = _set_cell,
    .get_size = _get_size,
};

const game_methods havannah_gbe{

    .game_name = "Havannah",
    .variant_name = "Standard",
    .impl_name = "surena_default",
    .version = semver{0, 1, 0},
    .features = game_feature_flags{
        .options = true,
        .options_bin = true,
        .options_bin_ref = false,
        .random_moves = false,
        .hidden_information = false,
        .simultaneous_moves = false,
        .sync_counter = false,
        .move_ordering = false,
        .id = false,
        .eval = false,
        .playout = true,
        .print = true,
    },
    .internal_methods = (void*)&havannah_gbe_internal_methods,
    
    #include "surena/game_impl.h"
    
};
