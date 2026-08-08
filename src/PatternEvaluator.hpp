#pragma once

#include <algorithm>
#include "CoreTypes.hpp"
#include "Board.hpp"
#include "Weights.hpp"

class PatternEvaluator {
private:
    static constexpr int WinScore = 1000000;
    EngineWeights w;

    int calculateWindowScore(int pieceCount, int openEnds, int winLength) const {
        if (pieceCount >= winLength)         return WinScore;

        if (pieceCount == winLength - 1) {
            if (openEnds == 2) return w.openFour;
            if (openEnds == 1) return w.blockedFour;
        } else if (pieceCount == winLength - 2) {
            if (openEnds == 2) return w.openThree;
            if (openEnds == 1) return w.blockedThree;
        } else if (pieceCount == winLength - 3) {
            if (openEnds == 2) return w.openTwo;
            if (openEnds == 1) return w.blockedTwo;
        } else if (pieceCount == winLength - 4) {
            return w.singlePiece;
        }

        return 0;
    }

public:
    PatternEvaluator() {
        w.loadFromFile("weights.txt");
    }

    void setWeights(const EngineWeights& newWeights) {
        w = newWeights;
    }

    const EngineWeights& getWeights() const {
        return w;
    }
    int evaluate(const Board& board, Piece sideToMove) const {
        int width     = board.getWidth();
        int height    = board.getHeight();
        int winLength = board.getWinLength();

        int scorePlayerX = 0;
        int scorePlayerO = 0;

        const int directionsX[4] = {1, 0, 1,  1};
        const int directionsY[4] = {0, 1, 1, -1};

        for (int dir = 0; dir < 4; ++dir) {
            int dx = directionsX[dir];
            int dy = directionsY[dir];

            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    int endX = x + (winLength - 1) * dx;
                    int endY = y + (winLength - 1) * dy;

                    if (!board.inBounds(endX, endY)) continue;

                    int countX = 0;
                    int countO = 0;

                    for (int k = 0; k < winLength; ++k) {
                        Piece piece = board.pieceAtUnsafe(x + k * dx, y + k * dy);
                        if      (piece == Piece::PlayerX) countX++;
                        else if (piece == Piece::PlayerO) countO++;
                    }

                    // Contaminated window containing both pieces offers no pattern value
                    if (countX > 0 && countO > 0) continue;

                    if (countX > 0) {
                        int leftX  = x - dx;
                        int leftY  = y - dy;
                        int rightX = x + winLength * dx;
                        int rightY = y + winLength * dy;

                        int openEnds = 0;
                        if (board.inBounds(leftX,  leftY)  && board.pieceAtUnsafe(leftX,  leftY)  == Piece::Empty) openEnds++;
                        if (board.inBounds(rightX, rightY) && board.pieceAtUnsafe(rightX, rightY) == Piece::Empty) openEnds++;

                        scorePlayerX += calculateWindowScore(countX, openEnds, winLength);
                    } else if (countO > 0) {
                        int leftX  = x - dx;
                        int leftY  = y - dy;
                        int rightX = x + winLength * dx;
                        int rightY = y + winLength * dy;

                        int openEnds = 0;
                        if (board.inBounds(leftX,  leftY)  && board.pieceAtUnsafe(leftX,  leftY)  == Piece::Empty) openEnds++;
                        if (board.inBounds(rightX, rightY) && board.pieceAtUnsafe(rightX, rightY) == Piece::Empty) openEnds++;

                        scorePlayerO += calculateWindowScore(countO, openEnds, winLength);
                    }
                }
            }
        }

        int netScore = scorePlayerX - scorePlayerO;
        if (sideToMove == Piece::PlayerO) {
            netScore = -netScore;
        }

        return netScore;
    }
};
