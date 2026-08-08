#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <algorithm>

#include "CoreTypes.hpp"
#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "SearchTables.hpp"
#include "PatternEvaluator.hpp"
#include "SearchEngine.hpp"

struct ParsedMove {
    int x;
    int y;
    Piece player;
};

struct JsonInput {
    int width;
    int height;
    int win;
    Piece engineSide;
    std::vector<ParsedMove> moves;
    int depth;
    int timeLimit;

    JsonInput() : width(19), height(19), win(5), engineSide(Piece::PlayerX), depth(20), timeLimit(2000) {}
};

class JsonParser {
private:
    static std::string extractValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        int pos = static_cast<int>(json.find(searchKey));
        if (pos == -1) return "";

        int colonPos = static_cast<int>(json.find(":", pos));
        if (colonPos == -1) return "";

        int start = colonPos + 1;
        while (start < static_cast<int>(json.size()) &&
               (json[start] == ' ' || json[start] == '\"' || json[start] == '\t' || json[start] == '\n' || json[start] == '\r')) {
            start++;
        }

        int end = start;
        while (end < static_cast<int>(json.size())) {
            char c = json[end];
            if (c == ',' || c == '}' || c == ']' || c == '\"' || c == ' ' || c == '\n' || c == '\r') {
                break;
            }
            end++;
        }

        return json.substr(start, end - start);
    }

public:
    static JsonInput parse(const std::string& json) {
        JsonInput input;

        std::string widthStr  = extractValue(json, "width");
        std::string heightStr = extractValue(json, "height");
        std::string winStr    = extractValue(json, "win");
        std::string engineStr = extractValue(json, "engine");
        std::string depthStr  = extractValue(json, "depth");
        std::string limitStr  = extractValue(json, "timeLimit");

        if (!widthStr.empty())  { std::stringstream ss(widthStr);  ss >> input.width;  }
        if (!heightStr.empty()) { std::stringstream ss(heightStr); ss >> input.height; }
        if (!winStr.empty())    { std::stringstream ss(winStr);    ss >> input.win;    }
        if (!depthStr.empty())  { std::stringstream ss(depthStr);  ss >> input.depth;  }
        if (!limitStr.empty())  { std::stringstream ss(limitStr);  ss >> input.timeLimit; }

        if (engineStr == "O" || engineStr == "o" || engineStr == "2") {
            input.engineSide = Piece::PlayerO;
        } else {
            input.engineSide = Piece::PlayerX;
        }

        int movesPos = static_cast<int>(json.find("\"moves\""));
        if (movesPos != -1) {
            int arrayStart = static_cast<int>(json.find("[", movesPos));
            int arrayEnd   = static_cast<int>(json.find("]", movesPos));

            if (arrayStart != -1 && arrayEnd != -1 && arrayEnd > arrayStart) {
                std::string movesJson = json.substr(arrayStart, arrayEnd - arrayStart + 1);

                int moveCount = 0;
                int searchIdx = 0;
                while (true) {
                    int foundIdx = static_cast<int>(movesJson.find("{", searchIdx));
                    if (foundIdx == -1) break;
                    moveCount++;
                    searchIdx = foundIdx + 1;
                }

                input.moves.resize(moveCount);
                int moveIdx = 0;
                searchIdx = 0;

                while (moveIdx < moveCount) {
                    int objStart = static_cast<int>(movesJson.find("{", searchIdx));
                    int objEnd   = static_cast<int>(movesJson.find("}", objStart));
                    if (objStart == -1 || objEnd == -1) break;

                    std::string objStr   = movesJson.substr(objStart, objEnd - objStart + 1);

                    std::string xStr      = extractValue(objStr, "x");
                    std::string yStr      = extractValue(objStr, "y");
                    std::string playerStr = extractValue(objStr, "player");

                    int xVal = 0, yVal = 0;
                    std::stringstream ssX(xStr); ssX >> xVal;
                    std::stringstream ssY(yStr); ssY >> yVal;

                    Piece pVal = Piece::PlayerX;
                    if (playerStr == "O" || playerStr == "o" || playerStr == "2") {
                        pVal = Piece::PlayerO;
                    }

                    input.moves[moveIdx] = ParsedMove{xVal, yVal, pVal};
                    moveIdx++;
                    searchIdx = objEnd + 1;
                }
            }
        }

        return input;
    }

    static std::string formatOutput(int bestX, int bestY, int score, int depth) {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"bestMove\":{\"x\":" << bestX << ",\"y\":" << bestY << "},\n";
        ss << "  \"score\":"           << score << ",\n";
        ss << "  \"depth\":"           << depth << "\n";
        ss << "}";
        return ss.str();
    }
};

