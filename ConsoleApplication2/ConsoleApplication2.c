// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "types.h"
#include "pathfinder.h"
#include "tracker.h"

#define SMALL_DELAY() Sleep(500);
bool Running = true;


void SimMove(uint8_t rowInitial, uint8_t columnInitial, uint8_t rowFinal, uint8_t columnFinal)
{
	SimSetSensor(rowInitial, columnInitial, 0);
	Sleep(100);
	SimSetSensor(rowFinal, columnFinal, 1);
}

char* ChessPieceTypeToString(enum PieceType pieceType)
{
	switch (pieceType)
	{
	case NONE: return "0";
	case PAWN: return "P";
	case BISHOP: return "B";
	case KNIGHT: return "N";
	case ROOK: return "R";
	case QUEEN: return "Q";
	case KING: return "K";
	default: return "UNKNOWN";
	}
}

DWORD WINAPI TrackingThreadFunction(void* data)
{
	while (Running)
	{
		if (Track())
		{
			for (int8_t row = NUM_ROWS - 1; row >= 0; row--)
			{
				for (uint8_t column = 0; column < NUM_COLS; column++)
				{
					struct Piece piece = GetPiece(row, column);
					if (piece.owner == BLACK)
					{
						printf("\033[0;34m");
					}
					else if (piece.owner == WHITE)
					{
						printf("\033[0;31m");
					}
					else
					{
						printf("\033[0;37m");
					}
					printf("%s ", ChessPieceTypeToString(piece.type));
				}
				printf("\n");
			}
			printf("\033[0;37m");
			printf("\n\n");
		}
		
		Sleep(1);
	}
	return 0;
}

void PrintAllLegalPaths(uint8_t pieceRow, uint8_t pieceColumn)
{
	struct PieceCoordinate piece = { GetPiece(pieceRow, pieceColumn), pieceRow, pieceColumn };
	uint8_t numLegalPaths;
	struct Coordinate allLegalPaths[MAX_LEGAL_MOVES] = { 0 };
	CalculateAllLegalPathsAndChecks(piece, allLegalPaths, &numLegalPaths);

	for (int8_t row = NUM_ROWS - 1; row >= 0; row--)
	{
		for (uint8_t column = 0; column < NUM_COLS; column++)
		{
			struct Piece piece = GetPiece(row, column);
			if (piece.owner == BLACK)
			{
				printf("\033[0;34m");
			}
			else if (piece.owner == WHITE)
			{
				printf("\033[0;31m");
			}
			else
			{
				printf("\033[0;37m");
			}

			if (row == pieceRow && column == pieceColumn)
			{

				printf("\033[0;33m");
			}

			// Print legal move positions
			for (uint8_t i = 0; i < numLegalPaths; i++)
			{
				struct Coordinate path = allLegalPaths[i];
				if (path.row == row && path.column == column)
				{
					printf("\033[0;32m");
				}
			}

			printf("%s ", ChessPieceTypeToString(piece.type));
		}
		printf("\n");
	}
	printf("\033[0;37m");
	printf("\n\n");
}

void PrintAllLegalPathsForTeam(enum PieceOwner owner)
{
	for (int8_t row = 0; row < NUM_ROWS; row++)
	{
		for (uint8_t column = 0; column < NUM_COLS; column++)
		{
			if (GetPiece(row, column).owner == owner)
			{
				PrintAllLegalPaths(row, column);
			}
		}
	}
}

