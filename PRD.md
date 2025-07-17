# Product Requirements Document

This document outlines the requirements for the Chesstimation project. It is a living document that will be updated as the project evolves.

## 1. Underpromotion

- [ ] **UI for Promotion Selection:**
    - [ ] Create a new UI component in `src/main.cpp` using the LVGL library to display the promotion options (Queen, Rook, Bishop, or Knight).
    - [ ] The UI component should appear when a pawn reaches the promotion rank.
    - [ ] The UI component should be intuitive and easy to use.

- [ ] **Logic for Handling Promotion:**
    - [ ] Modify the `setPieceBackTo` function in `src/board.cpp` to handle the promotion selection.
    - [ ] The function should wait for the user's selection from the UI.
    - [ ] Based on the selection, the appropriate piece should be placed on the board.

- [ ] **Communication Protocol:**
    - [ ] Update the communication protocol to send the promotion information to the connected chess program.
    - [ ] Modify the `generateSerialBoardMessage` function in `src/board.cpp` to include the promotion information in the message.

## 2. Improved Move Handling

- [ ] **Refactor Move Logic:**
    - [ ] Refactor the move handling logic in the `loop` function in `src/main.cpp` to allow for more natural move input.
    - [ ] The new logic should detect when a piece is moved to an occupied square and automatically handle the capture.

- [ ] **Update Board State:**
    - [ ] Update the `liftPieceFrom` and `setPieceBackTo` functions in `src/board.cpp` to handle captures correctly.
    - [ ] When a capture is detected, the captured piece should be removed from the board, and the capturing piece should be moved to the square.

- [ ] **UI Feedback:**
    - [ ] Update the UI to provide clear feedback when a capture occurs.
    - [ ] This could include highlighting the captured piece or displaying a message on the screen.

