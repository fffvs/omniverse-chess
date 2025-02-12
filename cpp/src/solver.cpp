// solver.cpp
#include "../include/solver.h"

#include <chrono>  // Required for timing

// Setting up the mersenne twister random number generator for better random number generation
random_device Solver::m_rd;
mt19937       Solver::m_rng(Solver::m_rd());
uniform_int_distribution<int> Solver::rng(0, INF);

// Map each character to a specific integer weight (higher = more important)
unordered_map<char, int> Solver::pieceWeight = {
    {'b', 40}, {'k', 100}, {'n', 40}, {'p', 30}, {'q', 60}, {'r', 50}, {'u', 40}
};

// Initialize piece-square table
int Solver::pieceSquareTable[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE] = {
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}},
    {{5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}},
    {{1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}},
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}},
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}}
};



// Parameterized constructor
Solver::Solver(int difficulty_) {
    difficulty = difficulty_;
}

// Utility function to generate a random integer in the range [low, high] inclusive
int Solver::randRange(int low, int high){
    int range = high - low + 1;
    assert(range >= 0);
    return low + rng(m_rng) % range;
}

// Distance function of a piece to the center of the board (the center is good for control)
int Solver::distance(Coordinate coord){
    // Prioritize level, then row, and column last
    return -(abs(coord.row - 2) * 3 + abs(coord.col - 2) + abs(coord.lvl - 2) * 5);
}

// Helper function to evaluate the usefulness of a piece on the board
int Solver::pieceScore(Piece *piece){
    // A piece's score is defined as their weight + their distance to the center
    // If the piece is dead ignore it
    if(!piece->getIsAlive()) return 0;
    // If the piece is a king then it should stay away from the middle
    if(piece->getId() == 'k'){
        return (pieceWeight[piece->getId()] - distance(piece->getLocation())) * piece->getColor();
    }
    return (pieceWeight[piece->getId()] + distance(piece->getLocation())) * piece->getColor();
}

// Utility function to determine whether a pawn can be promoted
bool Solver::canPromote(Piece* piece){
    return piece->getId() == 'p' &&
    ((piece->getColor() == WHITE && piece->getLocation().row + piece->getLocation().lvl == 8) ||
     (piece->getColor() == BLACK && piece->getLocation().row + piece->getLocation().lvl == 0));
}

int Solver::mobilityScore(Board &board, Piece *piece) {
    if (!piece->getIsAlive()) return 0;
    return piece->getMoves(board, false).size(); // Number of possible moves

}

int Solver::kingSafetyScore(Board &board, int color) {
    // Implement a function to evaluate king safety
    // Consider:
    // - Number of attackers near the king
    // - Presence of defenders
    // - Open files/diagonals towards the king
    // - King's location (center vs. corner)
    Coordinate kingLocation = board.getKingLocation(color);
    int score = 0;
    // Example:  Penalize if the king is in the center
    score -= (abs(kingLocation.row - 2) + abs(kingLocation.col - 2) + abs(kingLocation.lvl - 2));
    return score;
}

// Utility function to evaluate the current board entirely
int Solver::evaluate(Board &board){
    int score = 0;
    for(int row = 0; row < BOARD_SIZE; ++row){
        for(int col = 0; col < BOARD_SIZE; ++col){
            for(int lvl = 0; lvl < BOARD_SIZE; ++lvl){
                Piece* piece = board.getPieceAt(row, col, lvl);
                if(piece->getId() != ' ') {
                    score += pieceScore(piece);
                    score += pieceSquareTable[row][col][lvl] * piece->getColor();
                    score += mobilityScore(board, piece) * piece->getColor();
                }
            }
        }
    }
    score += kingSafetyScore(board, WHITE) - kingSafetyScore(board, BLACK);
    return score;
}