inline void runBenchmark() {
    std::cout << "STOCKCARO ENGINE BENCHMARK" << std::endl;

    Board board;
    board.initialize(19, 19, 5);

    // Setup a tactical Gomoku mid-game position
    board.makeMove(9, 9);   // X
    board.makeMove(9, 10);  // O
    board.makeMove(10, 9);  // X
    board.makeMove(8, 10);  // O
    board.makeMove(10, 10); // X
    board.makeMove(11, 11); // O
    board.makeMove(8, 8);   // X
    board.makeMove(7, 7);   // O

    SearchEngine engine;
    engine.setTTSize(128);

    int targetDepth = 12;
    int timeLimit   = 3000;

    std::cout << "Board Size: 19x19"                  << std::endl;
    std::cout << "Target Depth: "  << targetDepth      << std::endl;
    std::cout << "Time Limit: "    << timeLimit << " ms" << std::endl;
    std::cout << "Running search..." << std::endl;

    auto startTime = std::chrono::steady_clock::now();
    SearchResult result = engine.startSearch(board, targetDepth, timeLimit);
    auto endTime   = std::chrono::steady_clock::now();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    if (elapsedMs == 0) elapsedMs = 1;

    U64 nps = (result.nodes * 1000ULL) / static_cast<U64>(elapsedMs);


    std::cout << "Best Move: (" << result.bestMove.x << ", " << result.bestMove.y << ")" << std::endl;
    std::cout << "Score: "          << result.score  << std::endl;
    std::cout << "Depth Reached: "  << result.depth  << std::endl;
    std::cout << "Nodes Visited: "  << result.nodes  << std::endl;
    std::cout << "Time Elapsed: "   << elapsedMs << " ms" << std::endl;
    std::cout << "NPS (Nodes/Sec): " << nps       << std::endl;

}

