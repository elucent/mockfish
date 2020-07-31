#include "chess.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

const score piece_values[8] = {
    0, // empty
    0, 
    1, // pawn
    4, // knight
    3, // bishop
    3, // rook
    10, // queen
    10, // king
};


void set_piece(board& b, int8_t x, int8_t y, piece p) {
    b.rows[y] &= ~(15 << (4 * x));
    b.rows[y] |= p << (4 * x);
}

piece get_piece(const board& b, int8_t x, int8_t y) {
    return piece(b.rows[y] >> (4 * x) & 15);
}

piece make_piece(color c, kind k) {
    return piece(c | k);
}

kind get_kind(const board& b, int8_t x, int8_t y) {
    return kind(b.rows[y] >> (4 * x) & 7);
}

kind get_kind(piece p) {
    return kind(p & 7);
}

color get_color(const board& b, int8_t x, int8_t y) {
    return color(b.rows[y] >> (4 * x) & 8);
}

color get_color(piece p) {
    return color(p & 8);
}

const pos INVALID_POS = { 7u, 7u, 3u }; // all 1s

pos pos_of(int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return INVALID_POS;
    return { (uint8_t)x, (uint8_t)y, 0 };
}

const move INVALID_MOVE = { 7u, 7u, 7u, 7u, EMPTY }; // empty

move move_of(int8_t src_x, int8_t src_y, int8_t dst_x, int8_t dst_y, piece p) {
    if (dst_x < 0 || dst_x >= 8 || dst_y < 0 || dst_y >= 8) return INVALID_MOVE;
    return { (uint8_t)src_x, (uint8_t)src_y, (uint8_t)dst_x, (uint8_t)dst_y, p };
}

bool is_castle(move m) {
    // king moving more than one square
    return get_kind(m.p) == KING && m.dst_x != m.src_x - 1 && m.dst_x != m.src_x + 1;
}

bool is_promotion(move m) {
    // pawn moving to opposite row
    return get_kind(m.p) == PAWN && m.dst_y == (get_color(m.p) == WHITE ? 7 : 0);
}

bool operator==(pos a, pos b) {
    return a.x == b.x && a.y == b.y;
}

bool operator!=(pos a, pos b) {
    return !(a == b);
}

bool operator==(move a, move b) {
    return a.src_x == b.src_x && a.src_y == b.src_y && a.dst_x == b.dst_x && a.dst_y == b.dst_y && a.p == b.p;
}

bool operator!=(move a, move b) {
    return !(a == b);
}

bool is_targeted(const targets_set v, int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;
    return v >> (y * 8 + x) & 1ul;
}

void set_targeted(targets_set& v, int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return;
    v |= 1ul << uint8_t(y * 8 + x);
}

