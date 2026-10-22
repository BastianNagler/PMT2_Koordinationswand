---
trigger: always_on
---

# Embedded Architecture Rules

## 1. Programming Language & Standards
- Implementation must happen in `src/` while header files should be in `include/`.
- Header files should always use the file extension `.h` and include `#pragma once`.
- Code should be clean, modular, and object-oriented.
- Target platform is ESP32-S3 using the Arduino framework combined with FreeRTOS.

## 2. Memory Allocation Policy (CRITICAL)
- **STRICTLY ZERO DYNAMIC ALLOCATION**:
  - No `malloc()`, `calloc()`, `realloc()`, `free()`.
  - No `pvPortMalloc()`, `vPortFree()`.
  - No C++ `new`, `new[]`, `delete`, `delete[]`.
  - No dynamic STL containers (`std::vector`, `std::string`, `std::map`, `std::unique_ptr`, `std::shared_ptr`, etc.).
  - Always prefer `std::array`, `std::string_view`, fixed ring buffers, or statically sized structs.
- **Networking & Third-Party Library Isolation**:
  - Libraries requiring heap allocation (e.g., `ESPAsyncWebServer`, `WiFi`, `ArduinoJson`) must be isolated strictly to the web/networking layer (`webTask` / `WebInterface`).
  - Avoid repeated or unbounded runtime allocations in cyclic tasks.
- **Review Requirement**:
  - Flag, report, and reject any dynamic memory allocations introduced into real-time loops, drivers, or game state logic.

## 3. ESP32 & FreeRTOS Multitasking Safety
- **Non-Blocking Task Delays:** Always yield CPU time to lower-priority tasks using `vTaskDelay(pdMS_TO_TICKS(ms))`; do not use blocking `delay()` in FreeRTOS tasks.
- **Task Watchdog Timer (TWDT):** Tasks registered with the Task Watchdog (`esp_task_wdt_add(NULL)`) must reset it periodically (`esp_task_wdt_reset()`) and avoid operations that block past the watchdog timeout.
- **Concurrency & Shared State:** Synchronize access to shared resources across tasks using mutexes (`std::mutex`, `std::recursive_mutex`), FreeRTOS semaphores, queues, or atomic/volatile flags.
- **ISRs (Interrupt Handlers):** All ISR functions must have the `IRAM_ATTR` attribute, remain minimal and non-blocking, never allocate memory, and only use FreeRTOS `*FromISR` APIs.

## 4. Const-Correctness
- **Member Functions:** Mark all member functions `const` that do not modify the object's observable state (getters, queries, state checks).
- **Parameters & Buffers:** Input buffers and read-only arguments must be passed by `const` pointer or reference (`const uint8_t *data`, `const std::string_view`, `const Type &`).
- **Immutable Members:** Member variables that are set during construction and never modified must be declared `const` (e.g. `const uint8_t cs_pin_;`, `const uint8_t dev_addr_;`, `const UBaseType_t priority_;`).
- **Local Variables (`const`-by-default):** Any local variable, intermediate calculation, or status code not mutated after initialization must be declared `const` (e.g. `const esp_err_t status = ...;`, `const uint8_t i2c_addr = ...;`).
- **Constants & Lookup Tables:** Use `static constexpr` or `inline constexpr` for all compile-time constants and tables.