#pragma once

#include <algorithm>
#include "CoreTypes.hpp"
#include "Board.hpp"

enum class TTFlag {
    Exact      = 0,
    LowerBound = 1,
    UpperBound = 2
};

struct TTEntry {
    U64    hashKey;
    int    depth;
    int    score;
    TTFlag flag;
    Point  bestMove;
    bool   valid;
};

class TranspositionTable {
private:
    TTEntry* table;
    U64      entryCount;

public:
    TranspositionTable() : table(nullptr), entryCount(0) {}

    ~TranspositionTable() { delete[] table; }

    TranspositionTable(const TranspositionTable&)            = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    void resize(int sizeInMegabytes) {
        delete[] table;
        U64 bytes   = static_cast<U64>(sizeInMegabytes) * 1024ULL * 1024ULL;
        entryCount  = bytes / sizeof(TTEntry);
        if (entryCount == 0) entryCount = 1;
        table = new TTEntry[entryCount]();
        clear();
    }

    void clear() {
        if (!table) return;
        for (U64 i = 0; i < entryCount; ++i) {
            table[i].hashKey  = 0;
            table[i].depth    = 0;
            table[i].score    = 0;
            table[i].flag     = TTFlag::Exact;
            table[i].bestMove = Point{-1, -1};
            table[i].valid    = false;
        }
    }

    void store(U64 hash, int depth, int score, TTFlag flag, Point move) {
        if (entryCount == 0 || !table) return;
        U64 index = hash % entryCount;

        if (!table[index].valid || depth >= table[index].depth || table[index].hashKey != hash) {
            table[index].hashKey  = hash;
            table[index].depth    = depth;
            table[index].score    = score;
            table[index].flag     = flag;
            table[index].bestMove = move;
            table[index].valid    = true;
        }
    }

    bool probe(U64 hash, int depth, int alpha, int beta, int& scoreOut, Point& moveOut) const {
        if (entryCount == 0 || !table) return false;
        U64 index = hash % entryCount;

        if (table[index].valid && table[index].hashKey == hash) {
            moveOut = table[index].bestMove;
            if (table[index].depth >= depth) {
                int ttScore = table[index].score;
                if (table[index].flag == TTFlag::Exact) {
                    scoreOut = ttScore;
                    return true;
                }
                if (table[index].flag == TTFlag::LowerBound && ttScore >= beta) {
                    scoreOut = ttScore;
                    return true;
                }
                if (table[index].flag == TTFlag::UpperBound && ttScore <= alpha) {
                    scoreOut = ttScore;
                    return true;
                }
            }
        }
        return false;
    }

    bool getBestMove(U64 hash, Point& moveOut) const {
        if (entryCount == 0 || !table) return false;
        U64 index = hash % entryCount;
        if (table[index].valid && table[index].hashKey == hash) {
            moveOut = table[index].bestMove;
            return true;
        }
        return false;
    }
};

class KillerTable {
private:
    static constexpr int MaxDepth       = 64;
    static constexpr int KillersPerDepth = 2;
    Point killers[MaxDepth][KillersPerDepth];

public:
    KillerTable() { clear(); }

    void clear() {
        for (int d = 0; d < MaxDepth; ++d)
            for (int k = 0; k < KillersPerDepth; ++k)
                killers[d][k] = Point{-1, -1};
    }

    void addKiller(int depth, Point move) {
        if (depth < 0 || depth >= MaxDepth) return;
        if (killers[depth][0] == move) return;
        killers[depth][1] = killers[depth][0];
        killers[depth][0] = move;
    }

    bool isKiller(int depth, Point move) const {
        if (depth < 0 || depth >= MaxDepth) return false;
        return killers[depth][0] == move || killers[depth][1] == move;
    }
};

class HistoryTable {
private:
    static constexpr int MaxWidth  = 32;
    static constexpr int MaxHeight = 32;
    static constexpr int PieceTypes = 3;

    int history[MaxWidth][MaxHeight][PieceTypes];

    void scaleDown() {
        for (int x = 0; x < MaxWidth; ++x)
            for (int y = 0; y < MaxHeight; ++y)
                for (int p = 0; p < PieceTypes; ++p)
                    history[x][y][p] /= 2;
    }

public:
    HistoryTable() { clear(); }

    void clear() {
        for (int x = 0; x < MaxWidth; ++x)
            for (int y = 0; y < MaxHeight; ++y)
                for (int p = 0; p < PieceTypes; ++p)
                    history[x][y][p] = 0;
    }

    void addHistory(int x, int y, Piece piece, int depth) {
        if (x < 0 || x >= MaxWidth || y < 0 || y >= MaxHeight) return;
        int pieceIdx = static_cast<int>(piece);
        if (pieceIdx < 0 || pieceIdx >= PieceTypes) return;

        history[x][y][pieceIdx] += depth * depth;

        if (history[x][y][pieceIdx] > 1000000) {
            scaleDown();
        }
    }

    int getScore(int x, int y, Piece piece) const {
        if (x < 0 || x >= MaxWidth || y < 0 || y >= MaxHeight) return 0;
        int pieceIdx = static_cast<int>(piece);
        if (pieceIdx < 0 || pieceIdx >= PieceTypes) return 0;
        return history[x][y][pieceIdx];
    }
};
