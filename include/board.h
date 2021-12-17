/*  
    Copyright 2021 Andreas Petersik (andreas.petersik@gmail.com)
    
    This file is part of the Open Mephisto Project.

    Open Mephisto is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Open Mephisto is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Open Mephisto.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>

//#define MAX_CERTABO_MSG_LENGTH 768
#define MAX_CERTABO_MSG_LENGTH 680

#define toBoardIndex(row, col) ((7 - row) * 8 + col)
#define getColFromBoardIndex(index) (index % 8)
#define getRowFromBoardIndex(index) (index / 8)

//  ROOK=R, KNIGHT=N, BISHOP=B, QUEEN=Q, KING=K, PAWN=P

#define EMP 0

#define BR1 81
#define BN1 82
#define BB1 83
#define BQ1 84
#define BK1 85
#define BB2 86
#define BN2 87
#define BR2 88

#define BP1 71
#define BP2 72
#define BP3 73
#define BP4 74
#define BP5 75
#define BP6 76
#define BP7 77
#define BP8 78

#define WP1 21
#define WP2 22
#define WP3 23
#define WP4 24
#define WP5 25
#define WP6 26
#define WP7 27
#define WP8 28

#define WR1 11
#define WN1 12
#define WB1 13
#define WQ1 14
#define WK1 15
#define WB2 16
#define WN2 17
#define WR2 18

#define WQ2 34
#define BQ2 64

class Board
{
public:
    byte emulation  = 0; // 0=Certabo, 1=Chesslink
    byte flipped    = 0;

    // piece[64] stores, at which index position of the board which piece is located
    byte piece[64];

    byte milleniumLEDs[9][9];
    //    byte piecesInGame[32];  // All pieces which are located on the board
    uint16_t piecesLifted[32]; // All pieces which are lifted from the board: first byte: piece type, second byte boardIdx
                               //    byte gameIdx;
    int8_t liftedIdx;
    char liftedPiecesDisplayString[33];
    byte lastRawRow[8];
    char boardMessage[MAX_CERTABO_MSG_LENGTH];

    Board(void);
    byte calcBlockPar(const char *message);
    void updateMilleniumLEDs(const char *ledMessage);
    void extinguishMilleniumLEDs(void);
    void startPosition(byte queens);
    void generateSerialBoardMessage(void);
    // void copyPieceSetupToRaw(byte rawRow[8]);
    void printDebugMessage(void);
    void updateLiftedPiecesString(void);
    bool isWhitePawn(byte piece);
    bool isBlackPawn(byte piece);
    void setPieceBackTo(byte boardIndex);
    void liftPieceFrom(byte boardIndex);
    char FENpieceFromType(byte piece);
    byte getNextPromotionPieceForWhite(byte p);
    byte getNextPromotionPieceForBlack(byte p);
};
