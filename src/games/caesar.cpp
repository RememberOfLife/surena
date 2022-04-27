#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "surena/util/fast_prng.hpp"

#include "surena/games/caesar.hpp"

namespace surena {

    
    Caesar::Token::Token(ProvinceBonus tile_province_bonus)
    {
        //TODO
    }

    Caesar::Token::Token(InfluenceType tile_influence_basetype, bool tile_bordercontrol)
    {
        //TODO
    }
    
    void Caesar::Token::PlaceInfluence(Color token_color, InfluenceType token_influence_type, uint8_t token_influence_strength, bool token_influence_direction)
    {
        //TODO
    }
    
    void Caesar::Token::PlaceBorderControl(Color token_color)
    {
        //TODO
    }
    
    void Caesar::Token::PlaceControl(Color controlling_player)
    {
        //TODO
    }
    
    Caesar::ProvinceBonus Caesar::Token::RemoveProvinceBonus()
    {
        //TODO
        return PROVINCEBONUS_SENATE;
    }

    uint8_t Caesar::default_gameboard_nodes[] = {
        1, 2, INFLUENCETYPE_SHIELD, 3, INFLUENCETYPE_BOAT, 0,
        2, 1, 5, INFLUENCETYPE_SWORD, 0,
        3, 1, 4, INFLUENCETYPE_BOAT, 6, INFLUENCETYPE_SWORD, 0,
        4, 3, 5, INFLUENCETYPE_BORDERCONTROL, INFLUENCETYPE_SHIELD, 7, INFLUENCETYPE_BOAT, 8, INFLUENCETYPE_SHIELD, 0,
        5, 2, 4, 9, INFLUENCETYPE_SWORD, 0,
        6, 3, 7, INFLUENCETYPE_BOAT, 10, INFLUENCETYPE_BORDERCONTROL, INFLUENCETYPE_SWORD, 0,
        7, 4, 6, 8, INFLUENCETYPE_SWORD, 10, INFLUENCETYPE_BOAT, 0,
        8, 4, 7, 9, INFLUENCETYPE_SWORD, 11, INFLUENCETYPE_BOAT, 12, INFLUENCETYPE_SHIELD, 13, INFLUENCETYPE_BOAT, 0,
        9, 5, 8, 13, INFLUENCETYPE_SHIELD, 0,
        10, 6, 7, 11, INFLUENCETYPE_BOAT, 14, INFLUENCETYPE_SHIELD, 0,
        11, 8, 10, 12, INFLUENCETYPE_SWORD, 15, INFLUENCETYPE_SHIELD, 0,
        12, 8, 11, 13, INFLUENCETYPE_SWORD, 0,
        13, 8, 9, 12, 0,
        14, 10, 15, INFLUENCETYPE_SHIELD, 16, INFLUENCETYPE_SWORD, 0,
        15, 11, 14, 16, INFLUENCETYPE_BOAT, 17, INFLUENCETYPE_SHIELD, 0,
        16, 14, 15, 17, INFLUENCETYPE_BORDERCONTROL, INFLUENCETYPE_BOAT, 18, INFLUENCETYPE_SWORD, 0,
        17, 15, 16, 18, INFLUENCETYPE_SHIELD, 0,
        18, 16, 17, 0,
        0, 8, 0,
    };

    Caesar::gameboardNodeId Caesar::default_gameboard_ids[] = {
        {1, "HU"},
        {2, "HC"},
        {3, "MT"},
        {4, "SD"},
        {5, "GA"},
        {6, "NU"},
        {7, "SC"},
        {8, "IT"},
        {9, "GC"},
        {10, "AF"},
        {11, "AC"},
        {12, "MD"},
        {13, "DM"},
        {14, "CY"},
        {15, "CR"},
        {16, "AG"},
        {17, "AS"},
        {18, "SY"},
        {0, ""},
    };

    //TODO
    uint8_t Caesar::LUT_token_type[] = {
        0, // 0 token, empty
    };

