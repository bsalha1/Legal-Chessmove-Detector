#ifndef TRACKER_H
#define TRACKER_H

#ifndef SIM
#include "stm32l1xx_hal.h"
#endif

#include "types.h"

/* Constants */

#define NUM_ILLEGAL_PIECES 32
#define NUM_COL_BITS 3
#define ROOK_A1_COORDINATE 0, 0
#define ROOK_A8_COORDINATE 7, 0
#define ROOK_H1_COORDINATE 0, 7
#define ROOK_H8_COORDINATE 7, 7
#define WHITE_KING_COORDINATE 0, 4
#define BLACK_KING_COORDINATE 7, 4


#ifndef SIM
volatile static const struct GPIO_Pin ROW_NUMBER_TO_PIN_TABLE[NUM_ROWS] = {
	{ GPIO_PIN_10, GPIOA }, // D2
	{ GPIO_PIN_3,  GPIOB }, // D3
	{ GPIO_PIN_5,  GPIOB }, // D4
	{ GPIO_PIN_4,  GPIOB }, // D5
	{ GPIO_PIN_10, GPIOB }, // D6
	{ GPIO_PIN_8,  GPIOA }, // D7
	{ GPIO_PIN_9,  GPIOA }, // D8
	{ GPIO_PIN_7,  GPIOC }, // D9

};

volatile static const struct GPIO_Pin COLUMN_BIT_TO_PIN_TABLE[NUM_COL_BITS] = {
	{ GPIO_PIN_0,  GPIOA }, // A0
	{ GPIO_PIN_1,  GPIOA }, // A1
	{ GPIO_PIN_4,  GPIOA }, // A2

};
#else
void SimSetSensor(uint8_t row, uint8_t column, uint8_t value);
void SimMove(uint8_t rowInitial, uint8_t columnInitial, uint8_t rowFinal, uint8_t columnFinal);
#endif

volatile static const struct Piece INITIAL_CHESSBOARD[NUM_ROWS][NUM_COLS] = {
	{{ROOK, WHITE},   {KNIGHT, WHITE}, {BISHOP, WHITE}, {QUEEN, WHITE},  {KING, WHITE},   {BISHOP, WHITE}, {KNIGHT, WHITE}, {ROOK, WHITE}},
	{{PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE},   {PAWN, WHITE}},
	{{NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}},
	{{NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}},
	{{NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}},
	{{NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}, {NONE, NEUTRAL}},
	{{PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK},   {PAWN, BLACK}},
	{{ROOK, BLACK},   {KNIGHT, BLACK}, {BISHOP, BLACK}, {QUEEN, BLACK},  {KING, BLACK},   {BISHOP, BLACK}, {KNIGHT, BLACK}, {ROOK, BLACK}},
};



/* Functions */

/**
 * @brief Write column bits to MUXs and read the value of each row. Keep track of piece moves.
 */
uint8_t Track(void);


/**
 * @brief Initialize IO ports to use for tracking via the Hall Effect sensors.
 */
void InitTracker(void);


/**
 * @brief Make sure top and bottom 2 rows sensors are HIGH at the beginning of the match 
 */
uint8_t ValidateStartPositions(void);


/**
 * @brief Returns the piece at the specified row and column.
 */
struct Piece GetPiece(uint8_t row, uint8_t column);
struct PieceCoordinate GetPieceCoordinate(uint8_t row, uint8_t column);
struct Piece** GetChessboard();

// Comparison //
uint8_t IsPieceEqual(struct Piece piece1, struct Piece piece2);
uint8_t IsPiecePresent(uint8_t row, uint8_t column);
uint8_t IsPieceCoordinateEqual(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2);
uint8_t IsPieceCoordinateSamePosition(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2);


uint8_t PieceExists(struct PieceCoordinate placedPiece);
void ClearPiece(struct PieceCoordinate* pieceCoordinate);

#endif // TRACKER_H
