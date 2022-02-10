#pragma once

#include "surena/util/fast_prng.hpp"
#include "surena/engine.hpp"
#include "surena/game.hpp"

namespace surena {

    class SinglethreadedMCTS : public Engine {

        struct SearchTreeNode {
            SearchTreeNode* parent;
            SearchTreeNode* left_child;
            SearchTreeNode* right_sibling;
            uint64_t playout_count;
            uint64_t reward_count;
            uint64_t move_id;
            uint8_t player_to_move;
            SearchTreeNode(
                SearchTreeNode* parent,
                SearchTreeNode* left_child,
                SearchTreeNode* right_sibling,
                uint64_t playout_count,
                uint64_t reward_count,
                uint64_t move_id,
                uint8_t player_to_move
            );
            ~SearchTreeNode();
        };

        private:

            Game* gamestate;
            fast_prng rng;
            SearchTreeNode* root;

        public:

            SinglethreadedMCTS();

            SinglethreadedMCTS& operator=(SinglethreadedMCTS&& copy) = delete;

            SinglethreadedMCTS& operator=(SinglethreadedMCTS& copy) = delete;

            SinglethreadedMCTS(SinglethreadedMCTS&& copy) = delete;

            SinglethreadedMCTS(SinglethreadedMCTS& copy) = delete;

            ~SinglethreadedMCTS() override;

            void search_start() override;
            void search_stop() override;
            
            uint64_t get_best_move() override;

            void set_gamestate(Game* target_gamestate) override;

            uint8_t player_to_move() override;

            std::vector<uint64_t> get_moves() override;

            void apply_move(uint64_t move_id) override;

            void apply_internal_update(uint64_t update_id) override;

            uint8_t get_result() override;

            uint64_t get_move_id(std::string move_string) override;
            std::string get_move_string(uint64_t move_id) override;
            
            void debug_print() override;

    };

}
