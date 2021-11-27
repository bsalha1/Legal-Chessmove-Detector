#include <math.h>
#include "tracker.h"
#include "pathfinder.h"
#include "types.h"
#ifdef SIM
#include <windows.h>
#include "sim.h"
#else
#include "chessclock.h"
#endif

// Placement Handlers //
static void HandlePlace(struct PieceCoordinate placedPiece);
static void HandlePlaceIllegalState(struct PieceCoordinate placedPiece);
static void HandlePlaceKill(struct PieceCoordinate placedPiece);
static void HandlePlaceCastling(struct PieceCoordinate placedPiece);
static void HandlePlaceMove(struct PieceCoordinate placedPiece);
static void HandlePlaceNoMove(struct PieceCoordinate placedPiece);
static void HandlePlacePreemptPromotion(struct PieceCoordinate placedPiece);
static void HandlePlacePromotion(struct PieceCoordinate placedPiece);

// Pickup Handlers //
static void HandlePickup(struct PieceCoordinate pickedUpPiece);
static void HandlePickupIllegalState(struct PieceCoordinate pickedUpPiece);
static void HandlePickupPreemptKill(struct PieceCoordinate pickedUpPiece);
static void HandlePickupKill(struct PieceCoordinate pickedUpPiece);
static void HandlePickupCastling(struct PieceCoordinate pickedUpPiece);
static void HandlePickupMove(struct PieceCoordinate pickedUpPiece);
static void HandlePickupPromotion(struct PieceCoordinate pickedUpPiece);

// Internal Updaters //
static void UpdateCastleFlags();
static void AddIllegalPiece(struct PieceCoordinate current, struct PieceCoordinate destination);
static void RemoveIllegalPiece(uint8_t index);
static void CheckChessboardValidity(uint8_t switchTurns);
static void EndTurn();
static void SetPiece(uint8_t row, uint8_t column, struct Piece piece);
static void SetPieceCoordinate(struct PieceCoordinate pieceCoordinate);

// Legal Move Detection //
static uint8_t ValidateMove(struct PieceCoordinate from, struct PieceCoordinate to);
static uint8_t ValidateKill(struct PieceCoordinate victim, struct PieceCoordinate killer);
static uint8_t ValidateCastling(struct PieceCoordinate rook, struct PieceCoordinate king);
static uint8_t DidOtherTeamPickupLast(struct Piece piece);
static uint8_t DidSameTeamPickupLast(struct Piece piece);

// Utilities //
uint8_t PawnReachedEnd(struct PieceCoordinate pieceCoordinate);



// State Fields //
static struct Piece Chessboard[NUM_ROWS][NUM_COLS];
static enum PieceOwner CurrentTurn;
static enum TransitionType LastTransitionType;
static struct PieceCoordinate LastPickedUpPiece;

// Legal Piece Detection/Recovery Fields //
static struct PieceCoordinate PieceToKill;
static struct IllegalMove IllegalPieces[NUM_ILLEGAL_PIECES];
static uint8_t NumIllegalPieces;
static uint8_t SwitchTurnsAfterLegalState;

// Castling //
static uint8_t CanA1Castle;
static uint8_t CanH1Castle;
static uint8_t CanA8Castle;
static uint8_t CanH8Castle;
static uint8_t CanWhiteKingCastle;
static uint8_t CanBlackKingCastle;
static struct PieceCoordinate ExpectedKingCastleCoordinate;
static struct PieceCoordinate ExpectedRookCastleCoordinate;

// Promotion //
static struct PieceCoordinate PawnToPromote;



#ifdef SIM
uint8_t SimColumn = 0;
volatile uint8_t SimSensors[NUM_ROWS][NUM_COLS] = {
	{1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1},
};

void SimSetSensor(uint8_t row, uint8_t column, uint8_t value)
{
	SimSensors[row][column] = value;
}

uint8_t SimGetSensor(uint8_t row)
{
	return SimSensors[row][SimColumn];
}
#endif