void TestLegalMoves()
{
	PrintAllLegalPaths(1, 0); // Print paths for white pawn
	SMALL_DELAY();
	SimMove(1, 0, 2, 0); // Move white pawn in column 0 from row 1 to 2
	SMALL_DELAY();

	PrintAllLegalPaths(6, 3); // Print paths for black pawn
	SMALL_DELAY();
	SimMove(6, 3, 5, 3); // Move black pawn in column 3 from row 5 to 6
	SMALL_DELAY();

	PrintAllLegalPaths(1, 2);
	SMALL_DELAY();
	SimMove(1, 2, 2, 2); // Move white pawn in column 2 from row 1 to 2
	SMALL_DELAY();

	// Move bishop
	PrintAllLegalPaths(7, 2);
	SMALL_DELAY();
	SimMove(7, 2, 4, 5);
	SMALL_DELAY();

	// Move pawn
	PrintAllLegalPaths(1, 7);
	SMALL_DELAY();
	SimMove(1, 7, 2, 7);
	SMALL_DELAY();

	// Kill pawn with bishop
	PrintAllLegalPaths(4, 5);
	SMALL_DELAY();
	SimSetSensor(2, 7, 0);
	SMALL_DELAY();
	SimSetSensor(4, 5, 0);
	SMALL_DELAY();
	SimSetSensor(2, 7, 1);
	SMALL_DELAY();

	// Kill bishop with knight
	PrintAllLegalPaths(0, 6);
	SMALL_DELAY();
	SimSetSensor(2, 7, 0);
	SMALL_DELAY();
	SimSetSensor(0, 6, 0);
	SMALL_DELAY();
	SimSetSensor(2, 7, 1);
	SMALL_DELAY();

	// Move pawn
	PrintAllLegalPaths(5, 3);
	SMALL_DELAY();
	SimMove(5, 3, 4, 3);
	SMALL_DELAY();

	// Move knight
	PrintAllLegalPaths(2, 7);
	SMALL_DELAY();
	SimMove(2, 7, 3, 5);
	SMALL_DELAY();

	// Move pawn
	PrintAllLegalPaths(4, 3);
	SMALL_DELAY();
	SimMove(4, 3, 3, 3);
	SMALL_DELAY();

	// Kill pawn with pawn
	PrintAllLegalPaths(2, 2);
	SMALL_DELAY();
	SimSetSensor(3, 3, 0);
	SMALL_DELAY();
	SimSetSensor(2, 2, 0);
	SMALL_DELAY();
	SimSetSensor(3, 3, 1);
	SMALL_DELAY();

	// Kill pawn with queen
	PrintAllLegalPaths(7, 3);
	SMALL_DELAY();
	SimSetSensor(3, 3, 0);
	SMALL_DELAY();
	SimSetSensor(7, 3, 0);
	SMALL_DELAY();
	SimSetSensor(3, 3, 1);
	SMALL_DELAY();

	PrintAllLegalPaths(0, 7);
	SMALL_DELAY();
	SimMove(0, 7, 3, 7);
	SMALL_DELAY();

	PrintAllLegalPaths(3, 3);
	SMALL_DELAY();
	SimSetSensor(3, 5, 0);
	SMALL_DELAY();
	SimSetSensor(3, 3, 0);
	SMALL_DELAY();
	SimSetSensor(3, 5, 1);
	SMALL_DELAY();

	PrintAllLegalPaths(3, 7);
	SMALL_DELAY();
	SimSetSensor(3, 5, 0);
	SMALL_DELAY();
	SimSetSensor(3, 7, 0);
	SMALL_DELAY();
	SimSetSensor(3, 5, 1);
	SMALL_DELAY();

	PrintAllLegalPaths(6, 4);
	SMALL_DELAY();
	SimMove(6, 4, 5, 4);
	SMALL_DELAY();

	PrintAllLegalPaths(3, 5);
	SMALL_DELAY();
	SimSetSensor(6, 5, 0);
	SMALL_DELAY();
	SimSetSensor(3, 5, 0);
	SMALL_DELAY();
	SimSetSensor(6, 5, 1);
	SMALL_DELAY();

	PrintAllLegalPaths(7, 4);
	SMALL_DELAY();
	SimSetSensor(6, 5, 0);
	SMALL_DELAY();
	SimSetSensor(7, 4, 0);
	SMALL_DELAY();
	SimSetSensor(6, 5, 1);
	SMALL_DELAY();
}

