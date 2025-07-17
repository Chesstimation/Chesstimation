## Build, Lint, and Test

- **Build:** `pio run`
- **Upload:** `pio run --target upload`
- **Lint:** No linting is configured.
- **Test:** No testing is configured.

## Code Style

- **Language:** C++ for Arduino framework.
- **Formatting:** Use 4-space indentation. Braces on new lines for functions and classes, same line for control structures.
- **Naming:**
    - Use `camelCase` for functions and variables.
    - Use `PascalCase` for classes.
    - Use `UPPER_SNAKE_CASE` for constants and macros.
- **Imports:** Use `#include <...>` for libraries and `#include "..."` for local headers.
- **Error Handling:** No formal error handling. Return values and debug prints are used to indicate errors.
- **Comments:** Use `//` for single-line comments and `/* ... */` for multi-line comments.
- **Types:** Use `byte` for 8-bit unsigned integers. Use `int` for general-purpose integers.
- **Libraries:**
    - `TFT_eSPI` for display.
    - `lvgl` for UI.
    - `BluetoothSerial` for Bluetooth communication.
- **File Naming:** Use `lowercase_snake_case` for file names.
- **File Structure:**
    - `src/` contains all source code.
    - `include/` contains all header files.
    - `lib/` contains all libraries.
    - `platformio.ini` contains project configuration.
