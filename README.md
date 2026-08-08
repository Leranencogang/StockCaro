

<img width="1280" height="640" alt="converted_image" src="https://github.com/user-attachments/assets/b164a70c-68b1-4e7b-8592-30a36dfbc25a" />

# 🎮 Stockcaro AI Gomoku

Stockcaro is a high-performance Gomoku (Caro) AI engine written in C++17, wrapped in a lightweight Node.js Express server, and styled with a sleek glassmorphic HTML5 frontend. It features an advanced alpha-beta search with PVS (Principal Variation Search), transposition tables, iterative deepening, dynamic move-list pruning, a dynamic Elo rating progression profile, and configurable skill tiers.

---

## 📁 Project Architecture & Components

* **`src/`**: Core C++17 AI Engine code. Handles move generation, pruning, PVS search, transposition tables, history tables, and evaluation metrics.
* **`public/`**: Responsive web UI containing 3D glossy game pieces, a live evaluation bar, difficulty controls, and a persistent local Elo profile dashboard.
* **`server.js`**: Node.js Express server acting as a bridge communicating JSON payloads from the web client to the C++ engine executable.
* **`setup.sh`**: Automated shell script that resolves compiler packages, fetches Node, builds the optimized binary, runs unit benchmarks, and starts the server.
* **`Makefile`**: Compilation commands targeting native performance using GCC `-O3` flags.
* **`weights.txt`**: Hyperparameters defining positional rewards and threat patterns.

---

## ⚡ Setup & Installation

### 🚀 Automatic Launch (Recommended)
Compile the C++ engine and launch the Node.js server automatically using the setup script:
```bash
bash setup.sh
```
Once completed, open your browser and navigate to: **[http://localhost:3000](http://localhost:3000)**

### 🛠️ Manual Compilation
For developers desiring custom build actions:
1. **Compile C++ Engine:**
   ```bash
   make clean && make
   ```
   *Compiles source code with `-O3 -march=native` parameters for optimized vector operations.*
2. **Start Web Server:**
   ```bash
   node server.js
   ```
   *Listens for client connections on Port 3000.*

---

## 🧠 AI Difficulty & Skill Tiers

Stockcaro supports five difficulty settings, adjusting search depth and time budgets sent directly to the C++ engine:

| Skill | C++ Depth | Time Limit | Estimated Elo Rating |
|---|---|---|---|
| **Easy** | 2 plies | 50 ms | `1000` (Fast tactical checks, suitable for absolute beginners) |
| **Medium** | 4 plies | 150 ms | `1400` (Basic lookahead, defends single-step threat lines) |
| **Hard** | 6 plies | 450 ms | `1800` (Challenging level, constructs simple split attacks) |
| **Expert** | 9 plies | 1200 ms | `2200` (Advanced tactical foresight, prevents double-threes) |
| **Master** | 14 plies | 3000 ms | `2500` (Grandmaster level, deep transposition exploration) |

---

## 📊 Live User Elo Profile System

The web interface features a persistent local Profile Panel. Your results are recorded directly inside your browser:
* **Starting Rating:** `1200` (Standard baseline).
* **Rank Tiers:** Progression moves from *Novice*, *Apprentice*, *Intermediate*, *Expert*, *Master*, *Grandmaster*, *Super Grandmaster*, and finally *Stockcaro Slayer*.
* **Formula:** Elo adjustments are calculated using standard rating change mathematics:
  
  $$\text{Expected Score } (E_A) = \frac{1}{1 + 10^{(R_{AI} - R_{\text{user}}) / 400}}$$
  
  $$R_{\text{new}} = R_{\text{old}} + K \times (\text{Outcome} - E_A)$$
  
  *(where $K = 32$, Outcome is $1.0$ for Win, $0.0$ for Loss, and $0.5$ for Draw).*

---

## ⚙️ Search Engine & Algorithmic Optimizations

Stockcaro implements highly optimized game-playing algorithms to achieve massive search depths quickly:

### 1. Principal Variation Search (PVS)
PVS is a high-performance variant of Alpha-Beta minimax. It assumes that the first move ordered is likely the best move (the principal variation). It searches the first move with a full window $(\alpha, \beta)$, and subsequent moves are searched with a null window $(\alpha, \alpha+1)$. A full re-search is only triggered if a late move refutes the PV.

### 2. Dynamic Search Breadth (Width Pruning)
To prevent exponential branch explosion at deep plies, Stockcaro dynamically adjusts its search breadth factor:
```cpp
int movesToSearch = std::min(moveList.size(), depth >= 10 ? 8 : (depth >= 6 ? 12 : 18));
```
* **Deep Plies (depth >= 10):** Focuses search on the top 8 tactical candidates.
* **Mid-game Plies (depth >= 6):** Expands focus to 12 candidates.
* **Tactical Plies (depth < 6):** Scans up to 18 candidates to avoid horizon blunders.

### 3. Zobrist Hashing & Transposition Table
Board states are cached using 64-bit Zobrist hashes. The hash is updated incrementally via XOR operations during make/undo cycles. The Transposition Table (TT) stores:
* Search scores, evaluation depths, flags (`Exact`, `LowerBound`, `UpperBound`), and best moves for move ordering.

### 4. Move Ordering Heuristics
Moves are sorted to maximize alpha-beta cutoffs using:
* **Killer Move Heuristic:** Prioritizes quiet moves that caused beta cutoffs at the same depth.
* **History Heuristic:** Scores moves globally based on their success in causing cutoffs.
* **Neighborhood Radial Check:** Restricts move generation to cells within 2 units of existing pieces.

---

## 📈 Self-Play Training (Reinforcement Learning)

Stockcaro contains a built-in self-play training loop that optimizes the evaluation weights:
```bash
./Stockcaro --train 2>&1 | tee training.log
```
* **Mechanism:** The candidate model mutates a parameter in `weights.txt` by $\pm 15\%$ and plays 8 rapid matches against the current best model. If the candidate defeats the best model, it becomes the new baseline.
* **Evaluation Metrics Optimized:**
  * Open/Blocked Fours
  * Open/Blocked Threes
  * Open/Blocked Twos
  * Single piece weights.

---

> [!TIP]
> **Performance Tip:** Ensure your CPU supports standard vector extensions when compiling manually. GCC flags `-march=native` enable AVX/AVX2 instruction sets automatically, providing a significant boost to engine speed (Nodes Per Second).

> [!NOTE]
> All user Elo profile records and preferences are saved locally inside your browser's `localStorage` and will persist across browser restarts. Use the "Reset Profile" button to start a fresh campaign.
