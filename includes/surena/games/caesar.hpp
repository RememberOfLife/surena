#ifndef CAESAR_HPP
#define CAESAR_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "surena/game.hpp"

namespace surena {

    class Caesar : public ImperfectInformationGame {

        enum Color : uint8_t {
            COLOR_NONE = 0,
            COLOR_RED = 1,
            COLOR_BLUE = 2,
        };

        enum TokenType : uint8_t {
            TOKENTYPE_CONTROL = 0,
            TOKENTYPE_PROVINCE = 1,
            TOKENTYPE_INFLUENCE = 2,
        };
        
        enum ProvinceBonus : uint8_t {
            PROVINCEBONUS_SENATE = 0,
            PROVINCEBONUS_TACTICS = 1,
            PROVINCEBONUS_WEALTH = 2,
            PROVINCEBONUS_MIGHT = 3,
            PROVINCEBONUS_CENTURION = 4,
            PROVINCEBONUS_POISON = 5,
        };

        enum InfluenceType : uint8_t {
            INFLUENCETYPE_NONE = 0,
            INFLUENCETYPE_WILD = 1,
            INFLUENCETYPE_SWORD = 2,
            INFLUENCETYPE_SHIELD = 3,
            INFLUENCETYPE_BOAT = 4,
            INFLUENCETYPE_BORDERCONTROL = 5,
        };

        // every token is represented by a uint16_t, bits numbered B16->0 with 0 being least significant
        // B16 : 1=token has a control token placed on it
        // B15 : player color of placed control token
        // B14 : 0=influence 1=province token
        // B13 : 0=faceup 1=facedown
        // B12 : player color placed influence token
        // B11 : direction of influence 0=higher influence towards the higher node of the connection
        // B10 B9 B8 : type of influence
        // B7 B6 : strength type of influence
        // B5 B4 : influence type that may be placed on this border
        // B3 : is this a border control space?
        struct Token {
            //uint16_t data;
            Color control; // B16 15
            TokenType basetile_type; // B14
            bool faceup; // B13
            Color influence_player; // B12
            bool influence_direction; // 11
            union tile_type {
                InfluenceType influence_type;
                ProvinceBonus province_bonus;
            }; // B10 B9 B8
            uint8_t influence_strength; // B7 B6
            InfluenceType influence_basetype; // B5 B4
            bool bordercontrol; // B3
            Token(ProvinceBonus tile_province_bonus); // construct province bonus
            Token(InfluenceType tile_influence_basetype, bool tile_bordercontrol); // construct empty influence tile
            void PlaceInfluence(Color token_color, InfluenceType token_influence_type, uint8_t token_influence_strength, bool token_influence_direction);
            void PlaceBorderControl(Color token_color);
            void PlaceControl(Color controlling_player);
            ProvinceBonus RemoveProvinceBonus();
        };

        struct gameboardNodeId {
            uint8_t num;
            char name[7];
        };

        public:

            // start with id of described node, then list of node ids that this node is connected to,
            // suffix every node with the influence type that may be placed on this connection,
            // only suffix if the target node id is GREATER than the origin (forward declaring)
            // prefix the influence type with bordercontrol if applicable
            // finish sub list with 0
            // close node list with another 0 (i.e. as if starting a new node with id 0)
            // the 0 node is followed by a list of node ids that provide double control tokens on capture and always have a senate bonus
            // close everything with a 0
            static uint8_t default_gameboard_nodes[];
            // list of associations between node names and node ids, at most 6 character long names, end list with the 0 node
            static gameboardNodeId default_gameboard_ids[];

            // token type lookup table
            static uint8_t LUT_token_type[];

        private:

            // state
            Caesar::Color current_player;
            Caesar::Color winning_player;

            gameboardNodeId* node_ids;
            Token* nodes;
            uint32_t* connections_intermediate; // intermediate table for perfect hashing on connections
            Token* connections;

            //TODO state for each player: control markers left and 4 senate slots with value, and whats available in the bag+centurion+wealth
            // use extra vector for definitely holding handcards and definitely not holding handcards
            // keep list of province bonuses to work through for each player

        public:

            //TODO expansions?
            Caesar(uint8_t gameboard_nodes[], gameboardNodeId gameboard_ids[]);

            ~Caesar();

            void discretize(uint64_t seed) override;

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            uint8_t get_result() override;

            uint8_t perform_playout(uint64_t seed) override;
            
            PerfectInformationGame* clone() override;
            void copy_from(PerfectInformationGame* target) override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;
            
            void debug_print() override;

        private:

            // helpers

            //TODO some cheap hashing function with an offset
            uint32_t OffsetHash(uint32_t i_a, uint32_t i_b, uint32_t offset);

    };

}

#endif