#ifndef SIM
static void InitGPIO_Pin(struct GPIO_Pin pin, uint32_t mode, uint32_t pull)
{
	// Enable GPIO Bus
	if (pin.bus == GPIOA)
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();
	}
	else if (pin.bus == GPIOB)
	{
		__HAL_RCC_GPIOB_CLK_ENABLE();
	}
	else if (pin.bus == GPIOC)
	{
		__HAL_RCC_GPIOC_CLK_ENABLE();
	}
	else
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
	}

	if (mode == GPIO_MODE_OUTPUT_PP)
	{
		HAL_GPIO_WritePin(pin.bus, pin.pin, GPIO_PIN_RESET);
	}

	// Configure GPIO
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = pin.pin;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Pull = pull;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(pin.bus, &GPIO_InitStruct);
}
#endif

void InitTracker()
{
	// Initialize globals
	LastTransitionType = PLACE;
	CurrentTurn = WHITE;
	CanA1Castle = 1;
	CanH1Castle = 1;
	CanA8Castle = 1;
	CanH8Castle = 1;
	CanWhiteKingCastle = 1;
	CanBlackKingCastle = 1;
	SwitchTurnsAfterLegalState = 0;

	ClearPiece(&LastPickedUpPiece);
	ClearPiece(&PieceToKill);
	ClearPiece(&ExpectedKingCastleCoordinate);
	ClearPiece(&ExpectedRookCastleCoordinate);
	ClearPiece(&PawnToPromote);

#ifndef SIM
	// Initialize output column bits IO and the chessboard data structure
	for (uint8_t columnBit = 0; columnBit < NUM_COL_BITS; columnBit++)
	{
		InitGPIO_Pin(COLUMN_BIT_TO_PIN_TABLE[columnBit], GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
	}

	// Initialize input rows
	for (uint8_t row = 0; row < NUM_ROWS; row++)
	{
		InitGPIO_Pin(ROW_NUMBER_TO_PIN_TABLE[row], GPIO_MODE_INPUT, GPIO_NOPULL);
	}
#endif

	// Initialize the board data structure to the initial chessboard
	for (uint8_t column = 0; column < NUM_COLS; column++)
	{
		for (uint8_t row = 0; row < NUM_ROWS; row++)
		{
			Chessboard[row][column] = INITIAL_CHESSBOARD[row][column];
		}
	}

	// Initialize illegal piece destinations to empty pieces
	NumIllegalPieces = 0;
	for (uint8_t i = 0; i < NUM_ILLEGAL_PIECES; i++)
	{
		IllegalPieces[i].destination = EMPTY_PIECE_COORDINATE;
		IllegalPieces[i].current = EMPTY_PIECE_COORDINATE;
	}
}

static void WriteColumn(uint8_t column)
{
#ifndef SIM
	uint8_t columnBit0 = (column & 1) >> 0;
	uint8_t columnBit1 = (column & 2) >> 1;
	uint8_t columnBit2 = (column & 4) >> 2;

	HAL_GPIO_WritePin(COLUMN_BIT_TO_PIN_TABLE[0].bus, COLUMN_BIT_TO_PIN_TABLE[0].pin, columnBit0);
	HAL_GPIO_WritePin(COLUMN_BIT_TO_PIN_TABLE[1].bus, COLUMN_BIT_TO_PIN_TABLE[1].pin, columnBit1);
	HAL_GPIO_WritePin(COLUMN_BIT_TO_PIN_TABLE[2].bus, COLUMN_BIT_TO_PIN_TABLE[2].pin, columnBit2);
#else
	SimColumn = column;
#endif
}

static uint8_t ReadRow(uint8_t rowNumber)
{
	uint8_t value;

#ifndef SIM
	struct GPIO_Pin rowPin = ROW_NUMBER_TO_PIN_TABLE[rowNumber];
	GPIO_PinState value = HAL_GPIO_ReadPin(rowPin.bus, rowPin.pin);
#else
	value = SimGetSensor(rowNumber);
#endif

	return value;
}

uint8_t Track()
{
	uint8_t transitionOccured = 0;

	for (uint8_t column = 0; column < NUM_COLS; column++)
	{
		WriteColumn(column);

		for (uint8_t row = 0; row < NUM_ROWS; row++)
		{
			uint8_t cellValue = ReadRow(row);

			struct PieceCoordinate currentPieceCoordinate = GetPieceCoordinate(row, column);

			// If there was no piece here but the IO is HIGH, a piece was placed
			if ((currentPieceCoordinate.piece.type == NONE) && (cellValue == 1))
			{
				HandlePlace(currentPieceCoordinate);
				transitionOccured = 1;
			}

			// If there was a piece here but the IO is LOW, a piece has been picked up
			else if ((currentPieceCoordinate.piece.type != NONE) && (cellValue == 0))
			{
				HandlePickup(currentPieceCoordinate);
				transitionOccured = 1;
			}
		}
	}

	return transitionOccured;
}

static void HandlePlace(struct PieceCoordinate placedPiece)
{
	// If board is in illegal state
	if (NumIllegalPieces > 0)
	{
		HandlePlaceIllegalState(placedPiece);
	}

	// Don't do anything except update board if piece did not move
	else if (IsPieceCoordinateSamePosition(placedPiece, LastPickedUpPiece))
	{
		HandlePlaceNoMove(placedPiece);
	}

	// If there's a piece being killed, this placement should be in its stead (if it's a legal/possible kill)
	else if (PieceExists(PieceToKill))
	{
		HandlePlaceKill(placedPiece);
	}

	// If player is castling
	else if (PieceExists(ExpectedKingCastleCoordinate) || PieceExists(ExpectedRookCastleCoordinate))
	{
		HandlePlaceCastling(placedPiece);
	}

	// If promotion is occurring, this placed piece must be a knight or queen placed into PawnToPromote's place
	else if (PieceExists(PawnToPromote))
	{
		HandlePlacePromotion(placedPiece);
	}

	// Any other move, the last picked up piece is set to this position
	else
	{
		HandlePlaceMove(placedPiece);
	}


	// If pawn reaches last row, it must be replaced by a queen or knight in this move 
	if (PawnReachedEnd(placedPiece))
	{
		HandlePlacePreemptPromotion(placedPiece);
	}

	LastTransitionType = PLACE;
}

static void HandlePlaceIllegalState(struct PieceCoordinate placedPiece)
{
	PRINT_SIM("Chessboard in illegal state, validating...");

	for (uint8_t i = 0; i < NumIllegalPieces; i++)
	{
		// If placing an illegal piece in it's proper destination, remove it from the illegal pieces array
		if (IsPieceCoordinateSamePosition(IllegalPieces[i].destination, placedPiece))
		{
			SetPiece(placedPiece.row, placedPiece.column, IllegalPieces[i].destination.piece);

			// Remove from illegal pieces array
			RemoveIllegalPiece(i);

			// If chessboard is valid, switch turns if flagged to do so
			CheckChessboardValidity(SwitchTurnsAfterLegalState);

			return;
		}
	}

	// A piece was placed in an unexpected destination, add it as an illegal piece that must be removed from the board
	AddIllegalPiece(placedPiece, OFFBOARD_PIECE_COORDINATE);
}

static void HandlePlaceNoMove(struct PieceCoordinate placedPiece)
{
	SetPiece(placedPiece.row, placedPiece.column, LastPickedUpPiece.piece);
}

static void HandlePlaceKill(struct PieceCoordinate placedPiece)
{
	// If player put killer in victim's place, clear PieceToKill
	if (IsPieceCoordinateSamePosition(PieceToKill, placedPiece))
	{
		ClearPiece(&PieceToKill);
		EndTurn();
	}
	// If player didn't put killer in the victim's spot, must put the killer in the victim spot
	else
	{
		// Put killer in victim spot
		struct PieceCoordinate killerDestination = PieceToKill;
		killerDestination.piece = LastPickedUpPiece.piece;
		AddIllegalPiece(placedPiece, killerDestination);
		SwitchTurnsAfterLegalState = 1;
	}
	SetPiece(placedPiece.row, placedPiece.column, LastPickedUpPiece.piece);
}

static void HandlePlaceCastling(struct PieceCoordinate placedPiece)
{
	// If placing a piece in the King's expected location, assume it's a king and place it
	if (IsPieceCoordinateSamePosition(ExpectedKingCastleCoordinate, placedPiece))
	{
		SetPiece(placedPiece.row, placedPiece.column, ExpectedKingCastleCoordinate.piece);
		ClearPiece(&ExpectedKingCastleCoordinate);
	}
	// If placing a piece in the Rook's expected location, assume it's a rook and place it
	else if (IsPieceCoordinateSamePosition(ExpectedRookCastleCoordinate, placedPiece))
	{
		SetPiece(placedPiece.row, placedPiece.column, ExpectedRookCastleCoordinate.piece);
		ClearPiece(&ExpectedRookCastleCoordinate);
	}
	// If placing piece in wrong location
	else
	{
		// If King wasn't already placed in correct spot, put it in the correct spot
		if (PieceExists(ExpectedKingCastleCoordinate))
		{
			SetPiece(placedPiece.row, placedPiece.column, ExpectedKingCastleCoordinate.piece); // Assume the king was placed here (doesn't matter)
			AddIllegalPiece(placedPiece, ExpectedKingCastleCoordinate);
			SwitchTurnsAfterLegalState = 1;
		}

		// If Rook wasn't already placed in correct spot, put it in correct spot
		if (PieceExists(ExpectedRookCastleCoordinate))
		{
			SetPiece(placedPiece.row, placedPiece.column, ExpectedRookCastleCoordinate.piece); // Assume the rook was placed here (doesn't matter)
			AddIllegalPiece(placedPiece, ExpectedRookCastleCoordinate);
			SwitchTurnsAfterLegalState = 1;
		}
	}

	// If castling has been fulfilled
	if (!PieceExists(ExpectedKingCastleCoordinate) && !PieceExists(ExpectedRookCastleCoordinate))
	{
		EndTurn();
	}
}

static void HandlePlaceMove(struct PieceCoordinate placedPiece)
{
	if (ValidateMove(LastPickedUpPiece, placedPiece))
	{
		EndTurn();
	}
	// If move was invalid, put piece back
	else
	{
		AddIllegalPiece(placedPiece, LastPickedUpPiece);
	}
	SetPiece(placedPiece.row, placedPiece.column, LastPickedUpPiece.piece);
}

static void HandlePlacePreemptPromotion(struct PieceCoordinate placedPiece)
{
	/// @todo trigger audio cue to promote and then enable the button 1 and button 2 to be queen and knight 
	PawnToPromote = placedPiece;
}

static void HandlePlacePromotion(struct PieceCoordinate placedPiece)
{
	// If placed the promoted piece back into the pawn's old spot, get the PieceType (knight or queen) from the stored button state and set the piece as that type
	if (IsPieceCoordinateSamePosition(placedPiece, PawnToPromote))
	{
		/// @todo get button data, and set the right piececoordinate to the right PieceType
		placedPiece.piece.type = QUEEN;
		SetPiece(placedPiece.row, placedPiece.column, placedPiece.piece);
		ClearPiece(&PawnToPromote); // promotion is done
	}

	// If player doesn't place the promotion into the pawn's old spot, it must be placed in the right spot
	else
	{
		AddIllegalPiece(placedPiece, PawnToPromote);
	}
}



static void HandlePickup(struct PieceCoordinate pickedUpPiece)
{
	SetPiece(pickedUpPiece.row, pickedUpPiece.column, EMPTY_PIECE);

	// If a piece is picked up during an illegal state, if it's not an illegal piece it is NOW illegal
	if (NumIllegalPieces > 0)
	{
		HandlePickupIllegalState(pickedUpPiece);
	}
	
	// If player picked up piece from other team, they will kill pieceCoordinate
	else if (pickedUpPiece.piece.owner != CurrentTurn)
	{
		HandlePickupPreemptKill(pickedUpPiece);
	}

	// If there's a piece to kill, this picked up piece must be able to kill it
	else if (PieceExists(PieceToKill))
	{
		HandlePickupKill(pickedUpPiece);
	}

	// If there's a pawn to promote, the picked up piece must be this pawn
	else if (PieceExists(PawnToPromote))
	{
		HandlePickupPromotion(pickedUpPiece);
	}

	// Same team picked up piece twice in a row castling is occurring
	else if (DidSameTeamPickupLast(pickedUpPiece.piece))
	{
		HandlePickupCastling(pickedUpPiece);
	}

	// If simple pickup
	else
	{
		HandlePickupMove(pickedUpPiece);
	}

	LastPickedUpPiece = pickedUpPiece;
	LastTransitionType = PICKUP;
}

static void HandlePickupIllegalState(struct PieceCoordinate pickedUpPiece)
{
	PRINT_SIM("Chessboard in illegal state, validating...");

	for (uint8_t i = 0; i < NumIllegalPieces; i++)
	{
		// If pickup for illegal piece, let it slide
		if (IsPieceCoordinateEqual(IllegalPieces[i].current, pickedUpPiece))
		{
			// If pickup an illegal piece which is to be removed from the board is picked up, it is no longer illegal
			if (IsPieceCoordinateEqual(IllegalPieces[i].destination, OFFBOARD_PIECE_COORDINATE))
			{
				// Remove from illegal pieces array
				RemoveIllegalPiece(i);

				// If chessboard is valid, switch turns if flagged to do so
				CheckChessboardValidity(SwitchTurnsAfterLegalState);
			}
			return;
		}
	}
	
	// Player picked up a piece that wasn't illegal, so it must be added as an illegal piece which must be placed back
	AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, pickedUpPiece);
}

