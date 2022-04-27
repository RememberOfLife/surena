#include <chrono>
#include <math.h>

#include "surena/util/fast_prng.hpp"

#include "surena/engines/singlethreadedmcts.hpp"

namespace surena {

    SinglethreadedMCTS::SearchTreeNode::SearchTreeNode(
        SearchTreeNode* parent,
        SearchTreeNode* left_child,
        SearchTreeNode* right_sibling,
        uint64_t playout_count,
        uint64_t reward_count,
        uint64_t move_id,
        uint8_t player_to_move
    ):
        parent(parent),
        left_child(left_child),
        right_sibling(right_sibling),
        playout_count(playout_count),
        reward_count(reward_count),
        move_id(move_id),
        player_to_move(player_to_move)
    {}

    SinglethreadedMCTS::SearchTreeNode::~SearchTreeNode()
    {
        delete left_child;
        delete right_sibling;
    }

    SinglethreadedMCTS::SinglethreadedMCTS():
        gamestate(NULL),
        rng(42),
        root(new SearchTreeNode(NULL, NULL, NULL, 0, 0, 0, 0))
    {}

    SinglethreadedMCTS::~SinglethreadedMCTS()
    {
        delete gamestate;
        delete root;
    }

    void SinglethreadedMCTS::search_start(uint64_t ms_timeout)
    {
        std::chrono::time_point<std::chrono::steady_clock> start_clock = std::chrono::steady_clock::now();
        std::chrono::time_point<std::chrono::steady_clock> check_clock = std::chrono::steady_clock::now();
        uint64_t iterations = 0;
        Game* selection_game = gamestate->clone();
        Game* playout_game = gamestate->clone();
        while (true) {
        //while (iterations < 100) {
            if (ms_timeout > 0) {
                check_clock = std::chrono::steady_clock::now();
                if(std::chrono::duration_cast<std::chrono::milliseconds>(check_clock-start_clock).count() > ms_timeout) {
                    break;
                }
            }
            iterations++;

            selection_game->copy_from(gamestate);
            // #1 walk down the tree
            SearchTreeNode* target_node = root;
            SearchTreeNode* curr_node = root;
            while (true) {
                uint64_t parent_playouts = curr_node->playout_count;
                curr_node = curr_node->left_child;
                SearchTreeNode* best_node = NULL;
                float best_weight = -1;
                // #1.5 select best node
                while (curr_node != NULL) {
                    // use UCT for selection of the most worthwhile child to exploit/explore
                    float curr_weigth = (
                        (static_cast<float>(curr_node->reward_count) / static_cast<float>(curr_node->playout_count)) +
                        (
                            10.4f * //TODO make this an option for the engine
                            sqrtf( logf(static_cast<float>(parent_playouts)) / static_cast<float>(curr_node->playout_count) )
                        )
                    );
                    if (curr_weigth > best_weight) {
                        best_weight = curr_weigth;
                        best_node = curr_node;
                        target_node = best_node;
                    }
                    curr_node = curr_node->right_sibling;
                }
                curr_node = best_node;
                if(!curr_node) break;
                selection_game->apply_move(curr_node->move_id);
            }
            // game decided by selection, backpropagate
            if (selection_game->player_to_move() == 0) {
                uint8_t reward_player = selection_game->get_result();
                while (target_node != NULL) {
                    target_node->playout_count++;
                    uint8_t parent_player_to_move = target_node->player_to_move;
                    if (target_node->parent) {
                        parent_player_to_move = target_node->parent->player_to_move;
                    }
                    // give reward 2 if winning player, if draw then give reward 1 to all
                    if (reward_player == parent_player_to_move) {
                        target_node->reward_count += 2;
                    } else if (reward_player == 0) {
                        target_node->reward_count += 1;
                    }
                    target_node = target_node->parent;
                }
                continue;
            }
        
            // #2 make all the new nodes
            std::vector<uint64_t> available_moves = selection_game->get_moves();
            for (int i = 0; i < available_moves.size(); i++) {
                playout_game->copy_from(selection_game);
                playout_game->apply_move(available_moves[i]);
                // add node to tree
                SearchTreeNode* backprop_node = new SearchTreeNode(
                    target_node, NULL, target_node->left_child, 0, 0, available_moves[i], playout_game->player_to_move()
                );
                target_node->left_child = backprop_node;
                // #3 playout the game, make another copy
                uint8_t reward_player = playout_game->perform_playout(rng.rand());
                // #4 back up the tree again
                while (backprop_node != NULL) {
                    backprop_node->playout_count++;
                    uint8_t parent_player_to_move = target_node->player_to_move;
                    if (target_node->parent){
                        parent_player_to_move = target_node->parent->player_to_move;
                    }
                    // give reward 2 if winning player, if draw then give reward 1 to all
                    if (reward_player == parent_player_to_move) {
                        backprop_node->reward_count += 2;
                    } else if (reward_player == 0) {
                        backprop_node->reward_count += 1;
                    }
                    backprop_node = backprop_node->parent;
                }
            }
        }
        delete playout_game;
        delete selection_game;
    }

