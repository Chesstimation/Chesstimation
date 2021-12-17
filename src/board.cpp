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

#include "board.h"

byte Board::calcBlockPar(const char *message)
{
    byte blockPar = 0;
    for (int i = 0; i < strlen(message); i++)
    {
        blockPar ^= (byte)(message[i]);
    }
    return blockPar;
}

void Board::extinguishMilleniumLEDs(void) {
    for(byte i=0; i<9; i++) {
        for(byte j=0; j<9; j++) {
            milleniumLEDs[i][j]=0;
        }
    }
}

void Board::updateMilleniumLEDs(const char* ledMessage) {
    uint16_t messageIdx;
    byte high, low;
    messageIdx = 2;
    for(byte col=0; col<9; col++) {
        for(byte row=0; row<9; row++) {
            high = low = 0;
            if(ledMessage[messageIdx]>='0' && ledMessage[messageIdx]<='9') high = ledMessage[messageIdx]-48;
            if(ledMessage[messageIdx]>='A' && ledMessage[messageIdx]<='F') high = ledMessage[messageIdx]-55;
            if(ledMessage[messageIdx+1]>='0' && ledMessage[messageIdx+1]<='9') low = ledMessage[messageIdx+1]-48;
            if(ledMessage[messageIdx+1]>='A' && ledMessage[messageIdx+1]<='F') low = ledMessage[messageIdx+1]-55;
            milleniumLEDs[8-col][8-row]=(high<<4)+low;
            // if(milleniumLEDs[8-col][8-row]>0) milleniumLEDs[8-col][8-row]=0xFF; // Fix for alternate LED bug in BearChess
            messageIdx+=2;
        }
    }
}

void Board::startPosition(byte queens) {
    byte p[64] = {
        BR1, BN1, BB1, BQ1, BK1, BB2, BN2, BR2,
        BP1, BP2, BP3, BP4, BP5, BP6, BP7, BP8,
        EMP, EMP, EMP, EMP, EMP, EMP, EMP, EMP,
        EMP, EMP, EMP, EMP, EMP, EMP, EMP, EMP,
        EMP, EMP, EMP, EMP, EMP, EMP, EMP, EMP,
        EMP, EMP, EMP, EMP, EMP, EMP, EMP, EMP,
        WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8,
        WR1, WN1, WB1, WQ1, WK1, WB2, WN2, WR2
    };
    if (queens)
    {
        p[19] = BQ2;
        p[43] = WQ2;
    }
    for(int i=0; i<64; i++) {
        piece[i]=flipped?p[63-i]:p[i];
    }
    // gameIdx=32;
    for(liftedIdx=0; liftedIdx<32; liftedIdx++) {
        piecesLifted[liftedIdx]=EMP;
    }
    liftedIdx = 0;        
}

Board::Board(void) {
    boardMessage[0]=0;
    liftedIdx=255;
    startPosition(0);
    lastRawRow [0] = lastRawRow [1] = lastRawRow [6] = lastRawRow [7] = 255; 
    lastRawRow [2] = lastRawRow [3] = lastRawRow [4] = lastRawRow [5] = 0; 
}

void Board::generateSerialBoardMessage(void) {
    if (emulation==0) {
    // Certabo
        boardMessage[0] = ':';
        boardMessage[1] = 0;
        char number[12];

        for(int i=0; i<64; i++) {
            if (!flipped)
            {
                sprintf(number, "0 0 0 0 %i ", piece[i]);
            }
            else
            {
                sprintf(number, "0 0 0 0 %i ", piece[63-i]);
            }
            strcat(boardMessage, number);
        }
        strcat(boardMessage, "\r\n");
    } else {
    // Chesslink
        boardMessage[0] = 's';

        for(int i=0; i<64; i++) {
            boardMessage[i+1] = flipped?FENpieceFromType(piece[i]):FENpieceFromType(piece[63-i]);
        }
        boardMessage[65]=0;
    }
}

// This function is only for debug purposes and displays the board graphically via the debug serial interface

void Board::printDebugMessage(void) {
    char pieceStr[8]=" ";
    byte pieceType = EMP;

    for(int y=0; y<8; y++) {
        Serial.print("-----------------------------------------\r\n");
        for(int x=0; x<8; x++) {
            pieceType = piece[y*8+x];
            if (pieceType!=EMP) {
                sprintf(pieceStr, "| %2d ", pieceType);
            } else {
                sprintf(pieceStr, "|    ");
            }
            Serial.print(pieceStr);
        }
        Serial.print("|\r\n");
    }
    Serial.print("-----------------------------------------\r\n");

    // Now generate the display of the Lifted Pieces List:
    Serial.print("\r\n");
    for(int i=0; i<32; i++) {
        sprintf(pieceStr, "|%2d", i);
        Serial.print(pieceStr);
    }
     Serial.print("|\r\n");
// //    strcat(boardMessage, "------------------------------------------------------------------------------------------------\r\n");
     for(int i=0; i<32; i++) {
         byte p = 0x00ff & piecesLifted[i];
         if(p!=EMP) {
            sprintf(pieceStr, "|%2d", p);
            Serial.print(pieceStr);
         } else {
             Serial.print("|  ");
         }
     }
     Serial.print("|\r\n");
// //    strcat(boardMessage, "------------------------------------------------------------------------------------------------\r\n");
     for(int i=0; i<32; i++) {
         byte boardIndex = piecesLifted[i]>>8;
         byte p = 0x00ff & piecesLifted[i];
         if(p!=EMP) {
            sprintf(pieceStr, "|%c%1d", 'A'+getColFromBoardIndex(boardIndex), 7-getRowFromBoardIndex(boardIndex)+1);
            Serial.print(pieceStr);
         } else {
             Serial.print("|  ");
         }
     }
     Serial.print("|\r\n\r\n");
    
}

