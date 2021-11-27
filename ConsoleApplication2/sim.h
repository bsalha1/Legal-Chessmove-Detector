#ifndef SIM_H
#define SIM_H

#ifdef SIM

#include <stdio.h>

#define PRINT_SIM_FUNC() printf("%s\n", __func__)
#define PRINT_SIM(msg) printf("%s: %s\n", __func__, msg)
#define PRINT_SIM_PIECE(msg, piece_) printf("%s: %s {%d, %d} (%d, %d)\n", __func__, msg, piece_.piece.owner, piece_.piece.type, piece_.row, piece_.column)

#endif // SIM

#endif // SIM_H