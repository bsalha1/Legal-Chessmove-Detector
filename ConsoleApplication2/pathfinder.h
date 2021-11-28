#ifndef PATHFINDER_H_
#define PATHFINDER_H_

#include "types.h"
#define LEGAL_MOVE_SET_SIZE (NUM_PIECE_TYPES << 6) | ((NUM_ROWS - 1) << 3) | ((NUM_COLS - 1) << 0)

/**
 * @brief Fills LegalMove data structure with all the legal moves for the given team
 */
void CalculateTeamsLegalMoves(enum PieceOwner owner);

/**
 * @brief Determines if the given move is legal by invoking the LegalMove data structure
 */
uint8_t IsLegalMove(struct PieceCoordinate from, struct PieceCoordinate to);

/**
 * @brief Calculates all possible paths for a given piece given the current state of the chessboard. Also trims off moves that would put their king in check.
 */
void CalculateAllLegalPathsAndChecks(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths);

/**
 * @brief Returns 1 if a move (from -> to) will result in their king being in check. 0 otherwise.
 */
uint8_t WillResultInSelfCheck(struct PieceCoordinate from, struct PieceCoordinate to);

/**
 * @brief Calculates the expected castling position relative to the given rook
 */
void CalculateCastlingPositions(struct PieceCoordinate rookPieceCoordinate, struct PieceCoordinate* expectedKingPieceCoordinate, struct PieceCoordinate* expectedRookPieceCoordinate);

#endif /* PATHFINDER_H_ */