static void HandlePickupPreemptKill(struct PieceCoordinate pickedUpPiece)
{
	PieceToKill = pickedUpPiece;
}

static void HandlePickupKill(struct PieceCoordinate pickedUpPiece)
{
	// If piece can't kill PieceToKill, they need to be put back to their initial positions, and PieceToKill is not a piece to kill anymore
	if (!ValidateKill(PieceToKill, pickedUpPiece))
	{
		AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, PieceToKill);
		AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, pickedUpPiece);
		ClearPiece(&PieceToKill);
	}
}

static void HandlePickupCastling(struct PieceCoordinate pickedUpPiece)
{
	struct PieceCoordinate rook;
	struct PieceCoordinate king;

	if (pickedUpPiece.piece.type == ROOK && LastPickedUpPiece.piece.type == KING)
	{
		rook = pickedUpPiece;
		king = LastPickedUpPiece;
	}
	else if (pickedUpPiece.piece.type == KING && LastPickedUpPiece.piece.type == ROOK)
	{
		rook = LastPickedUpPiece;
		king = pickedUpPiece;
	}
	// If the past two picked up pieces aren't a king and rook, put them back
	else
	{
		AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, pickedUpPiece);
		AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, LastPickedUpPiece);
		return;
	}

	if (ValidateCastling(rook, king))
	{
		struct PieceCoordinate expectedKingPieceCoordinate;
		struct PieceCoordinate expectedRookPieceCoordinate;
		CalculateCastlingPositions(rook, &expectedKingPieceCoordinate, &expectedRookPieceCoordinate);

		// If castling won't result in a self-check then it's valid so copy to globals. Otherwise fall through to AddIllegalPiece.
		if (!WillResultInSelfCheck(rook, expectedRookPieceCoordinate) && !WillResultInSelfCheck(king, expectedKingPieceCoordinate))
		{
			ExpectedKingCastleCoordinate = expectedKingPieceCoordinate;
			ExpectedRookCastleCoordinate = expectedRookPieceCoordinate;
			return;
		}
	}

	AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, pickedUpPiece);
	AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, LastPickedUpPiece);
}

