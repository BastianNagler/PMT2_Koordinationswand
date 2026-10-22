---
trigger: always_on
---

# Code Commenting Rules

## 1. Doxygen & Language Server Compatibility (Antigravity IDE / clangd)
- **Automatic Doxygen Generation:** All public classes, structs, methods, functions, and key constants must be documented using Doxygen-compliant docblocks (`/** ... */` or `///`).
- **Markdown-Friendly Spacing:** Always place an empty line between the brief description, parameter blocks, and return blocks. Since `clangd` renders docblocks as Markdown, empty lines prevent tags from collapsing into an unreadable single line of text:
  ```cpp
  /**
   * Reads the current temperature in degrees Celsius.
   *
   * @param[out] out_temperature_c Target variable for the measured temperature.
   *
   * @return `true` if read succeeded, `false` on bus error.
   */
  ```
- **Markdown Inline Code:** Use backticks around code identifiers, literals, and types (e.g., `true`, `false`, `nullptr`, `ESP_OK`). Both Doxygen and `clangd` render these cleanly.
- **Structured Tags:** Use standard tags with `@` prefix (`@brief`, `@param`, `@param[out]`, `@return`, `@note`, `@warning`).

## 2. Language & Style Consistency
- **English Language Requirement:** All comments, Doxygen docblocks, log messages, and code identifiers must be written in English. Do not mix German and English comments.

## 3. In-Code Comments vs. Chat Explanations
- **No Chat Explanations in Code:** Do NOT use in-code comments to explain code to the user or narrate thought processes. Explanations belong in the chat, not inside production files.
- **Header vs. Implementation:**
  - Public interfaces, class contracts, and API methods are documented in the header (`.h`) files via Doxygen.
  - Implementation files (`.cpp`) should only contain comments when explaining non-obvious hardware quirks, register timings, or errata workarounds.