    Caesar::Caesar(uint8_t gameboard_nodes[], Caesar::gameboardNodeId gameboard_ids[]):
        current_player(COLOR_RED),
        winning_player(COLOR_NONE)
    {
        //TODO

        // loop through gameboard ids to find out the number of used ids
        int node_count = 0;
        while (true) {
            if (gameboard_ids[node_count].num == 0) {
                break;
            }
            node_count++;
        }
        //TODO sorted
        // save id-nums to the actual state of gameboard id assocs.
        node_ids = (gameboardNodeId*)malloc(sizeof(gameboardNodeId)*node_count);
        nodes = (Token*)malloc(sizeof(Token)*node_count); // will be initialized through moves later on
        for (int i = 0; i < node_count; i++) {
            node_ids[i] = gameboard_ids[i];
        }
        // create temporary 3(n-2) sized buffer for connection creation, space for ids of conn. nodes
        int max_connection_count = (3*(node_count-2));
        Token* tmp_connections = (Token*)malloc(sizeof(Token)*max_connection_count);
        // create temporary conn. state array and intermediate perfect hashing offset array with size from before (USE VECTOR BUCKETS for state array)
        struct ConnectionEntry {
            uint8_t a; // lesser id
            uint8_t b; // greater id
            int connection_id; // id in tmp_connections
        };
        ConnectionEntry* tmp_connectionentries = (ConnectionEntry*)malloc(sizeof(ConnectionEntry)*max_connection_count);
        std::vector<uint8_t> double_control_nodes{};
        // walk through the nodes and build connections into the buffer, storing conn. info and used node ids, get num of actual conns.
        int connection_count = 0;
        int connection_step = 0;
        while (true) {
            uint8_t origin_node_id = gameboard_nodes[connection_step++];
            if (origin_node_id == 0) {
                // read nodes that get double control tokens
                while (true) {
                    uint8_t double_control_node_id = gameboard_nodes[connection_step++];
                    if (double_control_node_id == 0) {
                        break;
                    }
                    double_control_nodes.push_back(double_control_node_id);
                }
                break;
            }
            // read target nodes that should be connected to origin
            while (true) {
                uint8_t target_node_id = gameboard_nodes[connection_step++];
                if (target_node_id == 0) {
                    // read all target nodes, return to reading more origin nodes
                    break;
                }
                // process target node
                InfluenceType connection_influence_type = INFLUENCETYPE_NONE;
                bool connection_bordercontrol = false;
                if (target_node_id > origin_node_id) {
                    // new connection to record, get influence type
                    connection_influence_type = static_cast<InfluenceType>(gameboard_nodes[connection_step++]);
                    if (connection_influence_type == INFLUENCETYPE_BORDERCONTROL) {
                        connection_bordercontrol = true;
                        connection_influence_type = static_cast<InfluenceType>(gameboard_nodes[connection_step++]);
                    }
                    // add new connection to known connections
                    tmp_connections[connection_count] = Token(connection_influence_type, connection_bordercontrol);
                    tmp_connectionentries[connection_count] = ConnectionEntry{origin_node_id, target_node_id, connection_count};
                    connection_count++;
                }
            }
        }
        // make vector bucket buffer for perfect hashing
        std::vector<ConnectionEntry>* connection_buckets = (std::vector<ConnectionEntry>*)malloc(sizeof(std::vector<ConnectionEntry>*)*connection_count);
        // hash all conns. using offset 0 into the buckets of the tmp state array
        connections_intermediate = (uint32_t*)malloc(sizeof(uint32_t)*connection_count);
        for (int i = 0; i < connection_count; i++) {
            connections_intermediate[i] = 0;
            uint32_t a = tmp_connectionentries[i].a;
            uint32_t b = tmp_connectionentries[i].b;
            connection_buckets[OffsetHash(a, b, 0)].push_back(tmp_connectionentries[i]);
        }
        // walk through the buckets from the top, if >1 item
        // then rehash them with different offsets until all are displaced into empty slots, save offset into intermediate array

        //TODO

        // save state array and intermediate to actual gamestates
        //TODO process double controls and place their senate marker, remove it from supply bag
        //TODO fill connection tokens
    }

    Caesar::~Caesar()
    {
        //TODO
    }

    void Caesar::import_state(const char* str)
    {
        //TODO
    }

    uint32_t Caesar::export_state(char* str)
    {
        //TODO
        return 0;
    }

    uint8_t Caesar::player_to_move()
    {
        return current_player;
    }

    std::vector<uint64_t> Caesar::get_moves()
    {
        //TODO
        std::vector<uint64_t> moves{};
        return moves;
    }

    void Caesar::apply_move(uint64_t move_id)
    {
        //TODO
    }

    uint8_t Caesar::get_result()
    {
        return winning_player;
    }

    void Caesar::discretize(uint64_t seed)
    {
        //TODO
    }

    uint8_t Caesar::perform_playout(uint64_t seed)
    {
        //TODO
        return winning_player;
    }
    
    Game* Caesar::clone()
    {
        //TODO
        return NULL;
    }

    void Caesar::copy_from(Game* target)
    {
        //TODO
    }

    uint64_t Caesar::get_move_id(std::string move_string)
    {
        //TODO
        return 0;
    }

    std::string Caesar::get_move_string(uint64_t move_id)
    {
        //TODO
        return "";
    }
    
    void Caesar::debug_print()
    {
        //TODO
    }

    uint32_t Caesar::OffsetHash(uint32_t i_a, uint32_t i_b, uint32_t offset)
    {
        //TODO
        return 0;
    }

}