static void HandlePickupPromotion(struct PieceCoordinate pickedUpPiece)
{
	// All picked up pieces during a promotion must be the PawnToPromote, otherwise they must be placed back
	if (!IsPieceCoordinateEqual(pickedUpPiece, PawnToPromote))
	{
		AddIllegalPiece(OFFBOARD_PIECE_COORDINATE, pickedUpPiece);
	}
}

static void HandlePickupMove(struct PieceCoordinate pickedUpPiece)
{
	// If this piece isn't owned by the current team, then they must put it back down
	if (pickedUpPiece.piece.owner != CurrentTurn)
	{
		AddIllegalPiece(EMPTY_PIECE_COORDINATE, pickedUpPiece);
	}
}


/**
 * @brief Put an illegal piece in the IllegalPieceDestinations array. Destination is the correct destination of the piece and Current is the current position of the piece.
 */
static void AddIllegalPiece(struct PieceCoordinate current, struct PieceCoordinate destination)
{
	PRINT_SIM_PIECE("Put piece in: ", destination);

	current.piece = destination.piece;

	IllegalPieces[NumIllegalPieces].current = current;
	IllegalPieces[NumIllegalPieces].destination = destination;
	NumIllegalPieces++;
}

/**
 * @brief Remove illegal piece from IllegalPieces array given its index
 */
