#ifndef PATHFINDER_H_
#define PATHFINDER_H_

#include "types.h"

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
