// the main code that handles all of the bots moves in 3D Chess.
// the bot is not anything like stockfish where they find 90TB worth of chess games and see which move can cause the best win
// the way the bot "finds" move will be unknown unless if you look in the source code.
// please do not try to call this specific cpp because its only soul purpose is just for board.js
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
    {{-5,-5, 0, 5, 5}, {-5,0,10,0,-5}, {0,10,20,10,0}, {5,0,10,0,-5}, {5, 5, 0, -5, -5}},
    {{-5,-5, 0, 5, 5}, {-5,0,10,0,-5}, {0,10,20,10,0}, {5,0,10,0,-5}, {5, 5, 0, -5, -5}},
    {{-5,-5, 0, 5, 5}, {-5,0,10,0,-5}, {0,10,20,10,0}, {5,0,10,0,-5}, {5, 5, 0, -5, -5}},
    {{-5,-5, 0, 5, 5}, {-5,0,10,0,-5}, {0,10,20,10,0}, {5,0,10,0,-5}, {5, 5, 0, -5, -5}},
    {{-5,-5, 0, 5, 5}, {-5,0,10,0,-5}, {0,10,20,10,0}, {5,0,10,0,-5}, {5, 5, 0, -5, -5}}
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
    int score = piece->getMoves(board, false).size(); // Number of possible moves
    if (dynamic_cast<Bishop*>(piece) || dynamic_cast<Queen*>(piece)) {
        score *= 1.2; // Bonus for 3D diagonal movement
    }
    return score;
}

int Solver::kingSafetyScore(Board &board, int color) {
    Coordinate kingLocation = board.getKingLocation(color);
    int danger = 0;
    // Check for enemy pieces in 3D proximity
    for(int lvl = max(0,kingLocation.lvl-1); lvl <= min(4,kingLocation.lvl+1); lvl++) {
        for(int row = max(0,kingLocation.row-1); row <= min(4,kingLocation.row+1); row++) {
            for(int col = max(0,kingLocation.col-1); col <= min(4,kingLocation.col+1); col++) {
                Piece* target = board.getPieceAt(row, col, lvl);
                if(target->color == -color && target->id != 'k') danger++;
            }
        }
    }
    return -danger * 15;
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
                        Coordinate target = piece->getLocation() + m;
                        if (target.row >= 0 && target.row < BOARD_SIZE &&
                            target.col >= 0 && target.col < BOARD_SIZE &&
                            target.lvl >= 0 && target.lvl < BOARD_SIZE &&
                            board.getPieceAt(target)->getId() == ' ') score++;
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

bool Solver::attacksMultipleLayers(Move move) {
    return abs(move.lvl) > 1 || abs(move.col) > 1 || abs(move.row) > 1;
}

bool Solver::creates3DPin(Move move) {
    //Needs actual implementation to detect 3D pins
    return false;
}

int Solver::evaluateHangingPieces(Board &board, int color) {
    int hangingPieces = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            for (int lvl = 0; lvl < BOARD_SIZE; ++lvl) {
                Piece* piece = board.getPieceAt(row, col, lvl);
                if (piece->getColor() == color && piece->getId() != 'k') { //Don't want the king
                    bool isSupported = false;
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            for (int dl = -1; dl <= 1; ++dl) {
                                if (dr == 0 && dc == 0 && dl == 0) continue;
                                int newRow = row + dr, newCol = col + dc, newLvl = lvl + dl;
                                if (newRow >= 0 && newRow < BOARD_SIZE && newCol >= 0 && newCol < BOARD_SIZE && newLvl >= 0 && newLvl < BOARD_SIZE) {
                                    Piece* supportingPiece = board.getPieceAt(newRow, newCol, newLvl);
                                    if (supportingPiece->getColor() == color) {
                                        isSupported = true;
                                        break;
                                    }
                                }
                            }
                            if (isSupported) break;
                        }
                        if (isSupported) break;
                    }
                    if (!isSupported) hangingPieces++;
                }
            }
        }
    }
    return hangingPieces;
}

int Solver::evaluateThreats(Board &board, int color) {
    int threats = 0;
    // Implementation to evaluate immediate and potential threats
    // This might involve checking for attacked pieces, potential checkmates, etc.
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            for (int lvl = 0; lvl < BOARD_SIZE; ++lvl) {
                Piece* piece = board.getPieceAt(row, col, lvl);
                if(piece->getColor() == color) {
                   auto moves = piece->getMoves(board, true);
                    for (Move move : moves) {
                        Coordinate newLoc = piece->getLocation() + move;
                        Piece* target = board.getPieceAt(newLoc);
                        if (target->getColor() == -color && target->getId() != 'k'){
                            threats += pieceWeight[target->getId()]; //Value of threatened piece

                        }
                        if(attacksMultipleLayers(move)) threats += 50;
                        if(creates3DPin(move)) threats += 30;

                    }
                }
            }
        }
    }
    return threats * THREAT_WEIGHT;
}

int Solver::evaluateCubeControl(Board &board, int color) {
    int control = 0;
    for(int lvl=1; lvl<4; lvl++) {
        for(int x=1; x<4; x++) { // Center cubes
            for(int y=1; y<4; y++) {
                Piece *piece = board.getPieceAt(x, y, lvl);
                if(piece->getColor() == color) control++;
            }
        }
    }
    return control * 10;
}

int Solver::evaluate3DPatterns(Board &board) {
    // Placeholder for pattern recognition
    return 0;
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
    score += evaluateCubeControl(board, WHITE) - evaluateCubeControl(board, BLACK);
    score -= evaluateHangingPieces(board, WHITE) - evaluateHangingPieces(board, BLACK);

    if(difficulty == HARD) {
        score += evaluateCubeControl(board, WHITE) - evaluateCubeControl(board, BLACK) * 1.5;
        score += evaluate3DPatterns(board); // Pre-calculated 3D motifs
        score -= evaluateHangingPieces(board, WHITE) - evaluateHangingPieces(board, BLACK) * 2;
    }

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
    if (difficulty == HARD_MODE) {
        depth += 6; // Deeper search for hard mode
        MAX_QUIESCENCE_DEPTH = 15;
    }
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
        int eval = -solve(board, depth - 1 - reduction, -BETA, -ALPHA, -color, newScore).score;

        // Undo the move
        if(promoted){
            // Delete the pointer if the pawn was promoted
            delete board.board[newLoc.row][newLoc.col][newLoc.lvl];
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = pawn;
        }
        board.updateLocation(newLoc, -curMove.change);
        if (oldPiece->getId() != ' ') {
            board.board[newLoc.row][newLoc.col][newLoc.lvl] = oldPiece;
            oldPiece->setIsAlive(true);
        }

        if(color == WHITE){
            if(eval > best.score){
                best = curMove;
                best.score = eval;
            }
            ALPHA = std::max(ALPHA, eval);
            if(ALPHA >= BETA){
                break; // Beta cutoff
            }
        } else {
            if(eval < best.score){
                best = curMove;
                best.score = eval;
            }
            BETA = std::min(BETA, eval);
            if(BETA <= ALPHA){
                break; // Alpha cutoff
            }
        }
    }
    return best;
}
