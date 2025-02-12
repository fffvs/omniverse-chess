// solver.cpp
#include "../include/solver.h"

#include <chrono>  // Required for timing

// Setting up the mersenne twister random number generator for better random number generation
std::random_device Solver::m_rd;
std::mt19937       Solver::m_rng(Solver::m_rd());
std::uniform_int_distribution<int> Solver::rng(0, INF);

// Map each character to a specific integer weight (higher = more important)
std::unordered_map<char, int> Solver::pieceWeight = {
    {'b', 40}, {'k', 100}, {'n', 40}, {'p', 30}, {'q', 60}, {'r', 50}, {'u', 40}
};

// Initialize piece-square table (example)
int Solver::pieceSquareTable[BOARD_SIZE][BOARD_SIZE][BOARD_SIZE] = {
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}},
    {{5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}, {5,  5,  5,  5,  5}},
    {{1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}, {1,  1,  2,  1,  1}},
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}},
    {{0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}, {0,  0,  0,  0,  0}}
};

// Parameterized constructor
Solver::Solver(int difficulty_) : difficulty(difficulty_) {}

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
    Coordinate kingLocation = board.getKingLocation(color);
    int score = 0;
    // Penalize if the king is in the center or near open files/diagonals
    score -= (abs(kingLocation.row - 2) + abs(kingLocation.col - 2) + abs(kingLocation.lvl - 2));
    return score;
}

int Solver::evaluateLevelControl(Board &board, int color) {
    int score = 0;
    for (int lvl = 0; lvl < BOARD_SIZE; ++lvl) {
        int levelScore = 0;
        for (int row = 0; row < BOARD_SIZE; ++row) {
            for (int col = 0; col < BOARD_SIZE; ++col) {
                Piece* piece = board.getPieceAt(row, col, lvl);
                if (piece->getColor() == color) {
                    levelScore++;
                } else if (piece->getColor() == -color) {
                    levelScore--;
                }
            }
        }
        score += levelScore * (lvl == 2 ? 2 : 1); // Prioritize the center level
    }
    return score * LEVEL_CONTROL_WEIGHT;
}

int Solver::evaluateSpaceControl(Board &board, int color) {
    int score = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            for (int lvl = 0; lvl < BOARD_SIZE; ++lvl) {
                Piece* piece = board.getPieceAt(row, col, lvl);
                if (piece->getColor() == color) {
                    //Count how many empty locations the color has
                    for (Move m : piece->getMoves(board, false)) {
                        if (board.getPieceAt(piece->getLocation() + m)->getId() == ' ') score++;
                    }
                }
            }
        }
    }
    return score * SPACE_CONTROL_WEIGHT;
}

int Solver::evaluateOutposts(Board &board, int color) {
    int score = 0;
    // Implementation to find and evaluate outposts
    // This would involve checking for squares protected by pawns and difficult to attack
    return score * OUTPOST_BONUS; // Placeholder
}

int Solver::evaluatePieceCoordination(Board &board, int color) {
    int score = 0;
    // Implementation to evaluate how well pieces are coordinated
    // This might involve checking for pieces supporting each other or controlling key squares
    return score * PIECE_COORDINATION_BONUS; // Placeholder
}

int Solver::evaluateThreats(Board &board, int color) {
    int score = 0;
    // Implementation to evaluate immediate and potential threats
    // This might involve checking for attacked pieces, potential checkmates, etc.
    return score * THREAT_WEIGHT; // Placeholder
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
                }
            }
        }
    }

    score += mobilityScore(board, board.getKing(WHITE)) - mobilityScore(board, board.getKing(BLACK));
    score += kingSafetyScore(board, WHITE) - kingSafetyScore(board, BLACK);
    score += evaluateLevelControl(board, WHITE) - evaluateLevelControl(board, BLACK);
    score += evaluateSpaceControl(board, WHITE) - evaluateSpaceControl(board, BLACK);
    score += evaluateOutposts(board, WHITE) - evaluateOutposts(board, BLACK);
    score += evaluatePieceCoordination(board, WHITE) - evaluatePieceCoordination(board, BLACK);
    score += evaluateThreats(board, WHITE) - evaluateThreats(board, BLACK);

    return score;
}

// Utility function to determine all the possible moves the current color can play
std::vector<Turn> Solver::genMoves(Board &board, int color){
    std::vector<Turn> moves;
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

bool Solver::isEndgame(Board &board) {
    // Simple check for the endgame:  few pieces remaining
    int pieceCount = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            for (int lvl = 0; lvl < BOARD_SIZE; ++lvl) {
                if (board.getPieceAt(row, col, lvl)->getId() != ' ' && board.getPieceAt(row, col, lvl)->getId() != 'k') {
                    pieceCount++;
                }
            }
        }
    }
    return pieceCount <= 8;  //Adjust this threshold
}