static void RemoveIllegalPiece(uint8_t index)
{
	NumIllegalPieces--;
	for (uint8_t i = index; i < NumIllegalPieces; i++)
	{
		IllegalPieces[i] = IllegalPieces[i + 1];
	}
}

/**
 * @brief Check if chessboard is valid and switch turns if flagged to do so
 */
static void CheckChessboardValidity(uint8_t switchTurns)
{
	if (NumIllegalPieces == 0)
	{
		PRINT_SIM("Chessboard is valid!");
		if (switchTurns)
		{
			EndTurn();
		}
	}
}

/**
 * @brief Return 1 if the given killer can take the victim, 0 otherwise. If the victim cannot be killed, then this is an illegal/impossible kill
 * so the victim and killer must return to their original spots, and a new move must be done.
 */
static uint8_t ValidateKill(struct PieceCoordinate victim, struct PieceCoordinate killer)
{
	// Temporarily add back victim and then check if it can be killed (need to be done for PAWN)
	SetPieceCoordinate(victim);

	uint8_t valid = ValidateMove(killer, victim);

	// Clear victim again
	victim.piece = EMPTY_PIECE;
	SetPieceCoordinate(victim);

	return valid;
}

/**
 * @brief Return 1 if the "to" is in the legal paths for "from", 0 otherwise. If the move is invalid, then the "from" must be placed back
 * in its original spot, and a new move must be done.
 */
