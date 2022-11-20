/*  
    Copyright 2021 Andreas Petersik (andreas.petersik@gmail.com)
    
    This file is part of the Chesstimation Project.

    Chesstimation is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Chesstimation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Chesstimation.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>

// #define MAX_CERTABO_MSG_LENGTH 768
#define MAX_CERTABO_MSG_LENGTH 680

#define toBoardIndex(row, col) ((7 - row) * 8 + col)
#define getColFromBoardIndex(index) (index % 8)
#define getRowFromBoardIndex(index) (index / 8)

//  ROOK=R, KNIGHT=N, BISHOP=B, QUEEN=Q, KING=K, PAWN=P

#define EMP 0

#define BR1 0b00000101
#define BN1 0b00000111
#define BB1 0b00001001
#define BQ1 0b00001011
#define BK1 0b00001101
#define BB2 0b00011001
#define BN2 0b00010111
#define BR2 0b00010101

#define BP1 0b00000011
#define BP2 0b00010011
#define BP3 0b00100011
#define BP4 0b00110011
#define BP5 0b01000011
#define BP6 0b01010011
#define BP7 0b01100011
#define BP8 0b01110011

#define WP1 0b00000010
#define WP2 0b00010010
#define WP3 0b00100010
#define WP4 0b00110010
#define WP5 0b01000010
#define WP6 0b01010010
#define WP7 0b01100010
#define WP8 0b01110010

#define WR1 0b00000100
#define WN1 0b00000110
#define WB1 0b00001000
#define WQ1 0b00001010
#define WK1 0b00001100
#define WB2 0b00011000
#define WN2 0b00010110
#define WR2 0b00010100

#define WQ2 0b00011010
#define BQ2 0b00011011

class Board
{
public:
    byte emulation  = 1; // 0=Certabo, 1=Chesslink, 2=DGT Pegasus
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