void runSelfPlayTraining() {
    std::cout << "STOCKCARO AI SELF-PLAY TRAINING" << std::endl;

    EngineWeights bestWeights;
    bestWeights.loadFromFile("weights.txt");
    bestWeights.saveToFile("weights.txt"); // Ensure it exists

    std::cout << "Initial Weights Loaded:" << std::endl;
    std::cout << "  OpenFour:     " << bestWeights.openFour << std::endl;
    std::cout << "  BlockedFour:  " << bestWeights.blockedFour << std::endl;
    std::cout << "  OpenThree:    " << bestWeights.openThree << std::endl;
    std::cout << "  BlockedThree: " << bestWeights.blockedThree << std::endl;
    std::cout << "  OpenTwo:      " << bestWeights.openTwo << std::endl;
    std::cout << "  BlockedTwo:   " << bestWeights.blockedTwo << std::endl;
    std::cout << "  SinglePiece:  " << bestWeights.singlePiece << std::endl;

    // Simple LCG random
    uint64_t rngState = 0xDEADBEEFCAFEBABEULL;
    auto randomInRange = [&](int minVal, int maxVal) -> int {
        rngState = rngState * 6364136223846793005ULL + 1442695040888963407ULL;
        return minVal + (rngState % (maxVal - minVal + 1));
    };

    auto mutate = [&](const EngineWeights& src) -> EngineWeights {
        EngineWeights dest = src;
        int param = randomInRange(0, 6);
        double factor = 0.85 + (randomInRange(0, 30) / 100.0); // Mutate parameter by +/- 15%
        
        if      (param == 0) dest.openFour     = std::max(1000, (int)(dest.openFour * factor));
        else if (param == 1) dest.blockedFour  = std::max(500,  (int)(dest.blockedFour * factor));
        else if (param == 2) dest.openThree    = std::max(100,  (int)(dest.openThree * factor));
        else if (param == 3) dest.blockedThree = std::max(50,   (int)(dest.blockedThree * factor));
        else if (param == 4) dest.openTwo      = std::max(10,   (int)(dest.openTwo * factor));
        else if (param == 5) dest.blockedTwo   = std::max(5,    (int)(dest.blockedTwo * factor));
        else if (param == 6) dest.singlePiece  = std::max(1,    (int)(dest.singlePiece * factor));
        return dest;
    };

    int generation = 1;
    int totalGames = 0;
    while (true) {
        std::cout << "\n[Generation " << generation << "]" << std::endl;
        EngineWeights candidateWeights = mutate(bestWeights);

        std::cout << "Evaluating Candidate Weights..." << std::endl;

        int candidateWins = 0;
        int bestWins = 0;

        // Play 8 games (4 where Candidate is X, 4 where Best is X)
        for (int gameIdx = 0; gameIdx < 8; ++gameIdx) {
            totalGames++;
            std::cout << "self played : " << totalGames << std::endl;
            Board board;
            board.initialize(15, 15, 5); // Train on 15x15 board to make it fast

            SearchEngine candidateEngine;
            candidateEngine.setEvaluatorWeights(candidateWeights);
            candidateEngine.setTTSize(16);

            SearchEngine bestEngine;
            bestEngine.setEvaluatorWeights(bestWeights);
            bestEngine.setTTSize(16);

            bool candidateIsX = (gameIdx % 2 == 0);

            // Play game loop
            while (!board.isBoardFull()) {
                Piece turn = board.getCurrentTurn();
                bool activeIsCandidate = (turn == Piece::PlayerX && candidateIsX) || 
                                         (turn == Piece::PlayerO && !candidateIsX);

                SearchResult res;
                if (activeIsCandidate) {
                    res = candidateEngine.startSearch(board, 6, 80); // Quick search (depth 6, 80ms)
                } else {
                    res = bestEngine.startSearch(board, 6, 80);
                }

                if (res.bestMove.x == -1 || !board.makeMove(res.bestMove.x, res.bestMove.y)) {
                    break; // Draw or illegal move
                }

                if (board.isWinAt(res.bestMove.x, res.bestMove.y)) {
                    if (activeIsCandidate) candidateWins++;
                    else bestWins++;
                    break;
                }
            }
        }

        std::cout << "Results -> Candidate Wins: " << candidateWins 
                  << " | Best Wins: " << bestWins << std::endl;

        if (candidateWins > bestWins) {
            std::cout << "NEW BEST WEIGHTS FOUND! Saving..." << std::endl;
            bestWeights = candidateWeights;
            bestWeights.saveToFile("weights.txt");
            std::cout << "  OpenFour:    " << bestWeights.openFour << std::endl;
            std::cout << "  OpenThree:   " << bestWeights.openThree << std::endl;
        } else {
            std::cout << "Candidate was weaker. Reverting." << std::endl;
        }
        generation++;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--bench" || arg1 == "bench" || arg1 == "-b") {
            runBenchmark();
            return 0;
        }
        if (arg1 == "--train" || arg1 == "train" || arg1 == "-t") {
            runSelfPlayTraining();
            return 0;
        }
    }

    std::string jsonContent;
    std::string line;
    while (std::getline(std::cin, line)) {
        jsonContent += line + "\n";
    }

    if (jsonContent.empty()) {
        runBenchmark();
        return 0;
    }

    JsonInput input = JsonParser::parse(jsonContent);

    Board board;
    board.initialize(input.width, input.height, input.win);

    for (int i = 0; i < static_cast<int>(input.moves.size()); ++i) {
        board.makeMove(input.moves[i].x, input.moves[i].y);
    }

    SearchEngine engine;
    engine.setTTSize(64);

    SearchResult result = engine.startSearch(board, input.depth, input.timeLimit);

    std::string outputJson = JsonParser::formatOutput(
        result.bestMove.x,
        result.bestMove.y,
        result.score,
        result.depth
    );

    std::cout << outputJson << std::endl;

    return 0;
}
