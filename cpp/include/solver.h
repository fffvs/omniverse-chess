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

class Solver {
    private:
        // Instance variables
        int difficulty;
        // Instance methods
        Turn solve(Board &board, int depth, int ALPHA, int BETA, int color, int score);
        int distance(Coordinate coord);
        int pieceScore(Piece *piece);
        bool canPromote(Piece* piece);
        int mobilityScore(Board &board, Piece *piece);
        int kingSafetyScore(Board &board, int color);
        int quiescenceSearch(Board &board, int ALPHA, int BETA, int color, int score);
        // Static variables
        static unordered_map<char, int> pieceWeight;
        static int pieceSquareTable[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE];
        // Random number generator
        static random_device m_rd;
        static mt19937 m_rng;
        static uniform_int_distribution<int> rng;
    public:
        static const int INF = 1e7;
        // Constructor
        Solver(int);
        // Useful utility methods
        int evaluate(Board &board);
        Turn nextMove(Board &board, int color);
        vector<Turn> genMoves(Board &board, int color);
        static int randRange(int low, int high);
};

#endif
