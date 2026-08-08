#pragma once

#include <cstdint>
#include <array>

// ─── Primitive Types ───────────────────────────────────────────────────────────
using U64 = uint64_t;

// ─── Piece Enum ────────────────────────────────────────────────────────────────
enum class Piece : int {
    Empty   = 0,
    PlayerX = 1,
    PlayerO = 2
};

inline Piece getOppositePiece(Piece p) {
    if (p == Piece::PlayerX) return Piece::PlayerO;
    if (p == Piece::PlayerO) return Piece::PlayerX;
    return Piece::Empty;
}

// ─── Point ─────────────────────────────────────────────────────────────────────
struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

// ─── Move (Point + ordering score) ────────────────────────────────────────────
struct Move {
    int x;
    int y;
    int score;

    Move() : x(-1), y(-1), score(0) {}
    Move(int x_, int y_, int s = 0) : x(x_), y(y_), score(s) {}
};

// ─── Zobrist Hasher ────────────────────────────────────────────────────────────
// Generates pseudo-random keys at construction for Zobrist hashing.
class ZobristHasher {
private:
    static constexpr int MaxWidth  = 32;
    static constexpr int MaxHeight = 32;
    static constexpr int PieceTypes = 3; // Empty, PlayerX, PlayerO

    U64 pieceKeys[MaxWidth][MaxHeight][PieceTypes];
    U64 sideKey; // XOR'd each time the side to move changes

    // Simple LCG to avoid <random> dependency in a header
    static U64 lcg(U64& state) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state ^ (state >> 33);
    }

public:
    ZobristHasher() {
        U64 state = 0xDEADBEEFCAFEBABEULL;
        for (int x = 0; x < MaxWidth; ++x) {
            for (int y = 0; y < MaxHeight; ++y) {
                for (int p = 0; p < PieceTypes; ++p) {
                    pieceKeys[x][y][p] = lcg(state);
                }
            }
        }
        sideKey = lcg(state);
    }

    U64 getPieceKey(int x, int y, Piece piece) const {
        int p = static_cast<int>(piece);
        return pieceKeys[x][y][p];
    }

    U64 getSideKey() const {
        return sideKey;
    }
};
