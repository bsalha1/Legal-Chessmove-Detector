#include <inttypes.h>

#ifndef TYPES_H_
#define TYPES_H_

#define NUM_COLS 8
#define NUM_ROWS 8
#define PIECES_PER_TEAM 16
#define MAX_LEGAL_MOVES 27
#define MAX_ROOK_MOVES 14
#define MAX_BISHOP_MOVES 13

struct Coordinate {
	int8_t row;
	int8_t column;
};

enum PieceType {
	NONE,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	NUM_PIECE_TYPES
};

enum PieceOwner {
	NEUTRAL,
	WHITE,
	BLACK,
	NUM_PIECE_OWNERS
};

enum TransitionType {
	PICKUP,
	PLACE
};

/*
struct GPIO_Pin {
	uint16_t pin;
	GPIO_TypeDef* bus;
};
*/

struct Piece {
	enum PieceType type;
	enum PieceOwner owner;
};

struct PieceCoordinate {
	struct Piece piece;
	uint8_t row;
	uint8_t column;
};


struct IllegalMove {
	struct PieceCoordinate destination;
	struct PieceCoordinate current;
};


struct Moves {
	struct PieceCoordinate from;
	struct Coordinate moves[MAX_LEGAL_MOVES];
	uint8_t numMoves;
};


volatile static const struct Piece EMPTY_PIECE = { NONE, NEUTRAL };
volatile static const struct PieceCoordinate EMPTY_PIECE_COORDINATE = { {NONE, NEUTRAL}, 0, 0 };
volatile static const struct PieceCoordinate OFFBOARD_PIECE_COORDINATE = { {NONE, NEUTRAL}, 0xFF, 0xFF };

#endif /* TYPES_H_ */
