#include "chess.h"
#include <cstdlib>

move random(const game& g, color c, const move* moves, uint8_t length) {
    return moves[rand() % length];
}

move min_opponent_moves(const game& g, color c, const move* moves, uint8_t length) {
    move options[MAX_MOVES];
    uint8_t num_options = 0;
    uint8_t min = 255;
    for (uint8_t i = 0; i < length; i ++) {
        game copy = g;
        const move* candidate = moves + i;
        move_piece(copy, *candidate);

        move oppt_moves[MAX_MOVES];
        uint8_t num_oppt_moves = 0;
        add_moves(copy, c, oppt_moves, num_oppt_moves);
        if (num_oppt_moves < min) {
            num_options = 0;
            options[num_options ++] = *candidate;
            min = num_oppt_moves;
        }
        else if (num_oppt_moves == min) {
            options[num_options ++] = *candidate;
        }
    }
    return options[rand() % num_options];
}