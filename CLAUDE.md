# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Chesstimation is an ESP32-based firmware that modernizes vintage Hegener+Glaser Mephisto Modular chess computers by adding a 3.5" color touchscreen display and wireless connectivity. The device interfaces directly with the original Mephisto board hardware through a 40-pin connector and emulates Certabo/Chesslink protocols for compatibility with modern chess software.

**Hardware Platform**: LOLIN D32 (ESP32) with Waveshare 3.5" TFT touch display

## Build and Development Commands

### Basic Commands
- **Build**: `pio run`
- **Upload to device**: `pio run --target upload`
- **Clean build**: `pio run --target clean`
- **Monitor serial output**: `pio device monitor`
- **Build and upload**: `pio run --target upload && pio device monitor`

### Development Environment
- **IDE**: Visual Studio Code with PlatformIO extension
- **Framework**: Arduino for ESP32
- **Libraries**:
  - LVGL 8.3.x (UI framework)
  - TFT_eSPI 2.5.x (display driver)

### Configuration Files
- `platformio.ini`: Build configuration and library dependencies
- `include/lvgl/lv_conf.h`: LVGL configuration (custom for this project)
- `include/TFT_eSPI/User_Setup.h`: Display driver pin mappings and settings

## Architecture Overview

### System Components

**1. Hardware Interface Layer** (`mephisto.h/cpp`)
- Reads the 8x8 sensor matrix from the Mephisto board via parallel GPIO pins
- Writes LED states back to the Mephisto board LEDs
- Uses latch signals (LDC_LE, ROW_LE, CB_EN, LDC_EN) to multiplex the 40-pin connector
- Pin mappings: D0-D7 data bus connects to GPIO pins 12,13,27,14,25,26,32,33

**2. Chess Logic Layer** (`board.h/cpp`)
- Maintains piece positions in `piece[64]` array using custom piece encoding (e.g., WP1=white pawn 1, BR1=black rook 1)
- Tracks lifted pieces during moves in `piecesLifted[32]` buffer
- Generates protocol messages for Certabo and Chesslink emulation modes
- Handles pawn promotion including underpromotion to rook/bishop/knight
- Manages LED feedback for opponent moves via `milleniumLEDs[9][9]`

**3. Display and UI Layer** (`main.cpp` + LVGL)
- **Critical constraint**: Display buffer is fixed at `DISP_BUF_SIZE = 320 * 60` (38.4KB)
- LVGL-based touch interface for settings, piece selection, board display
- Shows current position with 40x40 pixel chess piece bitmaps (defined in `pieces-bitmaps.cpp`)
- Multi-language support (EN, ES, DE) via `lv_i18n.h/c`
- Custom fonts with umlauts: `montserrat_umlaute20/22.cpp`

**4. Communication Layer** (`main.cpp`)
- **USB Serial**: Direct USB-C connection for chess software
- **Bluetooth Classic** (`BluetoothSerial`): Wireless connectivity for desktop apps
- **BLE**: Low-energy Bluetooth for mobile apps (WhitePawn iOS, Chess for Android)
- Protocol switching: Certabo vs Chesslink modes determined by `Board::emulation` flag

**5. Power Management**
- SPIFFS-based persistent storage for settings and board state (file: `/board_setup`)
- Deep sleep mode with wake on touch (IRQ on GPIO 34)
- Battery monitoring and USB-C charging support
- Fast boot (<2 seconds) and shutdown

### Data Flow

```
Mephisto Board → Mephisto::readRow() → Board::piece[64] → generateSerialBoardMessage()
                                                          ↓
                                      USB/BT/BLE → Chess Software (PC/Mobile)
                                                          ↑
Chess Software Move → updateMilleniumLEDs() → Mephisto::writeRow() → Board LEDs
```

### Memory Architecture (CRITICAL)

**ESP32 LOLIN D32 has severe RAM constraints:**
- Total internal DRAM: ~320KB
- `.dram0.data` segment limit: ~64KB for static/global data
- Already consumed by:
  - LVGL display buffer: 38.4KB
  - Chess piece bitmaps: ~1.4MB in flash (large arrays)
  - Bluetooth stacks: BT Classic + BLE simultaneously
  - Global arrays: `oldBoard[64]`, `mephistoLED[8][8]`, `led_buffer[8]`, etc.

**Memory Management Rules (from memory_strategy.md):**
1. Use `PROGMEM` and `const` for all large constant data to keep it in flash
2. Never increase `DISP_BUF_SIZE` or modify display buffer configuration
3. Allocate dynamically on heap for temporary large buffers
4. Consider `heap_caps_malloc(MALLOC_CAP_SPIRAM)` for non-critical data if PSRAM available
5. Use conditional compilation to disable BT Classic OR BLE (not both) if adding features
6. Monitor `.dram0.data` overflow errors during build - indicates static data limit exceeded

## Development Constraints

