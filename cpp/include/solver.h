// solver.h
/* Solver class, contains our CHESS AI featuring 3 levels of difficulty */

#ifndef solver_h
#define solver_h

#include "turn.h"
#include "board.h"
#include "piece.h"
#include "pawn.h"
#include "queen.h"
#include "globals.h"
#include <chrono>
#include <unordered_map>
#include <vector>
#include <random>

class Solver {
private:
    // Instance variables
    int difficulty;

    // Evaluation Constants (Tune these!)
    static const int LEVEL_CONTROL_WEIGHT = 5;
    static const int SPACE_CONTROL_WEIGHT = 2;
    static const int OUTPOST_BONUS = 10;
    static const int PIECE_COORDINATION_BONUS = 8;
    static const int KING_EXPOSED_PENALTY = 12;
    static const int THREAT_WEIGHT = 7;

    // Search Parameters (Tune these!)
    static const int MAX_QUIESCENCE_DEPTH = 3;
    static const int ASPIRATION_WINDOW = 50;
    static const int NULL_MOVE_REDUCTION = 2;
    static const int LATE_MOVE_REDUCTION = 1;

    // Instance methods
    Turn solve(Board &board, int depth, int ALPHA, int BETA, int color, int score);
    int distance(Coordinate coord);
    int pieceScore(Piece *piece);
    bool canPromote(Piece* piece);
    int mobilityScore(Board &board, Piece *piece);
    int kingSafetyScore(Board &board, int color);
    int evaluateLevelControl(Board &board, int color);
    int evaluateSpaceControl(Board &board, int color);
    int evaluateOutposts(Board &board, int color);
    int evaluatePieceCoordination(Board &board, int color);
    int evaluateThreats(Board &board, int color);

    int quiescenceSearch(Board &board, int ALPHA, int BETA, int color, int depth, int score);
    bool isEndgame(Board &board);

    // Static variables
    static std::unordered_map<char, int> pieceWeight;
    static int pieceSquareTable[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE];

    // Random number generator
    static std::random_device m_rd;
    static std::mt19937 m_rng;
    static std::uniform_int_distribution<int> rng;

public:
    static const int INF = 1e7;

    // Constructor
    Solver(int);

    // Useful utility methods
    int evaluate(Board &board);
    Turn nextMove(Board &board, int color);
    std::vector<Turn> genMoves(Board &board, int color);
    static int randRange(int low, int high);
};

#endif
