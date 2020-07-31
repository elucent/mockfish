#include "chess.h"
#include "ai.hpp"

int main(int argc, char** argv) {
    add_ai("random", random);
    add_ai("min_oppt_moves", min_opponent_moves);
    cmd_loop();
    return 0;
}