void TestIllegalMoves()
{
	PrintAllLegalPaths(1, 0); // Print paths for white pawn
	SMALL_DELAY();
	SimMove(1, 0, 4, 0); // Move pawn to illegal spot
	SMALL_DELAY();
	SimMove(4, 0, 1, 0); // Move pawn back to initial spot
	SMALL_DELAY();
	SimMove(1, 0, 3, 0); // Move pawn to legal spot
	SMALL_DELAY();
	
	SimMove(6, 1, 4, 1); // Move black pawn
	SMALL_DELAY();
	
	// Kill black pawn but move to wrong spot
	SimSetSensor(3, 0, 0);
	SMALL_DELAY();
	SimSetSensor(4, 2, 1);
	SMALL_DELAY();
	SimMove(4, 2, 3, 0);
	SMALL_DELAY();
	
	SimSetSensor(4, 1, 0);
	SMALL_DELAY();
	SimSetSensor(3, 0, 0);
	SMALL_DELAY();
	SimSetSensor(5, 1, 1); // Move to wrong spot after killing
	SMALL_DELAY();
	SimSetSensor(5, 1, 0);
	SMALL_DELAY();
	SimSetSensor(4, 1, 1); // Move to right spot after killing
	SMALL_DELAY();

	SimSetSensor(4, 1, 0);
	SMALL_DELAY();
	SimSetSensor(6, 2, 0);
	SMALL_DELAY();
	SimSetSensor(4, 1, 1);
	SMALL_DELAY();
	SimSetSensor(6, 2, 1);
	SMALL_DELAY();

	// Move black pawn down
	SimMove(6, 6, 4, 6);
	SMALL_DELAY();

	// Move white pawn up
	SimMove(1, 7, 3, 7);
	SMALL_DELAY();

	// Kill white pawn with black pawn
	SimSetSensor(3, 7, 0);
	SMALL_DELAY();
	SimSetSensor(4, 6, 0);
	SMALL_DELAY();
	SimSetSensor(3, 7, 1);
	SMALL_DELAY();

	// Kill black pawn with white rook
	SimSetSensor(3, 7, 0);
	SMALL_DELAY();
	SimSetSensor(0, 7, 0);
	SMALL_DELAY();
	SimSetSensor(3, 7, 1);
	SMALL_DELAY();

	// Move black bishop
	SimMove(7, 5, 6, 6);
	SMALL_DELAY();

	// Move white rook
	SimMove(3, 7, 3, 4);
	SMALL_DELAY();

	// Move black pawn down
	SimMove(6, 0, 5, 0);
	SMALL_DELAY();

	// Kill pawn in front of black king with white rook
	SimSetSensor(6, 4, 0);
	SMALL_DELAY();
	SimSetSensor(3, 4, 0);
	SMALL_DELAY();
	SimSetSensor(6, 4, 1);
	SMALL_DELAY();
	PrintAllLegalPathsForTeam(BLACK);
	
	// Kill rook with knight
	SimSetSensor(6, 4, 0);
	SMALL_DELAY();
	SimSetSensor(7, 6, 0);
	SMALL_DELAY();
	SimSetSensor(6, 4, 1);
	SMALL_DELAY();

	// Kill black pawn
	PrintAllLegalPaths(4, 1);
	SimSetSensor(5, 0, 0);
	SMALL_DELAY();
	SimSetSensor(4, 1, 0);
	SMALL_DELAY();
	SimSetSensor(5, 0, 1);
	SMALL_DELAY();

	// Move rook illegally then back legally
	SimMove(7, 0, 6, 1);
	SMALL_DELAY();
	SimMove(6, 1, 7, 0);
	SMALL_DELAY();
	SimMove(7, 0, 6, 0);
	SMALL_DELAY();


}

void TestCastling()
{
	// Move knights
	SimMove(0, 1, 2, 2);
	SMALL_DELAY();
	SimMove(7, 1, 5, 2);
	SMALL_DELAY();

	// Move pawns
	SimMove(1, 3, 2, 3);
	SMALL_DELAY();
	SimMove(6, 3, 5, 3);
	SMALL_DELAY();

	// Move bishops
	SimMove(0, 2, 2, 4);
	SMALL_DELAY();
	SimMove(7, 2, 5, 4);
	SMALL_DELAY();

	// Move queens
	SimMove(0, 3, 1, 3);
	SMALL_DELAY();
	SimMove(7, 3, 6, 3);
	SMALL_DELAY();

	// Lift rook
	SimSetSensor(0, 0, 0);
	SMALL_DELAY();
	// Lift king
	SimSetSensor(0, 4, 0);
	SMALL_DELAY();

	// Place king
	SimSetSensor(0, 2, 1);
	SMALL_DELAY();
	// Place rook in wrong spot
	SimSetSensor(0, 1, 1);
	SMALL_DELAY();
	// Move rook to right spot
	SimMove(0, 1, 0, 3);
	SMALL_DELAY();

	/*
	// Lift rook
	SimSetSensor(0, 0, 0);
	SMALL_DELAY();
	// Lift king
	SimSetSensor(0, 4, 0);
	SMALL_DELAY();

	// Place king
	SimSetSensor(0, 1, 1);
	SMALL_DELAY();
	// Place rook
	SimSetSensor(0, 2, 1);
	SMALL_DELAY();
	*/
}

int main()
{
	InitTracker();
	HANDLE trackingThread = CreateThread(NULL, 0, TrackingThreadFunction, NULL, 0, NULL);
	bool testLegalMoves = false;
	bool testIllegalMoves = true;
	bool testCastling = false;

	if (testLegalMoves)
	{
		TestLegalMoves();
	}
	else if(testIllegalMoves)
	{
		TestIllegalMoves();
	}
	else if (testCastling)
	{
		TestCastling();
	}
	
	Running = false;
	WaitForSingleObject(trackingThread, INFINITE);
}