char Board::FENpieceFromType(byte piece) {
    switch(piece) {
             case BR1:
             case BR2:
                return 'r';              
             case WR1:
             case WR2:
                return 'R';              
             case BN1:
             case BN2:
                return 'n';              
             case WB1:
             case WB2:
                return 'B';              
             case BB1:
             case BB2:
                return 'b';              
             case WN1:
             case WN2:
                return 'N';              
             case WP1:
             case WP2:
             case WP3:
             case WP4:
             case WP5:
             case WP6:
             case WP7:
             case WP8:
                return 'P';              
             case BP1:
             case BP2:
             case BP3:
             case BP4:
             case BP5:
             case BP6:
             case BP7:
             case BP8:
                return 'p';              
             case WQ1:
             case WQ2:
                return 'Q';              
             case BQ1:
             case BQ2:
                return 'q';              
             case WK1:
                return 'K';              
             case BK1:
                return 'k';              
             case EMP:
                return '.';              
             default:
                return '.';              
         }
}

void Board::updateLiftedPiecesString(void) {
    char piece[2]=" ";
    liftedPiecesDisplayString[0]=0;
    int start = liftedIdx-32;   // max number of pieces which can be displayed
    if(start<0) start = 0;
     for(int i=start; i<32; i++) {
         byte p = 0x00ff & piecesLifted[i];
         piece[0] = FENpieceFromType(p);
         if(piece[0]=='.') return;
         strcat(liftedPiecesDisplayString, piece);
     }
}

// This seems to be useless!
// void Board::copyPieceSetupToRaw(byte rawRow[8]) {
//     for(int row=0; row<8; row++) {
//         byte curRow = 0;
//         for(int col=0; col<8; col++) {
//             curRow<<=1;
//             if(piece[row*8+col]!=EMP) {
//                 curRow++;
//             }
//         }
//         rawRow[row]=curRow;
//     }
// }

bool Board::isWhitePawn(byte piece) {
    if(piece >= WP1 && piece <= WP8) {
        return true;
    }
    return false;
}

bool Board::isBlackPawn(byte piece) {
    if(piece >= BP1 && piece <= BP8) {
        return true;
    }
    return false;
}

byte Board::getNextPromotionPieceForWhite(byte p) {
    // WQ1 && WQ2
    bool q1 = false;
    bool q2 = false;
    for(int i=0; i<64; i++) {
        if(piece[i]==WQ1) q1=true;
        if(piece[i]==WQ2) q2=true;
    }
    if(!q1) return WQ1;
    if(!q2) return WQ2;
    return p;
}

byte Board::getNextPromotionPieceForBlack(byte p) {
    // BQ1 && BQ2
    bool q1 = false;
    bool q2 = false;
    for(int i=0; i<64; i++) {
        if(piece[i]==BQ1) q1=true;
        if(piece[i]==BQ2) q2=true;
    }
    if(!q1) return BQ1;
    if(!q2) return BQ2;
    return p;
}

void Board::setPieceBackTo(byte boardIndex)
{
    // if (liftedIdx == 0)
    //     return;
    // Check if white pawn was moved from row 7 to 8, then promote! (Internally rows are in reverse order 0-7 is 8-1)
    if (((!flipped && getRowFromBoardIndex(boardIndex) == 0 && getRowFromBoardIndex((piecesLifted[liftedIdx - 1] >> 8)) == 1 ) ||
        (flipped && getRowFromBoardIndex(boardIndex) == 7 && getRowFromBoardIndex((piecesLifted[liftedIdx - 1] >> 8)) == 6))
        && isWhitePawn(0x00ff & piecesLifted[liftedIdx - 1]))
    {
        liftedIdx--;
        if (liftedIdx >= 0)
        {
            piece[boardIndex] = getNextPromotionPieceForWhite(0x00ff & piecesLifted[liftedIdx]);
        }
    }
    // Check if black pawn was moved from row 2 to 1, then promote! (Internally rows are in reverse order 0-7 is 8-1)
    else if ((((!flipped) && getRowFromBoardIndex(boardIndex) == 7 && getRowFromBoardIndex((piecesLifted[liftedIdx - 1] >> 8)) == 6 ) ||
             (flipped && getRowFromBoardIndex(boardIndex) == 0 && getRowFromBoardIndex((piecesLifted[liftedIdx - 1] >> 8)) == 1 ))
             && isBlackPawn(0x00ff & piecesLifted[liftedIdx - 1]))
    {
        liftedIdx--;
        if (liftedIdx >= 0)
        {
            piece[boardIndex] = getNextPromotionPieceForBlack(0x00ff & piecesLifted[liftedIdx]);
        }
    }
    else
    {
        liftedIdx--;
        if (liftedIdx >= 0)
        {
            piece[boardIndex] = 0x00ff & piecesLifted[liftedIdx];
        }
    }
    if (liftedIdx >= 0)
    {
        piecesLifted[liftedIdx] = 0x0000;
    }
}

void Board::liftPieceFrom(byte boardIndex) {
//      piecesLifted[liftedIdx]=boardIndex;
//      piecesLifted[liftedIdx]<<=8;
      piecesLifted[liftedIdx]=((uint16_t)boardIndex)<<8 | piece[boardIndex];
      liftedIdx++;
      piece[boardIndex]=EMP;
}