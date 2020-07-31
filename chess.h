#ifndef CHESS_H
#define CHESS_H

#include <cstdint>

#define MAX_MOVES 256

using score = int64_t;

enum kind : uint8_t {
    INVALID_KIND = 0,
    PAWN = 2,
    KNIGHT = 3,
    BISHOP = 4,
    ROOK = 5,
    QUEEN = 6,
    KING = 7
};

extern const score piece_values[8];

enum color : uint8_t {
    WHITE = 0,
    BLACK = 8,
    INVALID_COLOR = 15
};

enum piece : uint8_t {
    EMPTY = 0,
    WHITE_PAWN = 2,
    WHITE_KNIGHT = 3,
    WHITE_BISHOP = 4,
    WHITE_ROOK = 5,
    WHITE_QUEEN = 6,
    WHITE_KING = 7,
    BLACK_PAWN = 10,
    BLACK_KNIGHT = 11,
    BLACK_BISHOP = 12,
    BLACK_ROOK = 13,
    BLACK_QUEEN = 14,
    BLACK_KING = 15
};

struct board { 
    uint32_t rows[8];
};

using targets_set = uint64_t;
using pieces_set = uint64_t;

struct pos {
    uint8_t x : 3, y : 3, extra : 2;
};

extern const pos INVALID_POS;

struct move {
    uint8_t src_x : 3, src_y : 3, dst_x : 3, dst_y : 3;
    piece p : 4;
};

extern const move INVALID_MOVE;

struct game {
    board b;
    pieces_set pieces, white_pieces, black_pieces, white_king, black_king;
    targets_set white_targets, black_targets;
    bool white_in_check, black_in_check;
    bool white_left_castle, white_right_castle,
        black_left_castle, black_right_castle;
};

using chess_ai_decider = move(*)(const game&, color, const move*, uint8_t);

struct chess_ai {
    char name[32];
    chess_ai_decider decider;
};

void set_piece(board& b, int8_t x, int8_t y, piece p);
piece get_piece(const board& b, int8_t x, int8_t y);
piece make_piece(color c, kind k);
kind get_kind(const board& b, int8_t x, int8_t y);
kind get_kind(piece p);
color get_color(const board& b, int8_t x, int8_t y);
color get_color(piece p);

pos pos_of(int8_t x, int8_t y);
move move_of(int8_t src_x, int8_t src_y, int8_t dst_x, int8_t dst_y, piece p);
bool is_castle(move m);
bool is_promotion(move m);

bool operator==(pos a, pos b);
bool operator!=(pos a, pos b);
bool operator==(move a, move b);
bool operator!=(move a, move b);

bool is_targeted(const targets_set v, int8_t x, int8_t y);
void set_targeted(targets_set& v, int8_t x, int8_t y);
targets_set find_targeted(const board& g, pieces_set ps, color c);

bool is_piece(const pieces_set v, int8_t x, int8_t y);
void set_piece(pieces_set& v, int8_t x, int8_t y);
void remove_piece(pieces_set& v, int8_t x, int8_t y);
pieces_set find_pieces(const board& g, color c);
pieces_set find_king(const board& g, color c);
void move_piece(game& g, move m);
void add_moves(const game& g, color c, move* moves, uint8_t& length);

score get_score(const game& b, color c);
void update_game_state(game& g);

void empty_game(game& g);
void setup_game(game& g);
void print_game(const game& g);

void add_ai(const char* name, chess_ai_decider decider);
const chess_ai* find_ai(const char* name);

void cmd_loop();

#endif