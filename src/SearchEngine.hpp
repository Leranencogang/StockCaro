#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include "CoreTypes.hpp"
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "SearchTables.hpp"
#include "PatternEvaluator.hpp"

struct SearchResult {
    Point bestMove;
    int   score;
    int   depth;
    U64   nodes;

    SearchResult() : bestMove(Point{-1, -1}), score(0), depth(0), nodes(0) {}
};

class SearchEngine {
private:
    TranspositionTable ttTable;
    KillerTable        killerTable;
    HistoryTable       historyTable;
    MoveGenerator      moveGen;
    PatternEvaluator   evaluator;

    bool stopFlag;
    U64  nodesVisited;
    int  maxSearchDepth;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    int  timeLimitMs;

    static constexpr int InfiniteScore = 10000000;
    static constexpr int MateScore     =  9000000;

    bool isTimeUp() const {
        if (stopFlag)         return true;
        if (timeLimitMs <= 0) return false;
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed     = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        return elapsed >= timeLimitMs;
    }

    void sortMovesWithHeuristics(const Board& board, MoveList& moveList, int depth, Point ttMove) const {
        Piece currentTurn = board.getCurrentTurn();
        for (int i = 0; i < moveList.size(); ++i) {
            Move& m = moveList[i];
            if (m.x == ttMove.x && m.y == ttMove.y) {
                m.score += 2000000;
            } else if (killerTable.isKiller(depth, Point{m.x, m.y})) {
                m.score += 500000;
            } else {
                m.score += historyTable.getScore(m.x, m.y, currentTurn);
            }
        }
        moveList.sort();
    }

    int pvs(Board& board, int depth, int alpha, int beta, bool isRoot) {
        nodesVisited++;

        if ((nodesVisited % 4096 == 0) && isTimeUp()) {
            stopFlag = true;
            return 0;
        }

        if (depth <= 0) {
            return evaluator.evaluate(board, board.getCurrentTurn());
        }

        U64   boardHash = board.getHash();
        int   ttScore   = 0;
        Point ttMove{-1, -1};

        if (!isRoot && ttTable.probe(boardHash, depth, alpha, beta, ttScore, ttMove)) {
            return ttScore;
        }

        ttTable.getBestMove(boardHash, ttMove);

        MoveList moveList;
        moveGen.generateMoves(board, moveList);

        if (moveList.empty()) {
            return 0;
        }

        sortMovesWithHeuristics(board, moveList, depth, ttMove);

        int   bestScore     = -InfiniteScore;
        Point bestMoveLocal{-1, -1};
        TTFlag flag         = TTFlag::UpperBound;

        int movesToSearch = std::min(moveList.size(), depth >= 10 ? 8 : (depth >= 6 ? 12 : 18));
        for (int i = 0; i < movesToSearch; ++i) {
            Move move = moveList[i];

            if (!board.makeMove(move.x, move.y)) continue;

            bool isWin = board.isWinAt(move.x, move.y);
            int  score = 0;

            if (isWin) {
                score = MateScore + depth;
            } else {
                int reduction = 0;
                if (i >= 3 && depth >= 3 && !killerTable.isKiller(depth, Point{move.x, move.y})) {
                    reduction = 1;
                    if (i >= 8) reduction = 2;
                }

                if (i == 0) {
                    score = -pvs(board, depth - 1, -beta, -alpha, false);
                } else {
                    score = -pvs(board, depth - 1 - reduction, -alpha - 1, -alpha, false);

                    if (score > alpha && reduction > 0) {
                        score = -pvs(board, depth - 1, -alpha - 1, -alpha, false);
                    }

                    if (score > alpha && score < beta) {
                        score = -pvs(board, depth - 1, -beta, -alpha, false);
                    }
                }
            }

            board.undoMove();

            if (stopFlag) return 0;

            if (score > bestScore) {
                bestScore     = score;
                bestMoveLocal = Point{move.x, move.y};
            }

            if (score > alpha) {
                alpha = score;
                flag  = TTFlag::Exact;

                if (alpha >= beta) {
                    flag = TTFlag::LowerBound;
                    killerTable.addKiller(depth, Point{move.x, move.y});
                    historyTable.addHistory(move.x, move.y, board.getCurrentTurn(), depth);
                    break;
                }
            }
        }

        if (!stopFlag) {
            ttTable.store(boardHash, depth, bestScore, flag, bestMoveLocal);
        }

        return bestScore;
    }

public:
    SearchEngine()
        : stopFlag(false),
          nodesVisited(0),
          maxSearchDepth(20),
          timeLimitMs(1000) {
        ttTable.resize(64);
    }

    void setEvaluatorWeights(const EngineWeights& newWeights) {
        evaluator.setWeights(newWeights);
    }

    void setTTSize(int sizeInMegabytes) { ttTable.resize(sizeInMegabytes); }

    void clearTables() {
        ttTable.clear();
        killerTable.clear();
        historyTable.clear();
    }

    SearchResult startSearch(Board& board, int maxDepth, int timeLimit) {
        stopFlag      = false;
        nodesVisited  = 0;
        maxSearchDepth = maxDepth;
        timeLimitMs   = timeLimit;
        startTime     = std::chrono::steady_clock::now();

        SearchResult finalResult;
        Point bestMoveOverall{-1, -1};
        int   bestScoreOverall = -InfiniteScore;

        for (int currentDepth = 1; currentDepth <= maxSearchDepth; ++currentDepth) {
            int alpha = -InfiniteScore;
            int beta  =  InfiniteScore;

            if (currentDepth >= 4) {
                int windowWidth = 1000;
                alpha = std::max(-InfiniteScore, bestScoreOverall - windowWidth);
                beta  = std::min( InfiniteScore, bestScoreOverall + windowWidth);
            }

            int score = pvs(board, currentDepth, alpha, beta, true);

            if (!stopFlag && (score <= alpha || score >= beta)) {
                score = pvs(board, currentDepth, -InfiniteScore, InfiniteScore, true);
            }

            if (stopFlag) break;

            Point iterationBestMove{-1, -1};
            if (ttTable.getBestMove(board.getHash(), iterationBestMove)) {
                bestMoveOverall = iterationBestMove;
            }

            bestScoreOverall = score;

            finalResult.bestMove = bestMoveOverall;
            finalResult.score    = bestScoreOverall;
            finalResult.depth    = currentDepth;
            finalResult.nodes    = nodesVisited;

            if (isTimeUp() || std::abs(bestScoreOverall) >= MateScore) break;
        }

        return finalResult;
    }
};