// Utility function to determine all the possible moves the current color can play
vector<Turn> Solver::genMoves(Board &board, int color){
    vector<Turn> moves;
    for(int i = 0; i < BOARD_SIZE; ++i) {
        for(int j = 0; j < BOARD_SIZE; ++j) {
            for(int k = 0; k < BOARD_SIZE; ++k) {
                // Must be piece of the same color and must be alive
                if(board.board[i][j][k]->getColor() == color && board.board[i][j][k]->getIsAlive()){
                    // Go through all the moves, make sure you don't leave your king checked
                    for (Move m : board.board[i][j][k]->getMoves(board, false)) {
                        Piece* oldPiece = board.board[i + m.row][j + m.col][k + m.lvl];
                        int newScore = -pieceScore(board.board[i][j][k]) - pieceScore(oldPiece);
                        board.updateLocation({i, j, k}, m);
                        if(!board.isChecked(color)) {
                            // Valid move
                            moves.push_back(Turn(newScore + pieceScore(board.board[i + m.row][j + m.col][k + m.lvl]), Coordinate(i, j, k), m));
                        }
                        // Undo the move on the board
                        board.updateLocation({i + m.row, j + m.col, k + m.lvl}, -m);
                        if (oldPiece->getId() != ' ') {
                            board.board[i + m.row][j + m.col][k + m.lvl] = oldPiece;
                            oldPiece->setIsAlive(true);
                        }
                    }
                }
            }
        }
    }
    // Sort moves in decreasing score (Start with the more promising moves first) for better alpha-beta pruning effect
    if(color == WHITE){
       sort(moves.begin(), moves.end(), [&](const Turn &lhs, const Turn &rhs){
          // 1. Prioritize captures (highest score if capturing a valuable piece)
          Coordinate newLocLhs = lhs.currentLocation + lhs.change;
          Coordinate newLocRhs = rhs.currentLocation + rhs.change;

          int lhsCaptureScore = Solver::pieceWeight[board.getPieceAt(newLocLhs)->getId()];
          int rhsCaptureScore = Solver::pieceWeight[board.getPieceAt(newLocRhs)->getId()];

          if(lhsCaptureScore > 0 || rhsCaptureScore > 0){
            return lhsCaptureScore > rhsCaptureScore;
          }

          // 2. Then prioritize moves that improve piece position (higher score)
          return lhs.score > rhs.score;
        });
    } else {
        sort(moves.begin(), moves.end(), [&](const Turn &lhs, const Turn &rhs){
          Coordinate newLocLhs = lhs.currentLocation + lhs.change;
          Coordinate newLocRhs = rhs.currentLocation + rhs.change;

          int lhsCaptureScore = Solver::pieceWeight[board.getPieceAt(newLocLhs)->getId()];
          int rhsCaptureScore = Solver::pieceWeight[board.getPieceAt(newLocRhs)->getId()];

          if(lhsCaptureScore > 0 || rhsCaptureScore > 0){
            return lhsCaptureScore < rhsCaptureScore;
          }
          return lhs.score < rhs.score;
        });
    }
    return moves;
}

int Solver::quiescenceSearch(Board &board, int ALPHA, int BETA, int color, int score) {
    // Stand pat (assume no further captures are good)
    int standPat = score;
    if (color == WHITE) {
        if (standPat >= BETA)
            return BETA;
        ALPHA = max(ALPHA, standPat);
    } else {
        if (standPat <= ALPHA)
            return ALPHA;
        BETA = min(BETA, standPat);
    }

    // Generate only capture moves
    vector<Turn> captureMoves;
    for(Turn move : genMoves(board, color)){
        Coordinate newLoc = move.currentLocation + move.change;
        if(board.getPieceAt(newLoc)->getId() != ' '){
            captureMoves.push_back(move);
        }
    }


    for (Turn curMove : captureMoves) {
        // Move new piece
        Coordinate newLoc = curMove.currentLocation + curMove.change;
        Piece *pawn, *oldPiece = board.getPieceAt(newLoc);
        // Update score as well
        int newScore = score + curMove.score;
        board.updateLocation(curMove.currentLocation, curMove.change);
        bool promoted = canPromote(board.getPieceAt(newLoc));
        if(promoted){
            // If the pawn can promote, then change the piece to a queen (the best option)
            pawn = board.getPieceAt(newLoc);
            // Update scores again
            newScore -= pieceScore(board.getPieceAt(newLoc));
            ((Pawn*)pawn)->promote(board, new Queen(newLoc.row, newLoc.col, newLoc.lvl, color), false);
            newScore += pieceScore(board.getPieceAt(newLoc));
        }
        int eval = quiescenceSearch(board, ALPHA, BETA, -color, newScore);
         // Revert the move
        if(promoted){
            // Delete the pointer if the pawn was promoted
            delete board.board[newLoc.row][newLoc.col][newLoc.lvl];
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = pawn;
        }
        board.updateLocation(newLoc, -curMove.change);
        if(oldPiece->getId() != ' '){
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = oldPiece;
            oldPiece->setIsAlive(true);
        }

        if (color == WHITE) {
            ALPHA = max(ALPHA, eval);
            if (ALPHA >= BETA)
                return BETA;
        } else {
            BETA = min(BETA, eval);
            if (BETA <= ALPHA)
                return ALPHA;
        }
    }
    if (color == WHITE) return ALPHA;
    return BETA;
}