int Solver::quiescenceSearch(Board &board, int ALPHA, int BETA, int color, int depth, int score) {
    // Stand pat (assume no further captures are good)
    int standPat = score;
    if (color == WHITE) {
        if (standPat >= BETA)
            return BETA;
        ALPHA = std::max(ALPHA, standPat);
    } else {
        if (standPat <= ALPHA)
            return ALPHA;
        BETA = std::min(BETA, standPat);
    }

    if (depth == 0) return standPat;

    // Generate only capture moves
    std::vector<Turn> captureMoves;
    for (Turn move : genMoves(board, color)) {
        Coordinate newLoc = move.currentLocation + move.change;
        if (board.getPieceAt(newLoc)->getId() != ' ') {
            captureMoves.push_back(move);
        }
    }

    for (Turn curMove : captureMoves) {
        // Move new piece
        Coordinate newLoc = curMove.currentLocation + curMove.change;
        Piece* pawn, * oldPiece = board.getPieceAt(newLoc);

        // Update score as well
        int newScore = score + curMove.score;
        board.updateLocation(curMove.currentLocation, curMove.change);
        bool promoted = canPromote(board.getPieceAt(newLoc));
        if (promoted) {
            // If the pawn can promote, then change the piece to a queen (the best option)
            pawn = board.getPieceAt(newLoc);
            // Update scores again
            newScore -= pieceScore(board.getPieceAt(newLoc));
            ((Pawn*)pawn)->promote(board, new Queen(newLoc.row, newLoc.col, newLoc.lvl, color), false);
            newScore += pieceScore(board.getPieceAt(newLoc));
        }

        int eval = quiescenceSearch(board, ALPHA, BETA, -color, depth - 1, newScore);

        // Revert the move
        if (promoted) {
            // Delete the pointer if the pawn was promoted
            delete board.board[newLoc.row][newLoc.col][newLoc.lvl];
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = pawn;
        }

        board.updateLocation(newLoc, -curMove.change);
        if (oldPiece->getId() != ' ') {
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = oldPiece;
            oldPiece->setIsAlive(true);
        }

        if (color == WHITE) {
            ALPHA = std::max(ALPHA, eval);
            if (ALPHA >= BETA)
                return BETA;
        } else {
            BETA = std::min(BETA, eval);
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
        return Turn(quiescenceSearch(board, ALPHA, BETA, color, MAX_QUIESCENCE_DEPTH, score), Coordinate(-4, -1, -1), Move(0, 0, 0));
    }

    // Null Move Pruning (disabled in endgame)
    if (depth > 2 && !isEndgame(board)) {
        // Temporarily "pass" the turn
        Board tempBoard = board; // Create a copy of the board
        int nullMoveScore = evaluate(tempBoard); // Evaluate the board after the null move
        int score = -solve(tempBoard, depth - 1 - NULL_MOVE_REDUCTION, -BETA, -ALPHA, -color, nullMoveScore).score;

        if (score >= BETA) {
            return Turn(BETA, Coordinate(-5, -1, -1), Move(0, 0, 0)); // Prune
        }
    }

    // Identify the best move on the board
    Turn best(color == WHITE ? -INF : INF, Coordinate(-3, -1, -1), Move(0, 0, 0));
    std::vector<Turn> moves = genMoves(board, color);
    for(size_t i = 0; i < moves.size(); ++i){
        Turn& curMove = moves[i];

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

        // Late Move Reduction
        int reduction = 0;
        if (i > 4 && depth > 2) { // Apply LMR to later moves at higher depths
            reduction = LATE_MOVE_REDUCTION;
        }

        // Recurse to the other opponent
        Turn candidate = Turn(solve(board, depth - 1 - reduction, -BETA, -ALPHA, -color, newScore).score, curMove.currentLocation, curMove.change);

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
            ALPHA = std::max(ALPHA, candidate.score);
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
            BETA = std::min(BETA, candidate.score);
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
        std::vector<char> active;
        std::vector<Turn> moves = genMoves(board, color);
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
    int searchDepth;
    if (difficulty == 1) {
        searchDepth = 3; // Medium difficulty
    } else {
        searchDepth = 4; // Hard difficulty
    }

    Turn bestMove;
    int bestScore = (color == WHITE) ? -INF : INF;

    // Aspiration Search
    int alpha = bestScore - ASPIRATION_WINDOW;
    int beta = bestScore + ASPIRATION_WINDOW;

    bestMove = solve(board, searchDepth, alpha, beta, color, evaluate(board));

     if (bestMove.score <= alpha) {
        // Lower bound failed, re-search with a wider window
        bestMove = solve(board, searchDepth, -INF, alpha, color, evaluate(board));
    } else if (bestMove.score >= beta) {
        // Upper bound failed, re-search with a wider window
        bestMove = solve(board, searchDepth, beta, INF, color, evaluate(board));
    }

    return bestMove;
}