### DO NOT MODIFY (from DEVELOPMENT_PLAN.md)
The following areas have been extensively debugged and are fragile:
- LVGL display buffer configuration and flush callbacks
- TFT_eSPI driver pin mappings and initialization sequence
- Chess piece bitmap loading and rendering logic
- Touch input calibration and event handling
- Screen creation and UI layout in `main.cpp`

**Rationale**: Previous DEBUG_SESSION documented extensive display integration challenges. Any changes risk breaking the working display system.

### Safe Modification Areas
- Bluetooth communication logic (protocol messages)
- Chess move validation and piece tracking
- Settings menu additions (within memory constraints)
- LED feedback patterns
- Language translations
- Power management and sleep modes

## Code Style and Conventions

### Language and Structure
- **Language**: C++ (Arduino framework)
- **Indentation**: 4 spaces
- **Braces**: New lines for functions/classes, same line for control structures
- **File naming**: `lowercase_snake_case.cpp`

### Naming Conventions
- Functions/variables: `camelCase` (e.g., `readRawRow`, `updateLiftedPiecesString`)
- Classes: `PascalCase` (e.g., `Board`, `Mephisto`)
- Constants/macros: `UPPER_SNAKE_CASE` (e.g., `DISP_BUF_SIZE`, `LED_TIME`)
- Board indices: `toBoardIndex(row, col)` macro converts row/col to 0-63 index

### Piece Encoding System
Pieces use bit-encoded byte values defined in `board.h`:
- Bit 0: Color (0=white, 1=black)
- Bits 1-2: Piece type (pawn=01, rook=10, knight=11, bishop=00, queen=01, king=10)
- Bits 3-6: Piece instance number for pawns (BP1-BP8, WP1-WP8)

Examples: `WK1=0x0C`, `BQ1=0x0B`, `WP1=0x02`, `BP8=0x73`

### Error Handling
- No formal exception handling in embedded context
- Use `Serial.printf()` for debug output
- Return values indicate success/failure where applicable
- Watchdog timer handles system crashes

## Common Development Patterns

### Adding a New Setting
1. Add variable to globals in `main.cpp` (consider memory impact)
2. Store/load from SPIFFS in `loadBoardSetup()`/`saveBoardSetup()`
3. Add UI element in settings screen creation (LVGL code)
4. Create event handler for touch interaction
5. Test memory usage: `esp_get_free_heap_size()`

### Modifying Board Communication
1. Update `generateSerialBoardMessage()` in `board.cpp` for message format
2. Handle protocol differences between Certabo (`emulation=0`) and Chesslink (`emulation=1`)
3. Test with actual chess software (WhitePawn, Lucas Chess, BearChess)
4. Verify LED feedback with `updateMilleniumLEDs()`

### Working with Display
- Use existing LVGL objects and styles (`fMediumStyle`, `fLargeStyle`, etc.)
- Chess board squares use pre-defined styles: `light_square`, `dark_square`
- Touch events handled via LVGL callbacks
- Never allocate new large image buffers - reuse existing bitmap arrays

## Testing and Validation

### Hardware Requirements
- LOLIN D32 board connected via USB-C
- Mephisto Modular board with 40-pin connection (optional for full testing)
- Waveshare 3.5" touch display properly wired

### Validation Checklist
1. Build completes without `.dram0.data` overflow errors
2. Display shows chess position correctly after upload
3. Touch calibration works (test in settings screen)
4. Board reading functional (if hardware connected)
5. Bluetooth pairing successful with test device
6. Free heap monitoring: `Serial.printf("Free heap: %d\n", esp_get_free_heap_size())`

## Important Project Documents

- `DEVELOPMENT_PLAN.md`: Feature roadmap, memory optimization strategy
- `memory_strategy.md`: Detailed ESP32 DRAM constraints and mitigation techniques
- `DISPLAY_DEBUG_SESSION.md`: Historical debugging notes on display integration
- `PRD.md`: Product requirements (underpromotion, WhitePawn compatibility)
- `README.md`: Hardware assembly, pin mappings, component list

## Development Workflow

1. **Before making changes**: Check current memory usage in build output
2. **During development**: Add `Serial.printf()` debugging as needed
3. **After changes**: Build, check for memory overflow, test on hardware
4. **Committing**: Follow existing commit style (see git log)

## Known Limitations

- Pawn promotion UI currently supports queen/rook/bishop/knight (underpromotion implemented)
- Captured pieces must be removed before capturing piece is placed
- Cannot combine with other Mephisto modules or power supplies
- Display driver expects specific Waveshare 3.5" hardware (ILI9488-based)
- BT Classic and BLE running simultaneously consumes significant memory

## External Dependencies

This project requires custom configurations for external libraries:
- **LVGL**: Must use `include/lvgl/lv_conf.h` from this repository
- **TFT_eSPI**: Must use `include/TFT_eSPI/User_Setup.h` and `User_Setup_Select.h` from this repository

Do not use default library configurations - they will not work with this hardware setup.
