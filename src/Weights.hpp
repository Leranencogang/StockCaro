#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

struct EngineWeights {
    int openFour = 50000;
    int blockedFour = 10000;
    int openThree = 5000;
    int blockedThree = 1000;
    int openTwo = 500;
    int blockedTwo = 100;
    int singlePiece = 10;

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string key;
            int val;
            if (ss >> key >> val) {
                if (key == "openFour") openFour = val;
                else if (key == "blockedFour") blockedFour = val;
                else if (key == "openThree") openThree = val;
                else if (key == "blockedThree") blockedThree = val;
                else if (key == "openTwo") openTwo = val;
                else if (key == "blockedTwo") blockedTwo = val;
                else if (key == "singlePiece") singlePiece = val;
            }
        }
    }

    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        file << "openFour " << openFour << "\n";
        file << "blockedFour " << blockedFour << "\n";
        file << "openThree " << openThree << "\n";
        file << "blockedThree " << blockedThree << "\n";
        file << "openTwo " << openTwo << "\n";
        file << "blockedTwo " << blockedTwo << "\n";
        file << "singlePiece " << singlePiece << "\n";
    }
};