static uint8_t ValidateMove(struct PieceCoordinate from, struct PieceCoordinate to)
{
	uint8_t numLegalPaths;
	struct Coordinate allLegalPaths[MAX_LEGAL_MOVES] = { 0 };
	CalculateAllLegalPathsAndChecks(from, allLegalPaths, &numLegalPaths);

	for (uint8_t i = 0; i < numLegalPaths; i++)
	{
		if ((allLegalPaths[i].column == to.column) && (allLegalPaths[i].row == to.row))
		{
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Return 1 if the given rook can castle with the given king. If not, they should return to their original positions.
 */
static uint8_t ValidateCastling(struct PieceCoordinate rook, struct PieceCoordinate king)
{
	// If white king can castle and the king and rook are in the starting row
	if (king.row == 0 && rook.row == 0 && CanWhiteKingCastle)
	{
		return (rook.column == 0 && CanA1Castle) || (rook.column == 7 && CanH1Castle);
	}
	// If black king can castle and the king and rook are in the starting row
	else if (king.row == 7 && rook.row == 7 && CanBlackKingCastle)
	{
		return (rook.column == 0 && CanA8Castle) || (rook.column == 7 && CanH8Castle);
	}
	return 0;
}


#ifndef SIM
uint8_t ValidateStartPositions()
{
	for (uint8_t columnNumber = 0; columnNumber < NUM_COLS; columnNumber++)
	{
		WriteColumn(columnNumber);
		for (uint8_t rowNumber = 0; rowNumber < NUM_ROWS; rowNumber++)
		{
			GPIO_PinState cellValue = ReadRow(rowNumber);

			switch (rowNumber)
			{

				// Make sure rows 0, 1, 6, 7 are all filled with pieces
			case 0:
			case 1:
			case 6:
			case 7:
				if (cellValue == GPIO_PIN_RESET)
				{
					return 0;
				}
				break;

				// Make sure other rows do not have a piece
			default:
				if (cellValue == GPIO_PIN_SET)
				{
					return 0;
				}
			}
		}
	}

	return 1;
}
#endif

static void EndTurn()
{
	UpdateCastleFlags();

	SwitchTurnsAfterLegalState = 0;

	// Switch teams
	CurrentTurn = CurrentTurn == WHITE ? BLACK : WHITE;
	if (CurrentTurn == WHITE)
	{
		PRINT_SIM("Switching team to WHITE");
	}
	else
	{
		PRINT_SIM("Switching team to BLACK");
	}
}

static void UpdateCastleFlags()
{
	// If any rooks moved, flag them as not castle-able
	if (CanA1Castle && !IsPiecePresent(ROOK_A1_COORDINATE))
	{
		CanA1Castle = 0;
	}
	else if (CanH1Castle && !IsPiecePresent(ROOK_H1_COORDINATE))
	{
		CanH1Castle = 0;
	}
	else if (CanA8Castle && !IsPiecePresent(ROOK_A8_COORDINATE))
	{
		CanA8Castle = 0;
	}
	else if (CanH8Castle && !IsPiecePresent(ROOK_H8_COORDINATE))
	{
		CanH8Castle = 0;
	}
	// If any kings moved, flag them as not castle-able
	else if (CanWhiteKingCastle && !IsPiecePresent(WHITE_KING_COORDINATE))
	{
		CanWhiteKingCastle = 0;
	}
	else if (CanBlackKingCastle && !IsPiecePresent(BLACK_KING_COORDINATE))
	{
		CanBlackKingCastle = 0;
	}
}

uint8_t PawnReachedEnd(struct PieceCoordinate pieceCoordinate)
{
	uint8_t finalRow = CurrentTurn == WHITE ? 7 : 0;
	return (pieceCoordinate.piece.owner == CurrentTurn) && (pieceCoordinate.piece.type == PAWN) && (pieceCoordinate.row == finalRow);
}

inline uint8_t PieceExists(struct PieceCoordinate pieceCoordinate)
{
	return !IsPieceCoordinateEqual(pieceCoordinate, EMPTY_PIECE_COORDINATE);
}

inline void ClearPiece(struct PieceCoordinate* pieceCoordinate)
{
	*pieceCoordinate = EMPTY_PIECE_COORDINATE;
}

inline void SetPiece(uint8_t row, uint8_t column, struct Piece piece)
{
	Chessboard[row][column] = piece;
}

inline void SetPieceCoordinate(struct PieceCoordinate pieceCoordinate)
{
	Chessboard[pieceCoordinate.row][pieceCoordinate.column] = pieceCoordinate.piece;
}

inline struct Piece GetPiece(uint8_t row, uint8_t column)
{
	return Chessboard[row][column];
}

inline struct PieceCoordinate GetPieceCoordinate(uint8_t row, uint8_t column)
{
	struct PieceCoordinate pieceCoordinate = { GetPiece(row, column), row, column };
	return pieceCoordinate;
}

inline uint8_t DidOtherTeamPickupLast(struct Piece piece)
{
	return LastTransitionType == PICKUP && LastPickedUpPiece.piece.owner != piece.owner;
}

inline uint8_t DidSameTeamPickupLast(struct Piece piece)
{
	return LastTransitionType == PICKUP && LastPickedUpPiece.piece.owner == piece.owner;
}

inline uint8_t IsPieceEqual(struct Piece piece1, struct Piece piece2)
{
	return piece1.owner == piece2.owner
		&& piece1.type == piece2.type;
}

uint8_t IsPiecePresent(uint8_t row, uint8_t column)
{
	return Chessboard[row][column].type != NONE;
}

inline uint8_t IsPieceCoordinateEqual(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2)
{
	return IsPieceEqual(pieceCoordinate1.piece, pieceCoordinate2.piece)
		&& pieceCoordinate1.row == pieceCoordinate2.row
		&& pieceCoordinate1.column == pieceCoordinate2.column;
}

inline uint8_t IsPieceCoordinateSamePosition(struct PieceCoordinate pieceCoordinate1, struct PieceCoordinate pieceCoordinate2)
{
	return pieceCoordinate1.row == pieceCoordinate2.row && pieceCoordinate1.column == pieceCoordinate2.column;
}