#pragma once

#include <algorithm>
#include "CoreTypes.hpp"
#include "Board.hpp"

struct MoveList {
    static constexpr int MaxMoves = 1024;
    Move moves[MaxMoves];
    int count;

    MoveList() : count(0) {}

    void clear() { count = 0; }

    void add(int x, int y, int score) {
        if (count < MaxMoves) {
            moves[count].x     = x;
            moves[count].y     = y;
            moves[count].score = score;
            count++;
        }
    }

    int  size()  const { return count; }
    bool empty() const { return count == 0; }

    Move& operator[](int index)             { return moves[index]; }
    const Move& operator[](int index) const { return moves[index]; }

    void sort() {
        std::sort(moves, moves + count, [](const Move& a, const Move& b) {
            return a.score > b.score;
        });
    }
};

class MoveGenerator {
private:
    static constexpr int NeighborhoodRadius = 2;

    int getLineScore(int consecutive) const {
        if (consecutive >= 4) return 100000;
        if (consecutive == 3) return 5000;
        if (consecutive == 2) return 500;
        if (consecutive == 1) return 50;
        return 0;
    }

    int countRayInDir(const Board& board, int startX, int startY, int dx, int dy, Piece targetPiece) const {
        int consecutiveCount = 0;
        int currentX = startX + dx;
        int currentY = startY + dy;

        while (board.inBounds(currentX, currentY) && board.pieceAtUnsafe(currentX, currentY) == targetPiece) {
            consecutiveCount++;
            currentX += dx;
            currentY += dy;
        }
        return consecutiveCount;
    }

    int quickEvaluatePoint(const Board& board, int x, int y, Piece player) const {
        Piece opponent = getOppositePiece(player);
        int totalScore = 0;

        const int directionsX[4] = {1, 0, 1,  1};
        const int directionsY[4] = {0, 1, 1, -1};

        for (int d = 0; d < 4; ++d) {
            int dx = directionsX[d];
            int dy = directionsY[d];

            int attackCount  = countRayInDir(board, x, y,  dx,  dy, player)
                             + countRayInDir(board, x, y, -dx, -dy, player);

            int defenseCount = countRayInDir(board, x, y,  dx,  dy, opponent)
                             + countRayInDir(board, x, y, -dx, -dy, opponent);

            totalScore += getLineScore(attackCount);
            totalScore += getLineScore(defenseCount);
        }

        return totalScore;
    }

public:
    void generateMoves(const Board& board, MoveList& moveList) const {
        moveList.clear();

        int width  = board.getWidth();
        int height = board.getHeight();

        if (board.getMoveCount() == 0) {
            int centerX = width  / 2;
            int centerY = height / 2;
            moveList.add(centerX, centerY, 1000);
            return;
        }

        Piece currentTurn = board.getCurrentTurn();

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                if (board.pieceAtUnsafe(x, y) == Piece::Empty) {
                    if (board.hasNeighbor(x, y, NeighborhoodRadius)) {
                        int moveScore = quickEvaluatePoint(board, x, y, currentTurn);
                        moveList.add(x, y, moveScore);
                    }
                }
            }
        }

        moveList.sort();
    }
};
