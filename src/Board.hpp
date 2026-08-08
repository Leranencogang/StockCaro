#pragma once

#include <vector>
#include <algorithm>
#include "CoreTypes.hpp"

struct UndoInfo {
    Point point;
    U64 hashKey;
    Piece side;
};

class Board {
private:
    int boardWidth;
    int boardHeight;
    int targetWinLength;
    Piece currentTurn;
    U64 hashKey;

    Piece* cells;
    std::vector<UndoInfo> undoStack;
    ZobristHasher zobrist;

    inline int getIndex(int x, int y) const {
        return y * boardWidth + x;
    }

    inline int countRay(int startX, int startY, int stepX, int stepY, Piece player) const {
        int consecutiveCount = 0;
        int currentX = startX + stepX;
        int currentY = startY + stepY;

        while (inBounds(currentX, currentY) && pieceAtUnsafe(currentX, currentY) == player) {
            consecutiveCount++;
            currentX += stepX;
            currentY += stepY;
        }

        return consecutiveCount;
    }

public:
    Board()
        : boardWidth(0),
          boardHeight(0),
          targetWinLength(5),
          currentTurn(Piece::PlayerX),
          hashKey(0),
          cells(nullptr) {}

    ~Board() {
        delete[] cells;
    }

    // Copy constructor
    Board(const Board& other)
        : boardWidth(other.boardWidth),
          boardHeight(other.boardHeight),
          targetWinLength(other.targetWinLength),
          currentTurn(other.currentTurn),
          hashKey(other.hashKey),
          undoStack(other.undoStack),
          zobrist(other.zobrist) {
        if (other.cells) {
            int totalCells = boardWidth * boardHeight;
            cells = new Piece[totalCells];
            for (int i = 0; i < totalCells; ++i) {
                cells[i] = other.cells[i];
            }
        } else {
            cells = nullptr;
        }
    }

    // Copy assignment operator
    Board& operator=(const Board& other) {
        if (this != &other) {
            delete[] cells;

            boardWidth        = other.boardWidth;
            boardHeight       = other.boardHeight;
            targetWinLength   = other.targetWinLength;
            currentTurn       = other.currentTurn;
            hashKey           = other.hashKey;
            undoStack         = other.undoStack;
            zobrist           = other.zobrist;

            if (other.cells) {
                int totalCells = boardWidth * boardHeight;
                cells = new Piece[totalCells];
                for (int i = 0; i < totalCells; ++i) {
                    cells[i] = other.cells[i];
                }
            } else {
                cells = nullptr;
            }
        }
        return *this;
    }

    // Move constructor
    Board(Board&& other) noexcept
        : boardWidth(other.boardWidth),
          boardHeight(other.boardHeight),
          targetWinLength(other.targetWinLength),
          currentTurn(other.currentTurn),
          hashKey(other.hashKey),
          cells(other.cells),
          undoStack(std::move(other.undoStack)),
          zobrist(other.zobrist) {
        other.cells       = nullptr;
        other.boardWidth  = 0;
        other.boardHeight = 0;
    }

    // Move assignment operator
    Board& operator=(Board&& other) noexcept {
        if (this != &other) {
            delete[] cells;

            boardWidth        = other.boardWidth;
            boardHeight       = other.boardHeight;
            targetWinLength   = other.targetWinLength;
            currentTurn       = other.currentTurn;
            hashKey           = other.hashKey;
            cells             = other.cells;
            undoStack         = std::move(other.undoStack);
            zobrist           = other.zobrist;

            other.cells       = nullptr;
            other.boardWidth  = 0;
            other.boardHeight = 0;
        }
        return *this;
    }

    void initialize(int width, int height, int winLength) {
        delete[] cells;
        boardWidth      = width;
        boardHeight     = height;
        targetWinLength = winLength;
        currentTurn     = Piece::PlayerX;

        int totalCells = boardWidth * boardHeight;
        cells = new Piece[totalCells];
        for (int i = 0; i < totalCells; ++i) {
            cells[i] = Piece::Empty;
        }

        undoStack.clear();
        undoStack.reserve(totalCells);

        // Hash side key only; empty cells are NOT hashed
        hashKey = zobrist.getSideKey();
    }

    inline bool inBounds(int x, int y) const {
        return x >= 0 && x < boardWidth && y >= 0 && y < boardHeight;
    }

    // Fast unsafe lookup for performance critical inner loops
    inline Piece pieceAtUnsafe(int x, int y) const {
        return cells[getIndex(x, y)];
    }

    // Safe lookup with bounds check
    inline Piece pieceAt(int x, int y) const {
        if (!inBounds(x, y)) {
            return Piece::Empty;
        }
        return cells[getIndex(x, y)];
    }

    bool makeMove(int x, int y) {
        if (!inBounds(x, y) || pieceAtUnsafe(x, y) != Piece::Empty) {
            return false;
        }

        // Push state to undo stack
        undoStack.push_back(UndoInfo{
            Point{x, y},
            hashKey,
            currentTurn
        });

        int index = getIndex(x, y);
        cells[index] = currentTurn;

        // Apply hash for active piece only
        hashKey ^= zobrist.getPieceKey(x, y, currentTurn);
        currentTurn = getOppositePiece(currentTurn);
        hashKey ^= zobrist.getSideKey();

        return true;
    }

    void undoMove() {
        if (undoStack.empty()) {
            return;
        }

        UndoInfo info = undoStack.back();
        undoStack.pop_back();

        int index = getIndex(info.point.x, info.point.y);
        cells[index] = Piece::Empty;

        // Restore prior state
        hashKey     = info.hashKey;
        currentTurn = info.side;
    }

    bool isWinAt(int lastX, int lastY) const {
        Piece player = pieceAtUnsafe(lastX, lastY);
        if (player == Piece::Empty) {
            return false;
        }

        const int directionsX[4] = {1, 0, 1,  1};
        const int directionsY[4] = {0, 1, 1, -1};

        for (int i = 0; i < 4; ++i) {
            int dx = directionsX[i];
            int dy = directionsY[i];

            int positiveRay = countRay(lastX, lastY,  dx,  dy, player);
            int negativeRay = countRay(lastX, lastY, -dx, -dy, player);

            int totalSequence = 1 + positiveRay + negativeRay;
            if (totalSequence >= targetWinLength) {
                return true;
            }
        }

        return false;
    }

    bool hasNeighbor(int x, int y, int radius) const {
        int startX = std::max(0, x - radius);
        int endX   = std::min(boardWidth  - 1, x + radius);
        int startY = std::max(0, y - radius);
        int endY   = std::min(boardHeight - 1, y + radius);

        for (int nx = startX; nx <= endX; ++nx) {
            for (int ny = startY; ny <= endY; ++ny) {
                if (nx == x && ny == y) continue;
                if (pieceAtUnsafe(nx, ny) != Piece::Empty) {
                    return true;
                }
            }
        }
        return false;
    }

    bool isBoardFull() const {
        return static_cast<int>(undoStack.size()) == (boardWidth * boardHeight);
    }

    U64  getHash()        const { return hashKey; }
    Piece getCurrentTurn() const { return currentTurn; }
    int  getWidth()       const { return boardWidth; }
    int  getHeight()      const { return boardHeight; }
    int  getWinLength()   const { return targetWinLength; }
    int  getMoveCount()   const { return static_cast<int>(undoStack.size()); }

    const std::vector<UndoInfo>& getUndoStack() const { return undoStack; }
};
