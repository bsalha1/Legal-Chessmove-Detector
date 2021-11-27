#include "pathfinder.h"
#include "tracker.h"
#include <stdlib.h>

// State Invariant Pathfinding //
static void CalculateAllPaths(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsPawn(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsRook(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsBishop(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsKnight(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsQueen(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);
static void CalculateAllPathsKing(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths);

// State Varying Pathfinding //
static void CalculateAllLegalPathsNoChecks(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths);
static void CalculateAllLegalPaths(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths, uint8_t calculateCheck);

// Utilities //
static void GetPiecesForTeam(enum PieceOwner owner, struct PieceCoordinate* pieces, uint8_t* numPieces);
static uint8_t IsValidCoordinate(struct Coordinate path);
static uint8_t IsPieceMovingStraight(struct PieceCoordinate from, struct PieceCoordinate to);
static uint8_t IsPieceBlockingStraight(struct PieceCoordinate from, struct PieceCoordinate to);
static uint8_t IsPieceMovingDiagonal(struct PieceCoordinate from, struct PieceCoordinate to);
static uint8_t IsPieceBlockingDiagonal(struct PieceCoordinate from, struct PieceCoordinate to);
static uint8_t IsPieceCoordinateSameTeam(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2);

// Allows us to draft moves and their consequences without effecting the real chessboard
static struct Piece MockChessboard[NUM_ROWS][NUM_COLS];
static struct PieceCoordinate PossibleMoveMap[NUM_PIECE_TYPES * 128 + NUM_PIECE_OWNERS * 64 + 64][MAX_LEGAL_MOVES];

/*
uint16_t GetHashCode(struct PieceCoordinate pieceCoordinate)
{
	return pieceCoordinate.piece.type * 128 + pieceCoordinate.piece.owner * 64 + pieceCoordinate.row * 8 + pieceCoordinate.column;
}

void AddPossibleMove(struct PieceCoordinate from, struct PieceCoordinate to)
{
	PossibleMoveMap[GetHashCode(from)]
}
*/

void CalculateAllLegalPathsAndChecks(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths)
{
	for (uint8_t row = 0; row < NUM_ROWS; row++)
	{
		for (uint8_t column = 0; column < NUM_COLS; column++)
		{
			MockChessboard[row][column] = GetPiece(row, column);
		}
	}

	CalculateAllLegalPaths(from, allLegalPaths, numLegalPaths, 1);
}

/**
 * @brief Calculates all legal paths but without regard for putting their own king in check. Used by WillResultInSelfCheck for each enemy piece.
 */
static void CalculateAllLegalPathsNoChecks(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths)
{
	CalculateAllLegalPaths(from, allLegalPaths, numLegalPaths, 0);
}

/**
 * @brief Calculates all legal paths with trimming off moves that would put their king in check controlled by calculateCheck
 */
static void CalculateAllLegalPaths(struct PieceCoordinate from, struct Coordinate* allLegalPaths, uint8_t* numLegalPaths, uint8_t calculateCheck)
{
	*numLegalPaths = 0;

	// Get all paths
	uint8_t numPaths;
	struct Coordinate allPaths[MAX_LEGAL_MOVES] = { 0 };
	CalculateAllPaths(from, &numPaths, allPaths);

	// Populate legal paths from all paths
	for (uint8_t i = 0; i < numPaths; i++)
	{
		struct Coordinate path = allPaths[i];
		struct PieceCoordinate to = { MockChessboard[path.row][path.column], path.row, path.column };

		if (IsPieceCoordinateSameTeam(from, to))
		{
			continue;
		}
		else if (IsPieceMovingStraight(from, to) && IsPieceBlockingStraight(from, to))
		{
			continue;
		}
		else if (IsPieceMovingDiagonal(from, to))
		{
			// For pawn to move in diagonal line, it must have an enemy piece on the diagonal
			if ((from.piece.type == PAWN) && (to.piece.owner == NEUTRAL))
			{
				continue;
			}
			else if (IsPieceBlockingDiagonal(from, to))
			{
				continue;
			}
			else if (calculateCheck && WillResultInSelfCheck(from, to))
			{
				continue;
			}
		}
		else if (calculateCheck && WillResultInSelfCheck(from, to))
		{
			continue;
		}
		allLegalPaths[(*numLegalPaths)++] = path;
	}
}

static void CalculateAllPaths(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	*numPaths = 0;

	switch (pieceCoordinate.piece.type)
	{
	case PAWN:
		CalculateAllPathsPawn(pieceCoordinate, numPaths, paths);
		break;
	case ROOK:
		CalculateAllPathsRook(pieceCoordinate, numPaths, paths);
		break;
	case BISHOP:
		CalculateAllPathsBishop(pieceCoordinate, numPaths, paths);
		break;
	case KNIGHT:
		CalculateAllPathsKnight(pieceCoordinate, numPaths, paths);
		break;
	case QUEEN:
		CalculateAllPathsQueen(pieceCoordinate, numPaths, paths);
		break;
	case KING:
		CalculateAllPathsKing(pieceCoordinate, numPaths, paths);
		break;
	}
}

static void CalculateAllPathsPawn(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t row = pieceCoordinate.piece.owner == WHITE ? pieceCoordinate.row + 1 : pieceCoordinate.row - 1;
	uint8_t column = pieceCoordinate.column;

	for (int8_t i = -1; i <= 1; i++)
	{

		struct Coordinate path = { row, column + i };
		if (IsValidCoordinate(path))
		{
			paths[(*numPaths)++] = path;
		}
	}

	if (pieceCoordinate.piece.owner == WHITE && pieceCoordinate.row == 1)
	{
		struct Coordinate path = { row + 1, column };
		paths[(*numPaths)++] = path;
	}
	else if (pieceCoordinate.piece.owner == BLACK && pieceCoordinate.row == 6)
	{
		struct Coordinate path = { row - 1, column };
		paths[(*numPaths)++] = path;
	}
}

static void CalculateAllPathsRook(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t row = pieceCoordinate.row;
	uint8_t column = pieceCoordinate.column;

	for (uint8_t move = 0; move < 8; move++)
	{
		if (move != row)
		{
			struct Coordinate path = { move, column };
			paths[(*numPaths)++] = path;
		}

		if (move != column)
		{
			struct Coordinate path = { row, move };
			paths[(*numPaths)++] = path;
		}
	}
}

static void CalculateAllPathsBishop(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t row = pieceCoordinate.row;
	uint8_t column = pieceCoordinate.column;

	for (uint8_t move = 1; move < 8; move++)
	{
		if (row + move < 8)
		{
			if (column + move < 8)
			{
				struct Coordinate path = { row + move, column + move };
				paths[(*numPaths)++] = path;
			}
			if (column >= move)
			{
				struct Coordinate path = { row + move, column - move };
				paths[(*numPaths)++] = path;
			}

		}

		if (row >= move)
		{
			if (column + move < 8)
			{
				struct Coordinate path = { row - move, column + move };
				paths[(*numPaths)++] = path;
			}
			if (column >= move)
			{
				struct Coordinate path = { row - move, column - move };
				paths[(*numPaths)++] = path;
			}
		}
	}
}

static void CalculateAllPathsKnight(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t row = pieceCoordinate.row;
	uint8_t column = pieceCoordinate.column;

	const struct Coordinate adders[] = {
		{1, 2}, {-1, 2}, {1, -2}, {-1, -2},
		{2, 1}, {-2, 1}, {2, -1}, {-2, -1}
	};

	for (uint8_t move = 0; move < sizeof(adders) / sizeof(*adders); move++)
	{
		int8_t newRow = row + adders[move].row;
		int8_t newColumn = column + adders[move].column;
		struct Coordinate path = { newRow, newColumn };
		if (IsValidCoordinate(path))
		{
			paths[(*numPaths)++] = path;
		}
	}
}

static void CalculateAllPathsQueen(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t numRookPaths = 0;
	struct Coordinate rookPaths[MAX_ROOK_MOVES] = { 0 };
	CalculateAllPathsRook(pieceCoordinate, &numRookPaths, rookPaths);

	uint8_t numBishopPaths = 0;
	struct Coordinate bishopPaths[MAX_BISHOP_MOVES] = { 0 };
	CalculateAllPathsBishop(pieceCoordinate, &numBishopPaths, bishopPaths);

	*numPaths = numRookPaths + numBishopPaths;

	// Add in Rook paths
	for (uint8_t i = 0; i < numRookPaths; i++)
	{
		paths[i] = rookPaths[i];
	}

	// Append Bishop paths
	for (uint8_t i = 0; i < numBishopPaths; i++)
	{
		paths[i + numRookPaths] = bishopPaths[i];
	}
}

static void CalculateAllPathsKing(struct PieceCoordinate pieceCoordinate, uint8_t* numPaths, struct Coordinate* paths)
{
	uint8_t row = pieceCoordinate.row;
	uint8_t column = pieceCoordinate.column;

	for (int8_t i = -1; i <= 1; i++)
	{
		for (int8_t j = -1; j <= 1; j++)
		{
			if (i == 0 && j == 0)
			{
				continue;
			}

			struct Coordinate path = { row + i, column + j };
			if (IsValidCoordinate(path))
			{
				paths[(*numPaths)++] = path;
			}
		}
	}
}

static uint8_t IsPieceBlockingStraight(struct PieceCoordinate from, struct PieceCoordinate to)
{
	// If move is in same column
	if (from.column == to.column)
	{
		uint8_t startRow = from.row > to.row ? to.row : from.row;
		uint8_t endRow = from.row > to.row ? from.row : to.row;

		for (uint8_t row = startRow + 1; row < endRow; row++)
		{
			if (MockChessboard[row][from.column].type != NONE) // If piece in row between "from" and "to" then it is blocking it.
			{
				return 1;
			}
		}
	}
	// If move is in same row
	else if (from.row == to.row)
	{
		uint8_t startColumn = from.column > to.column ? to.column : from.column;
		uint8_t endColumn = from.column > to.column ? from.column : to.column;
		for (uint8_t column = startColumn + 1; column < endColumn; column++)
		{
			// If piece in column between "from" and "to" then it is blocking it.
			if (MockChessboard[from.row][column].type != NONE)
			{
				return 1;
			}
		}
	}
	return 0;
}

uint8_t WillResultInSelfCheck(struct PieceCoordinate from, struct PieceCoordinate to)
{
	// Temporarily populate the chessboard with this move to see if it causes a self check
	MockChessboard[from.row][from.column] = EMPTY_PIECE;
	MockChessboard[to.row][to.column] = from.piece;

	enum PieceOwner enemyTeam = from.piece.owner == WHITE ? BLACK : WHITE;
	uint8_t numEnemyPieces;
	struct PieceCoordinate enemyPieces[PIECES_PER_TEAM] = { 0 };

	// For each enemy piece
	GetPiecesForTeam(enemyTeam, enemyPieces, &numEnemyPieces);
	for (uint8_t i = 0; i < numEnemyPieces; i++)
	{
		uint8_t numEnemyPieceLegalPaths;
		struct Coordinate enemyPieceLegalPaths[MAX_LEGAL_MOVES] = { 0 };

		// For each legal path this enemy piece can take
		CalculateAllLegalPathsNoChecks(enemyPieces[i], enemyPieceLegalPaths, &numEnemyPieceLegalPaths);
		for (uint8_t j = 0; j < numEnemyPieceLegalPaths; j++)
		{
			struct Coordinate enemyFinalLocation = enemyPieceLegalPaths[j];
			struct Piece killedPiece = MockChessboard[enemyFinalLocation.row][enemyFinalLocation.column];

			// If the enemy piece can take our king, this move (from -> to) will result in a check so it cannot be legal
			if ((killedPiece.type == KING) && (killedPiece.owner == from.piece.owner))
			{
				// Undo temporary move
				MockChessboard[from.row][from.column] = from.piece;
				MockChessboard[to.row][to.column] = to.piece;
				return 1;
			}
		}
	}

	// Undo temporary move
	MockChessboard[from.row][from.column] = from.piece;
	MockChessboard[to.row][to.column] = to.piece;
	return 0;
}

void GetPiecesForTeam(enum PieceOwner owner, struct PieceCoordinate* pieces, uint8_t* numPieces)
{
	*numPieces = 0;

	for (uint8_t row = 0; row < NUM_ROWS; row++)
	{
		for (uint8_t column = 0; column < NUM_COLS; column++)
		{
			struct PieceCoordinate piece = { MockChessboard[row][column], row, column };
			if (piece.piece.owner == owner)
			{
				pieces[(*numPieces)++] = piece;
			}
		}
	}
}

static uint8_t IsPieceBlockingDiagonal(struct PieceCoordinate from, struct PieceCoordinate to)
{
	uint8_t startRow;
	uint8_t startColumn;
	uint8_t endRow;
	uint8_t endColumn;

	if (from.row > to.row)
	{
		startRow = to.row;
		startColumn = to.column;
		endRow = from.row;
		endColumn = from.column;
	}
	else
	{
		startRow = from.row;
		startColumn = from.column;
		endRow = to.row;
		endColumn = to.column;
	}

	int8_t columnIncrement = startColumn > endColumn ? -1 : 1;
	int8_t column = startColumn + columnIncrement;
	for (uint8_t row = startRow + 1; row < endRow; row++, column += columnIncrement)
	{
		// If piece is between "from" and "to" on the diagonal, it is blocking it
		if (MockChessboard[row][column].type != NONE)
		{
			return 1;
		}
	}
	return 0;
}


void CalculateCastlingPositions(
	struct PieceCoordinate rookPieceCoordinate,
	struct PieceCoordinate* expectedKingPieceCoordinate, struct PieceCoordinate* expectedRookPieceCoordinate)
{
	// Fill in the piece attributes
	expectedKingPieceCoordinate->piece.owner = rookPieceCoordinate.piece.owner;
	expectedRookPieceCoordinate->piece.owner = rookPieceCoordinate.piece.owner;
	expectedKingPieceCoordinate->piece.type = KING;
	expectedRookPieceCoordinate->piece.type = ROOK;

	// Calculate expected ROOK and KING columns
	if (rookPieceCoordinate.column == 0)
	{
		expectedKingPieceCoordinate->column = 2;
		expectedRookPieceCoordinate->column = 3;
	}
	else if (rookPieceCoordinate.column == 7)
	{
		expectedKingPieceCoordinate->column = 6;
		expectedRookPieceCoordinate->column = 5;
	}

	// Calculate expected ROOK and KING rows
	if (rookPieceCoordinate.piece.owner == WHITE)
	{
		expectedKingPieceCoordinate->row = expectedRookPieceCoordinate->row = 0;
	}
	else if (rookPieceCoordinate.piece.owner == BLACK)
	{
		expectedKingPieceCoordinate->row = expectedRookPieceCoordinate->row = 7;
	}
}

static inline uint8_t IsValidCoordinate(struct Coordinate path)
{
	return path.row >= 0 && path.row < 8 && path.column >= 0 && path.column < 8;
}

static inline uint8_t IsPieceMovingStraight(struct PieceCoordinate from, struct PieceCoordinate to)
{
	return (from.column == to.column || from.row == to.row);
}

static inline uint8_t IsPieceMovingDiagonal(struct PieceCoordinate from, struct PieceCoordinate to)
{
	return abs((int8_t)from.column - (int8_t)to.column) == abs((int8_t)from.row - (int8_t)to.row);
}

static inline uint8_t IsPieceCoordinateSameTeam(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2)
{
	return pieceCoordinate1.piece.owner == pieceCoordinate2.piece.owner;
}
