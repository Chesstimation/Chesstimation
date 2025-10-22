# Product Requirements Document

This document outlines the requirements for the Chesstimation project. It is a living document that will be updated as the project evolves.

## 1. Underpromotion

- [x] **UI for Promotion Selection:**
    - [x] Create a new UI component in `src/main.cpp` using the LVGL library to display the promotion options (Queen, Rook, Bishop, or Knight).
    - [x] The UI component should appear when a pawn reaches the promotion rank.
    - [x] The UI component should be intuitive and easy to use.

- [x] **Logic for Handling Promotion:**
    - [x] Modify the `setPieceBackTo` function in `src/board.cpp` to handle the promotion selection.
    - [x] The function should wait for the user's selection from the UI.
    - [x] Based on the selection, the appropriate piece should be placed on the board.

- [x] **Communication Protocol:**
    - [x] Update the communication protocol to send the promotion information to the connected chess program.
    - [x] Modify the `generateSerialBoardMessage` function in `src/board.cpp` to include the promotion information in the message.


## 3. WhitePawn iOS App Compatibility

- [x] **Investigate Communication Protocol:**
    - [x] Research the WhitePawn iOS app's communication protocol (e.g., OpenExchange Protocol, BLE characteristics, message formats).
    - [x] Analyze existing Chesstimation communication logic in `src/main.cpp` and `src/board.cpp` (Certabo, Chesslink emulations).
- [x] **Identify Compatibility:**
    - [x] Confirmed that WhitePawn app supports existing emulation protocols, eliminating the need for a separate WhitePawn emulation mode.
- [x] **Implementation Approach:**
    - [x] WhitePawn compatibility achieved through existing Chesslink/Millennium emulation mode.
    - [x] Removed unnecessary WhitePawn-specific emulation code to prevent bugs and maintain code simplicity.
    - [x] Users can connect WhitePawn app using the standard Chesslink/Millennium emulation mode.
- [ ] **Test Compatibility:**
    - [ ] Conduct thorough testing with the WhitePawn iOS app using Chesslink/Millennium emulation to ensure seamless communication and correct board state synchronization.