    void SinglethreadedMCTS::search_stop()
    {

    }
    
    uint64_t SinglethreadedMCTS::get_best_move()
    {
        if (root->left_child == NULL) {
            return 0;
        }
        SearchTreeNode* curr_node = root->left_child;
        SearchTreeNode* best_node = curr_node;
        while (curr_node->right_sibling != NULL) {
            curr_node = curr_node->right_sibling;
            if (curr_node->playout_count > best_node->playout_count) {
                best_node = curr_node;
            }
        }
        return best_node->move_id;
    }

    void SinglethreadedMCTS::set_gamestate(Game* target_gamestate)
    {
        gamestate = target_gamestate;
        delete root;
        root = new SearchTreeNode(NULL, NULL, NULL, 0, 0, 0, gamestate->player_to_move());
    }

    uint8_t SinglethreadedMCTS::player_to_move()
    {
        return gamestate->player_to_move();
    }

    std::vector<uint64_t> SinglethreadedMCTS::get_moves()
    {
        return gamestate->get_moves();
    }

    void SinglethreadedMCTS::apply_move(uint64_t move_id)
    {
        gamestate->apply_move(move_id);
        SearchTreeNode* prev_node = NULL;
        SearchTreeNode* new_root = NULL;
        SearchTreeNode* predecessor_of_new_root = NULL;
        uint64_t children_reward_sum = 0;
        for (SearchTreeNode* curr = root->left_child; curr != NULL; curr = curr->right_sibling) {
            children_reward_sum += curr->reward_count;
            if (curr->move_id == move_id) {
                new_root = curr;
                predecessor_of_new_root = prev_node;
            }
            prev_node = curr;
        }
        if (!new_root) {
            delete root;
            root = new SearchTreeNode(NULL, NULL, NULL, 0, 0, move_id, gamestate->player_to_move());
            return;
        }
        if (!predecessor_of_new_root) {
            predecessor_of_new_root = root;
            root->left_child = NULL;
        }
        predecessor_of_new_root->right_sibling = new_root->right_sibling;
        delete root;
        root = new_root;
        root->parent = NULL;
        root->right_sibling = NULL;
        root->reward_count = children_reward_sum;
    }

    uint8_t SinglethreadedMCTS::get_result()
    {
        return gamestate->get_result();
    }

    uint64_t SinglethreadedMCTS::get_move_id(std::string move_string)
    {
        return gamestate->get_move_id(move_string);
    }

    std::string SinglethreadedMCTS::get_move_string(uint64_t move_id)
    {
        return gamestate->get_move_string(move_id);
    }

    void SinglethreadedMCTS::debug_print()
    {
        printf("root (%lu/%lu)  ==>", root->reward_count, root->playout_count);
        SearchTreeNode* curr_node = root->left_child;
        while (curr_node != NULL) {
            printf("  %s=(%lu/%lu)", get_move_string(curr_node->move_id).c_str(), curr_node->reward_count, curr_node->playout_count);
            curr_node = curr_node->right_sibling;
        }
        printf("\n");
    }

}