static void threaten(pieces_set ps, targets_set& v, piece p, int8_t x, int8_t y) {
    switch (get_kind(p)) {
        case PAWN:
            if (get_color(p) == BLACK) // top-down
                set_targeted(v, x - 1, y - 1), set_targeted(v, x + 1, y - 1);
            else // bottom-up
                set_targeted(v, x - 1, y + 1), set_targeted(v, x + 1, y + 1);
            return;
        case KNIGHT:
            set_targeted(v, x - 1, y - 2);
            set_targeted(v, x + 1, y - 2);
            set_targeted(v, x - 1, y + 2);
            set_targeted(v, x + 1, y + 2);
            set_targeted(v, x - 2, y - 1);
            set_targeted(v, x - 2, y + 1);
            set_targeted(v, x + 2, y - 1);
            set_targeted(v, x + 2, y + 1);
            return;
        case BISHOP:
            for (int8_t vx = x - 1, vy = y - 1; vx >= 0 && vy >= 0; vx --, vy --) { // upper left
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y - 1; vx < 8 && vy >= 0; vx ++, vy --) { // upper right
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y + 1; vx < 8 && vy < 8; vx ++, vy ++) { // lower right
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1, vy = y + 1; vx >= 0 && vy < 8; vx --, vy ++) { // lower left
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            return;
        case ROOK:
            for (int8_t vx = x - 1; vx >= 0; vx --) {
                set_targeted(v, vx, y);
                if (is_piece(ps, vx, y)) break; // if piece there
            }
            for (int8_t vx = x + 1; vx < 8; vx ++) {
                set_targeted(v, vx, y);
                if (is_piece(ps, vx, y)) break; // if piece there
            }
            for (int8_t vy = y - 1; vy >= 0; vy --) {
                set_targeted(v, x, vy);
                if (is_piece(ps, x, vy)) break; // if piece there
            }
            for (int8_t vy = y + 1; vy < 8; vy ++) {
                set_targeted(v, x, vy);
                if (is_piece(ps, x, vy)) break; // if piece there
            }
            return;
        case QUEEN:
            for (int8_t vx = x - 1, vy = y - 1; vx >= 0 && vy >= 0; vx --, vy --) { // upper left
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y - 1; vx < 8 && vy >= 0; vx ++, vy --) { // upper right
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y + 1; vx < 8 && vy < 8; vx ++, vy ++) { // lower right
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1, vy = y + 1; vx >= 0 && vy < 8; vx --, vy ++) { // lower left
                set_targeted(v, vx, vy);
                if (is_piece(ps, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1; vx >= 0; vx --) {
                set_targeted(v, vx, y);
                if (is_piece(ps, vx, y)) break; // if piece there
            }
            for (int8_t vx = x + 1; vx < 8; vx ++) {
                set_targeted(v, vx, y);
                if (is_piece(ps, vx, y)) break; // if piece there
            }
            for (int8_t vy = y - 1; vy >= 0; vy --) {
                set_targeted(v, x, vy);
                if (is_piece(ps, x, vy)) break; // if piece there
            }
            for (int8_t vy = y + 1; vy < 8; vy ++) {
                set_targeted(v, x, vy);
                if (is_piece(ps, x, vy)) break; // if piece there
            }
            return;
        case KING:
            set_targeted(v, x - 1, y - 1); // these lines look like a crown :3
            set_targeted(v, x, y - 1);
            set_targeted(v, x + 1, y - 1);
            set_targeted(v, x - 1, y);
            set_targeted(v, x + 1, y);
            set_targeted(v, x - 1, y + 1);
            set_targeted(v, x, y + 1);
            set_targeted(v, x + 1, y + 1);
            return;
        default: return;
    }
}

targets_set find_targeted(const board& b, pieces_set ps, color c) {
    targets_set v = 0;
    for (int8_t x = 0; x < 8; x ++) {
        for (int8_t y = 0; y < 8; y ++) {
            piece p = get_piece(b, x, y);
            if (get_color(p) == c) threaten(ps, v, p, x, y);
        }
    }
    return v;
}

bool is_piece(const pieces_set v, int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;
    return v >> (y * 8 + x) & 1ul;
}

void set_piece(pieces_set& v, int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return;
    v |= 1ul << (y * 8 + x);
}

void remove_piece(pieces_set& v, int8_t x, int8_t y) {
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return;
    v &= ~(1ul << (y * 8 + x));
}

pieces_set find_pieces(const board& b, color c) {
    pieces_set v = 0;
    for (int8_t x = 0; x < 8; x ++) {
        for (int8_t y = 0; y < 8; y ++) {
            if (get_color(b, x, y) == c && get_piece(b, x, y)) set_piece(v, x, y);
        }
    }
    return v;
}

pieces_set find_king(const board& b, color c) {
    pieces_set v = 0;
    for (int8_t x = 0; x < 8; x ++) {
        for (int8_t y = 0; y < 8; y ++) {
            piece p = get_piece(b, x, y);
            if (get_color(p) == c && get_kind(p) == KING) set_piece(v, x, y);
        }
    }
    return v;
}

void move_piece(game& g, move m) {
    piece p = m.p;
    set_piece(g.b, m.src_x, m.src_y, EMPTY);
    set_piece(g.b, m.dst_x, m.dst_y, p);

    if (get_kind(p) == ROOK) {
        if (m.src_x == 0)
            get_color(p) == BLACK ? g.black_left_castle : g.white_left_castle = false;
        if (m.src_x == 7)
            get_color(p) == BLACK ? g.black_right_castle : g.white_right_castle = false;
    }
    if (get_kind(p) == KING) {
        get_color(p) == BLACK ? g.black_left_castle : g.white_left_castle = false;
        get_color(p) == BLACK ? g.black_right_castle : g.white_right_castle = false;
    }
    if (is_castle(m)) { // move rooks
        if (m.dst_x < m.src_x) // queenside
            move_piece(g, { 0, m.dst_y, 3, m.dst_y, get_piece(g.b, 0, m.dst_y) });
        if (m.dst_x > m.src_x) // kingside
            move_piece(g, { 7, m.dst_y, 5, m.dst_y, get_piece(g.b, 7, m.dst_y) });
    }
    update_game_state(g);
}

void add_move(move* moves, uint8_t& length, move m) {
    if (m != INVALID_MOVE) moves[length ++] = m;
}

void try_promotion(move* moves, uint8_t& length, move m) {
    uint8_t limit = get_color(m.p) == WHITE ? 7 : 0;
    if (m.dst_y == limit) {
        for (uint8_t k = uint8_t(KNIGHT); k < uint8_t(KING); k ++) {
            move n = m;
            n.p = make_piece(get_color(m.p), kind(k));
            add_move(moves, length, n);
        }
    }
    else add_move(moves, length, m);
}

void add_moves(const game& g, color c, int8_t x, int8_t y, move* moves, uint8_t& length) {
    pieces_set enemies = c == WHITE ? g.black_pieces : g.white_pieces; 
    pieces_set allies = c == WHITE ? g.white_pieces : g.black_pieces;
    piece p = get_piece(g.b, x, y);
    switch (get_kind(p)) {
        case PAWN:
            if (c == WHITE) {
                if (!is_piece(g.pieces, x, y + 1)) {
                    try_promotion(moves, length, move_of(x, y, x, y + 1, p));
                    if (y == 1 && !is_piece(g.pieces, x, y + 2)) // white starting line
                        add_move(moves, length, move_of(x, y, x, y + 2, p));
                }
                if (is_piece(enemies, x - 1, y + 1)) try_promotion(moves, length, move_of(x, y, x - 1, y + 1, p));
                if (is_piece(enemies, x + 1, y + 1)) try_promotion(moves, length, move_of(x, y, x + 1, y + 1, p));
            }
            else {
                if (!is_piece(g.pieces, x, y - 1)) {
                    try_promotion(moves, length, move_of(x, y, x, y - 1, p));
                    if (y == 6 && !is_piece(g.pieces, x, y - 2)) // black starting line 
                        add_move(moves, length, move_of(x, y, x, y - 2, p));
                }
                if (is_piece(enemies, x - 1, y - 1)) try_promotion(moves, length, move_of(x, y, x - 1, y - 1, p));
                if (is_piece(enemies, x + 1, y - 1)) try_promotion(moves, length, move_of(x, y, x + 1, y - 1, p));
            }
            return;
        case KNIGHT:
            if (!is_piece(allies, x - 1, y - 2)) add_move(moves, length, move_of(x, y, x - 1, y - 2, p));
            if (!is_piece(allies, x + 1, y - 2)) add_move(moves, length, move_of(x, y, x + 1, y - 2, p));
            if (!is_piece(allies, x - 1, y + 2)) add_move(moves, length, move_of(x, y, x - 1, y + 2, p));
            if (!is_piece(allies, x + 1, y + 2)) add_move(moves, length, move_of(x, y, x + 1, y + 2, p));
            if (!is_piece(allies, x - 2, y - 1)) add_move(moves, length, move_of(x, y, x - 2, y - 1, p));
            if (!is_piece(allies, x - 2, y + 1)) add_move(moves, length, move_of(x, y, x - 2, y + 1, p));
            if (!is_piece(allies, x + 2, y - 1)) add_move(moves, length, move_of(x, y, x + 2, y - 1, p));
            if (!is_piece(allies, x + 2, y + 1)) add_move(moves, length, move_of(x, y, x + 2, y + 1, p));
            return;
        case BISHOP:
            for (int8_t vx = x - 1, vy = y - 1; vx >= 0 && vy >= 0; vx --, vy --) { // upper left
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y - 1; vx < 8 && vy >= 0; vx ++, vy --) { // upper right
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y + 1; vx < 8 && vy < 8; vx ++, vy ++) { // lower right
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1, vy = y + 1; vx >= 0 && vy < 8; vx --, vy ++) { // lower left
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            return;
        case ROOK:
            for (int8_t vx = x - 1; vx >= 0; vx --) {
                if (!is_piece(allies, vx, y)) add_move(moves, length, move_of(x, y, vx, y, p));
                if (is_piece(g.pieces, vx, y)) break; // if piece there
            }
            for (int8_t vx = x + 1; vx < 8; vx ++) {
                if (!is_piece(allies, vx, y)) add_move(moves, length, move_of(x, y, vx, y, p));
                if (is_piece(g.pieces, vx, y)) break; // if piece there
            }
            for (int8_t vy = y - 1; vy >= 0; vy --) {
                if (!is_piece(allies, x, vy)) add_move(moves, length, move_of(x, y, x, vy, p));
                if (is_piece(g.pieces, x, vy)) break; // if piece there
            }
            for (int8_t vy = y + 1; vy < 8; vy ++) {
                if (!is_piece(allies, x, vy)) add_move(moves, length, move_of(x, y, x, vy, p));
                if (is_piece(g.pieces, x, vy)) break; // if piece there
            }
            return;
        case QUEEN:
            for (int8_t vx = x - 1, vy = y - 1; vx >= 0 && vy >= 0; vx --, vy --) { // upper left
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y - 1; vx < 8 && vy >= 0; vx ++, vy --) { // upper right
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x + 1, vy = y + 1; vx < 8 && vy < 8; vx ++, vy ++) { // lower right
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1, vy = y + 1; vx >= 0 && vy < 8; vx --, vy ++) { // lower left
                if (!is_piece(allies, vx, vy)) add_move(moves, length, move_of(x, y, vx, vy, p));
                if (is_piece(g.pieces, vx, vy)) break; // if piece there
            }
            for (int8_t vx = x - 1; vx >= 0; vx --) {
                if (!is_piece(allies, vx, y)) add_move(moves, length, move_of(x, y, vx, y, p));
                if (is_piece(g.pieces, vx, y)) break; // if piece there
            }
            for (int8_t vx = x + 1; vx < 8; vx ++) {
                if (!is_piece(allies, vx, y)) add_move(moves, length, move_of(x, y, vx, y, p));
                if (is_piece(g.pieces, vx, y)) break; // if piece there
            }
            for (int8_t vy = y - 1; vy >= 0; vy --) {
                if (!is_piece(allies, x, vy)) add_move(moves, length, move_of(x, y, x, vy, p));
                if (is_piece(g.pieces, x, vy)) break; // if piece there
            }
            for (int8_t vy = y + 1; vy < 8; vy ++) {
                if (!is_piece(allies, x, vy)) add_move(moves, length, move_of(x, y, x, vy, p));
                if (is_piece(g.pieces, x, vy)) break; // if piece there
            }
            return;
        case KING:
            if (!is_piece(allies, x - 1, y - 1))    add_move(moves, length, move_of(x, y, x - 1, y - 1, p)); // these lines look like a crown :3
            if (!is_piece(allies, x, y - 1))        add_move(moves, length, move_of(x, y, x, y - 1, p));
            if (!is_piece(allies, x + 1, y - 1))    add_move(moves, length, move_of(x, y, x + 1, y - 1, p));
            if (!is_piece(allies, x - 1, y))        add_move(moves, length, move_of(x, y, x - 1, y, p));
            if (!is_piece(allies, x + 1, y))        add_move(moves, length, move_of(x, y, x + 1, y, p));
            if (!is_piece(allies, x - 1, y + 1))    add_move(moves, length, move_of(x, y, x - 1, y + 1, p));
            if (!is_piece(allies, x, y + 1))        add_move(moves, length, move_of(x, y, x, y + 1, p));
            if (!is_piece(allies, x + 1, y + 1))    add_move(moves, length, move_of(x, y, x + 1, y + 1, p));
            {
                bool left_castle = c == WHITE ? g.white_left_castle : g.black_left_castle;
                bool right_castle = c == WHITE ? g.white_right_castle : g.black_right_castle;
                if (left_castle && !(c == WHITE ? g.white_in_check : g.black_in_check)) {
                    bool open = true;
                    for (uint8_t i = x - 1; i > 0; i --) if (is_piece(g.pieces, i, y)) open = false;
                    if (open) add_move(moves, length, move_of(x, y, x - 2, y, p)); 
                }
                if (right_castle && !(c == WHITE ? g.white_in_check : g.black_in_check)) {
                    bool open = true;
                    for (uint8_t i = x + 1; i < 7; i ++) if (is_piece(g.pieces, i, y)) open = false;
                    if (open) add_move(moves, length, move_of(x, y, x + 2, y, p)); 
                }
            }
            return;
        default: return;
    }
}

void add_moves(const game& g, color c, move* moves, uint8_t& length) {
    for (int8_t x = 0; x < 8; x ++) {
        for (int8_t y = 0; y < 8; y ++) {
            if (get_color(g.b, x, y) == c) add_moves(g, c, x, y, moves, length);
        }
    }
    move* writer = moves;
    const move* reader = moves;
    for (uint8_t i = 0; i < length; i ++) {
        move m = reader[i];
        game copy = g;
        move_piece(copy, m);
        if (c == WHITE && !copy.white_in_check) *writer++ = m;
        else if (c == BLACK && !copy.black_in_check) *writer++ = m;
    }
    length = writer - moves;
}

void update_game_state(game& g) {
    g.white_pieces = find_pieces(g.b, WHITE), g.black_pieces = find_pieces(g.b, BLACK);
    g.white_king = find_king(g.b, WHITE), g.black_king = find_king(g.b, BLACK);
    g.pieces = g.white_pieces | g.black_pieces;
    g.white_targets = find_targeted(g.b, g.pieces, WHITE), g.black_targets = find_targeted(g.b, g.pieces, BLACK); // compute enemy targets
    g.white_in_check = g.white_king & g.black_targets;
    g.black_in_check = g.black_king & g.white_targets;
}

void empty_game(game& g) {
    g.pieces = g.white_pieces = g.black_pieces = g.white_king = g.black_king = 0;
    g.white_in_check = g.black_in_check = false;
    g.white_targets = g.black_targets = 0;
    g.black_left_castle = g.black_right_castle = false;
    g.white_left_castle = g.white_right_castle = false;

    for (uint8_t i = 0; i < 8; i ++) g.b.rows[i] = 0;
}

void setup_game(game& g) {
    empty_game(g);

    g.black_left_castle = g.black_right_castle = true;
    g.white_left_castle = g.white_right_castle = true;

    set_piece(g.b, 0, 0, WHITE_ROOK);
    set_piece(g.b, 1, 0, WHITE_KNIGHT);
    set_piece(g.b, 2, 0, WHITE_BISHOP);
    set_piece(g.b, 3, 0, WHITE_QUEEN);
    set_piece(g.b, 4, 0, WHITE_KING);
    set_piece(g.b, 5, 0, WHITE_BISHOP);
    set_piece(g.b, 6, 0, WHITE_KNIGHT);
    set_piece(g.b, 7, 0, WHITE_ROOK);
    for (uint8_t i = 0; i < 8; i ++) set_piece(g.b, i, 1, WHITE_PAWN); // white pawns

    for (uint8_t i = 0; i < 8; i ++) set_piece(g.b, i, 6, BLACK_PAWN); // black pawns
    set_piece(g.b, 0, 7, BLACK_ROOK);
    set_piece(g.b, 1, 7, BLACK_KNIGHT);
    set_piece(g.b, 2, 7, BLACK_BISHOP);
    set_piece(g.b, 3, 7, BLACK_QUEEN);
    set_piece(g.b, 4, 7, BLACK_KING);
    set_piece(g.b, 5, 7, BLACK_BISHOP);
    set_piece(g.b, 6, 7, BLACK_KNIGHT);
    set_piece(g.b, 7, 7, BLACK_ROOK);

    update_game_state(g);
}

const char* piece_icons[] = {
    " ", " ",
    "♙", "♘", "♗", "♖", "♕", "♔",
    " ", " ",
    "♟︎", "♞", "♝", "♜", "♛", "♚"
};

void print_game(const game& g) {
    printf("  abcdefgh \n");
    printf(" ╔════════╗\n");
    for (uint8_t i = 0; i < 8; i ++) {
        printf("%u║%s%s%s%s%s%s%s%s║\n", i + 1,
            piece_icons[get_piece(g.b, 0, i)],
            piece_icons[get_piece(g.b, 1, i)],
            piece_icons[get_piece(g.b, 2, i)],
            piece_icons[get_piece(g.b, 3, i)],
            piece_icons[get_piece(g.b, 4, i)],
            piece_icons[get_piece(g.b, 5, i)],
            piece_icons[get_piece(g.b, 6, i)],
            piece_icons[get_piece(g.b, 7, i)]);
    }
    printf(" ╚════════╝\n");
}

void print_targets(const game& g, targets_set set) {
    printf("  abcdefgh \n");
    printf(" ╔════════╤════════╗\n");
    for (uint8_t i = 0; i < 8; i ++) {
        printf("%u║%s%s%s%s%s%s%s%s┆%c%c%c%c%c%c%c%c║\n", i + 1,
            piece_icons[get_piece(g.b, 0, i)],
            piece_icons[get_piece(g.b, 1, i)],
            piece_icons[get_piece(g.b, 2, i)],
            piece_icons[get_piece(g.b, 3, i)],
            piece_icons[get_piece(g.b, 4, i)],
            piece_icons[get_piece(g.b, 5, i)],
            piece_icons[get_piece(g.b, 6, i)],
            piece_icons[get_piece(g.b, 7, i)],
            is_targeted(set, 0, i) ? 'X' : ' ', is_targeted(set, 1, i) ? 'X' : ' ', 
            is_targeted(set, 2, i) ? 'X' : ' ', is_targeted(set, 3, i) ? 'X' : ' ',
            is_targeted(set, 4, i) ? 'X' : ' ', is_targeted(set, 5, i) ? 'X' : ' ', 
            is_targeted(set, 6, i) ? 'X' : ' ', is_targeted(set, 7, i) ? 'X' : ' ');
    }
    printf(" ╚════════╧════════╝\n");
}

void print_targets(const game& g, color c) {
    print_targets(g, c == WHITE ? g.white_targets : g.black_targets);
}

color color_from_string(const char* color_name) {
    if (!color_name) return INVALID_COLOR;

    if (!strcmp(color_name, "white")) return WHITE;
    else if (!strcmp(color_name, "black")) return BLACK;
    else return INVALID_COLOR;
}

kind kind_from_string(const char* kind_name) {
    if (!kind_name) return INVALID_KIND;

    if (!strcmp(kind_name, "pawn")) return PAWN;
    else if (!strcmp(kind_name, "knight")) return KNIGHT;
    else if (!strcmp(kind_name, "bishop")) return BISHOP;
    else if (!strcmp(kind_name, "rook")) return ROOK;
    else if (!strcmp(kind_name, "queen")) return QUEEN;
    else if (!strcmp(kind_name, "king")) return KING;
    else return INVALID_KIND;
}

pos pos_from_string(const char* pos_string) {
    if (!pos_string[0] || !pos_string[1])
        return INVALID_POS;
    char c = pos_string[0];
    if (c >= 'a') c -= 32;
    int8_t x = c - 'A';
    int8_t y = pos_string[1] - '1';
    if (x < 0 || y < 0 || x >= 8 || y >= 8) return INVALID_POS;
    return pos_of(x, y);
}

chess_ai ai_array[256];
uint8_t ai_length = 0;

void add_ai(const char* name, chess_ai_decider decider) {
    if (ai_length == 255) {
        fprintf(stderr, "Max AI limit reached.\n");
        return;
    }
    chess_ai ai;
    strncpy(ai.name, name, 32);
    ai.decider = decider;
    ai_array[ai_length ++] = ai;
}

const chess_ai* find_ai(const char* name) {
    for (uint8_t i = 0; i < ai_length; i ++) {
        if (!strcmp(ai_array[i].name, name)) return ai_array + i;
    }
    return nullptr;
}

void cmd_loop() {
    srand(time(0));

    game g;
    setup_game(g);

    bool done = false;
    printf(R"(
╔══════════════════════════════════════╗
║                                      ║
║   𝓜𝓸𝓬𝓴𝓯𝓲𝓼𝓱 - Version 0.1             ║
║                                      ║
╚══════════════════════════════════════╝

Welcome to the Mockfish Chess engine! Type commands below, or 'help' to get started!
)");
    while (!done) {
        printf("➤ ");
        char buffer[512], *writer = buffer, ch;
        while ((ch = fgetc(stdin)) != '\n') *writer ++ = ch;
        *writer ++ = ' ';

        const char* cmd = strtok(buffer, " \r\t");
        if (!strcmp(cmd, "help")) {
            printf("Commands:\n");
            printf("➤ help\n");
            printf("\tDisplays this message.\n");
            printf("➤ print\n");
            printf("\tDisplays the board.\n");
            printf("➤ reset\n");
            printf("\tResets board to initial position.\n");
            printf("➤ clear\n");
            printf("\tRemoves all pieces from the board.\n");
            printf("➤ place <color> <piece> at <pos>\n");
            printf("\tAdds a piece of the provided color to the board.\n");
            printf("➤ remove piece at <pos>\n");
            printf("\tRemoves a piece from the board.\n");
            printf("➤ move <pos> to <pos>\n");
            printf("\tMoves a piece to a new position.\n");
            printf("➤ quit\n");
            printf("\tCloses the program.\n");
            printf("\n");
            printf("Parameters:\n");
            printf(" - color: either 'white' or 'black'\n");
            printf(" - piece: any of 'pawn', 'knight', 'bishop', 'rook', 'queen', or 'king'\n");
            printf(" - pos: any coordinate of the form [A-Ha-h][1-8], e.g. 'A2', 'e6'\n");
            printf("\n");
        }
        else if (!strcmp(cmd, "print")) {
            print_game(g);
        }
        else if (!strcmp(cmd, "reset")) {
            setup_game(g);
            print_game(g);
            printf("Reset pieces to initial positions.\n");
        }
        else if (!strcmp(cmd, "clear")) {
            empty_game(g);
            print_game(g);
            printf("Cleared board.\n");
        }
        else if (!strcmp(cmd, "place")) {
            color c = color_from_string(strtok(nullptr, " \r\t"));
            kind k = kind_from_string(strtok(nullptr, " \r\t"));
            const char* at = strtok(nullptr, " \r\t");
            pos p = pos_from_string(strtok(nullptr, " \r\t"));
            if (c == INVALID_COLOR || k == INVALID_KIND || strcmp(at, "at") || p == INVALID_POS) {
                fprintf(stderr, "Usage: place <color> <piece> at <pos>\n");
                fprintf(stderr, " - color: either 'white' or 'black'\n");
                fprintf(stderr, " - piece: any of 'pawn', 'knight', 'bishop', 'rook', 'queen', or 'king'\n");
                fprintf(stderr, " - pos: any coordinate of the form [A-Ha-h][1-8], e.g. 'A2', 'e6'\n");
                continue;
            }
            set_piece(g.b, p.x, p.y, make_piece(c, k));
            update_game_state(g);
            print_game(g);
        }
        else if (!strcmp(cmd, "remove")) {
            const char* piece = strtok(nullptr, " \r\t");
            const char* at = strtok(nullptr, " \r\t");
            pos p = pos_from_string(strtok(nullptr, " \r\t"));
            if (strcmp(piece, "piece") || strcmp(at, "at") || p == INVALID_POS) {
                fprintf(stderr, "Usage: remove piece at <pos>\n");
                fprintf(stderr, " - pos: any coordinate of the form [A-Ha-h][1-8], e.g. 'A2', 'e6'\n");
                continue;
            }
            set_piece(g.b, p.x, p.y, EMPTY);
            update_game_state(g);
            print_game(g);
        }
        else if (!strcmp(cmd, "move")) {
            pos from = pos_from_string(strtok(nullptr, " \r\t"));
            const char* to = strtok(nullptr, " \r\t");
            pos dest = pos_from_string(strtok(nullptr, " \r\t"));
            if (from == INVALID_POS || strcmp(to, "to") || dest == INVALID_POS) {
                fprintf(stderr, "Usage: move <pos> to <pos>\n");
                fprintf(stderr, " - pos: any coordinate of the form [A-Ha-h][1-8], e.g. 'A2', 'e6'\n");
                continue;
            }
            move_piece(g, { from.x, from.y, dest.x, dest.y, get_piece(g.b, from.x, from.y) });
            print_game(g);
        }
        else if (!strcmp(cmd, "moves")) {
            color c = color_from_string(strtok(nullptr, " \r\t"));
            if (c == INVALID_COLOR) {
                fprintf(stderr, "Usage: moves <color>\n");
                fprintf(stderr, " - color: either 'white' or 'black'\n");
                continue;
            }
            move moves[MAX_MOVES];
            uint8_t num_moves = 0;
            add_moves(g, c, moves, num_moves);
            targets_set endpoints = 0;
            for (uint8_t i = 0; i < num_moves; i ++) {
                set_targeted(endpoints, moves[i].dst_x, moves[i].dst_y);
            }
            print_targets(g, endpoints);

            for (uint8_t i = 0; i < num_moves;) {
                for (uint8_t j = 0; j < 4 && i < num_moves; i ++, j ++) {
                    printf("%s %c%c to %c%c\t", 
                        piece_icons[get_piece(g.b, moves[i].src_x, moves[i].src_y)],
                        'a' + moves[i].src_x, '1' + moves[i].src_y,
                        'a' + moves[i].dst_x, '1' + moves[i].dst_y);
                }
                printf("\n");
            }
        }
        else if (!strcmp(cmd, "targets")) {
            color c = color_from_string(strtok(nullptr, " \r\t"));
            if (c == INVALID_COLOR) {
                fprintf(stderr, "Usage: targets <color>\n");
                fprintf(stderr, " - color: either 'white' or 'black'\n");
                continue;
            }
            print_targets(g, c);
        }
        else if (!strcmp(cmd, "pieces")) {
            color c = color_from_string(strtok(nullptr, " \r\t"));
            if (c == INVALID_COLOR) {
                fprintf(stderr, "Usage: pieces <color>\n");
                fprintf(stderr, " - color: either 'white' or 'black'\n");
                continue;
            }
            print_targets(g, c == WHITE ? g.white_pieces : g.black_pieces);
        }
        else if (!strcmp(cmd, "check")) {
            color c = color_from_string(strtok(nullptr, " \r\t"));
            if (c == INVALID_COLOR) {
                fprintf(stderr, "Usage: check <color>\n");
                fprintf(stderr, " - color: either 'white' or 'black'\n");
                continue;
            }
            printf("%s\n", (c == WHITE ? g.white_in_check : g.black_in_check) ? "true" : "false");
        }
        else if (!strcmp(cmd, "play")) {
            const chess_ai* ai = nullptr;
            bool human = false;
            color human_color = INVALID_COLOR;
            const char* opponent = strtok(nullptr, " \r\t");
            if (!strcmp(opponent, "human")) {
                human = true;
            }
            else {
                ai = find_ai(opponent);
                if (!ai) {
                    fprintf(stderr, "Usage: play human|<ai> '%s'.\n", opponent);
                    fprintf(stderr, "Registered AI options:\n");
                    for (uint8_t i = 0; i < ai_length; i ++) fprintf(stderr, " - %s\n", ai_array[i].name);
                    continue;
                }

                human = false;
                while (human_color == INVALID_COLOR) {
                    printf("White or black?: ");
                    writer = buffer;
                    while ((ch = fgetc(stdin)) != '\n') *writer ++ = ch;
                    *writer ++ = ' ';

                    human_color = color_from_string(strtok(buffer, " \r\t"));
                    if (human_color == INVALID_COLOR) {
                        fprintf(stderr, "Please enter 'white' or 'black'.\n");
                    }
                }
            }

            color player = WHITE;
            while (true) {
                print_game(g);

                move moves[MAX_MOVES];
                uint8_t length = 0;
                add_moves(g, player, moves, length);
                if (length == 0) {
                    printf("Checkmate! %s player wins.\n", player == WHITE ? "Black" : "White");
                    break;
                }

                printf("%s player's turn.", player == WHITE ? "White" : "Black");
                if (player == WHITE ? g.white_in_check : g.black_in_check) printf(" You are in check.");
                printf("\n");

                bool moved = false;
                if (!human && player != human_color) {
                    move m = ai->decider(g, player, moves, length);
                    move_piece(g, m);
                }
                else while (!moved) {
                    printf("%s ", player == WHITE ? "⚐" : "⚑");
                    pos from, dest;
                    // use buffer from before
                    writer = buffer;
                    while ((ch = fgetc(stdin)) != '\n') *writer ++ = ch;
                    *writer ++ = ' ';

                    from = pos_from_string(strtok(buffer, " \r\t"));
                    const char* to = strtok(nullptr, " \r\t");
                    dest = pos_from_string(strtok(nullptr, " \r\t"));
                    if (from == INVALID_POS || strcmp(to, "to") || dest == INVALID_POS) {
                        fprintf(stderr, "Usage: <pos> to <pos>\n");
                        fprintf(stderr, " - pos: any coordinate of the form [A-Ha-h][1-8], e.g. 'A2', 'e6'\n");
                        continue;
                    }

                    move choice = { from.x, from.y, dest.x, dest.y, get_piece(g.b, from.x, from.y) };

                    if (is_promotion(choice)) {
                        kind k = INVALID_KIND;

                        while (k == INVALID_KIND) {
                            printf("Which piece should your pawn promote to?: ");
                            writer = buffer;
                            while ((ch = fgetc(stdin)) != '\n') *writer ++ = ch;
                            *writer ++ = ' ';

                            k = kind_from_string(strtok(buffer, " \r\t"));
                            if (k == INVALID_KIND) {
                                fprintf(stderr, "Please enter any of 'knight', 'bishop', 'rook', 'queen', or 'king'.\n");
                            }
                            if (k == PAWN) {
                                fprintf(stderr, "Cannot promote to pawn.\n");
                                k = INVALID_KIND;
                            }
                        }
                        choice.p = make_piece(get_color(choice.p), k);
                    }

                    for (uint8_t i = 0; i < length && !moved; i ++) {
                        if (moves[i] == choice) {
                            move_piece(g, moves[i]);
                            moved = true;
                        }
                    }
                    if (!moved) {
                        fprintf(stderr, "Illegal move. Cannot move piece from %c%c to %c%c.\n",
                            'a' + from.x, '1' + from.y,
                            'a' + dest.x, '1' + dest.y);
                    }
                }

                player = player == WHITE ? BLACK : WHITE;
            }
        }
        else if (!strcmp(cmd, "quit")) {
            done = true;
        }
    }
}