Turn Solver::solve(Board &board, int depth, int ALPHA, int BETA, int color, int score){

    // First look for checkmates, then stalemates
    if(board.isChecked(color)){
        if(board.isCheckmated(color)){
            // White Mate --> -INF, Black Mate --> INF
            return Turn(INF * -color, Coordinate(-2, -1, -1), Move(0, 0, 0));
        }
    } else if(board.isStalemated(color)){
        // Nobody wins
        return Turn(0, Coordinate(-1, -1, -1), Move(0, 0, 0));
    }

    if(depth == 0){
        // Evaluate board
        // For Hard mode, the randRange is smaller in an attempt to pick the optimal move
        return Turn(quiescenceSearch(board, ALPHA, BETA, color, score), Coordinate(-4, -1, -1), Move(0, 0, 0));
    }
    // Identify the best move on the board
    Turn best(color == WHITE ? -INF : INF, Coordinate(-3, -1, -1), Move(0, 0, 0));
    for(Turn curMove : genMoves(board, color)){
        // Move new piece
        Coordinate newLoc = curMove.currentLocation + curMove.change;
        Piece *pawn, *oldPiece = board.getPieceAt(newLoc);
        // Update score as well
        int newScore = score + curMove.score;
        board.updateLocation(curMove.currentLocation, curMove.change);
        bool promoted = canPromote(board.getPieceAt(newLoc));
        if(promoted){
            // If the pawn can promote, then change the piece to a queen (the best option)
            pawn = board.getPieceAt(newLoc);
            // Update scores again
            newScore -= pieceScore(board.getPieceAt(newLoc));
            ((Pawn*)pawn)->promote(board, new Queen(newLoc.row, newLoc.col, newLoc.lvl, color), false);
            newScore += pieceScore(board.getPieceAt(newLoc));
        }
        // Recurse to the other opponent
        Turn candidate = Turn(solve(board, depth - 1, ALPHA, BETA, -color, newScore).score, curMove.currentLocation, curMove.change);
        // Revert the move
        if(promoted){
            // Delete the pointer if the pawn was promoted
            delete board.board[newLoc.row][newLoc.col][newLoc.lvl];
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = pawn;
        }
        board.updateLocation(newLoc, -curMove.change);
        if(oldPiece->getId() != ' '){
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = oldPiece;
            oldPiece->setIsAlive(true);
        }
        // White --> Wants to maximize
        // Black --> Wants to minimize
        if(color == WHITE){
            ALPHA = max(ALPHA, candidate.score);
            // If the move is really good, then update it
            if(candidate.score >= best.score){
                best = candidate;
                // Alpha-Beta pruning
                // If this move is way better than the opponent's best move, then use it
                if(best.score >= BETA){
                    break;
                }
            }
        } else {
            BETA = min(BETA, candidate.score);
            if(candidate.score <= best.score){
                best = candidate;
                // Alpha-Beta pruning
                // If this move is way better than the opponent's best move, then use it
                if(best.score <= ALPHA){
                    break;
                }
            }
        }
    }
    // Return the best move
    return best;
}

Turn Solver::nextMove(Board &board, int color) {
    // Count the number of pieces left to determine the stage of the game (start, middle, end)
    int countPieces = 0;
    for(int i = 0; i < BOARD_SIZE; ++i)
        for(int j = 0; j < BOARD_SIZE; ++j)
            for(int k = 0; k < BOARD_SIZE; ++k)
                if(board.board[i][j][k]->getColor() == -color && board.board[i][j][k]->getIsAlive())
                    ++countPieces;
    // For the easy difficulty we arbitrarily pick a piece (while there are many pieces left on the board)
    if(difficulty == 0 && countPieces >= 10){
        // Easy mode -- randomly choose a move
        vector<char> active;
        vector<Turn> moves = genMoves(board, color);
        // Shuffle to randomize the moves
        shuffle(moves.begin(), moves.end(), m_rng);
        // Just in case someone tried to get another move after a checkmate
        if(moves.empty()){
            return Turn(0, Coordinate(-1, 0, 0), Move(0, 0, 0));
        }
        // Get all types of pieces remaining
        for(auto turn : moves){
            active.push_back(board.getPieceAt(turn.currentLocation)->getId());
        }
        // Remove duplicates (so that the computer doesn't always pick a pawn)
        sort(active.begin(), active.end());
        active.erase(unique(active.begin(), active.end()), active.end());
        // Choose a random piece
        char tgt = active[randRange(0, active.size() - 1)];
        // Find an associated move and return it
        for(auto turn : moves){
            if(board.getPieceAt(turn.currentLocation)->getId() == tgt){
                return turn;
            }
        }
        // If no moves are somehow found, return an End Game turn
        return Turn(0, Coordinate(-1, 0, 0), Move(0, 0, 0));
    }

    // Medium and Hard difficulties use iterative deepening
    int maxDepth;
    if (difficulty == 1) {
        maxDepth = 3; // Medium difficulty
    } else {
        maxDepth = 5; // Hard difficulty
    }

    Turn bestMove;
    int bestScore = (color == WHITE) ? -INF : INF;

    // Iterative Deepening
    for (int depth = 1; depth <= maxDepth; ++depth) {
        Turn currentMove = solve(board, depth, -INF, INF, color, evaluate(board));

        if ((color == WHITE && currentMove.score > bestScore) || (color == BLACK && currentMove.score < bestScore)) {
            bestMove = currentMove;
            bestScore = currentMove.score;
        }
    }

    return bestMove;
}
