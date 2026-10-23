#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include "config.h"

/**
 * Thread-safe, lock-free sensor state container managing binary pressed states
 * across all wall sensor fields.
 *
 * Automatically adapts to `NUM_FIELDS` with zero dynamic allocation, eliminating
 * any 32-bit integer limits or bitshift overflows.
 */
class SensorState {
private:
    std::array<std::atomic<bool>, NUM_FIELDS> states;

public:
    SensorState();
    ~SensorState() = default;
    SensorState(const SensorState&) = delete;
    SensorState& operator=(const SensorState&) = delete;

    /**
     * Updates the pressed state of a single sensor field.
     *
     * @param[in] index Linear field index (0..NUM_FIELDS-1).
     * @param[in] pressed `true` if field is currently pressed, `false` otherwise.
     */
    void setPressed(const uint8_t index, const bool pressed);

    /**
     * Atomically queries whether an individual sensor field is currently pressed.
     *
     * @param[in] index Linear field index (0..NUM_FIELDS-1).
     *
     * @return `true` if pressed, `false` if released or index out of range.
     */
    bool isPressed(const uint8_t index) const;

    /**
     * Checks if all sensor fields in the given initializer list are simultaneously pressed.
     *
     * @param[in] indices Initializer list of sensor indices to verify.
     *
     * @return `true` if every specified sensor is pressed, `false` otherwise.
     */
    bool areAllPressed(const std::initializer_list<uint8_t> indices) const;

    /**
     * Checks if all sensor fields in the given array are simultaneously pressed.
     *
     * @param[in] indices Pointer to array of sensor indices to verify.
     * @param[in] count Number of indices in the array.
     *
     * @return `true` if every specified sensor is pressed, `false` otherwise.
     */
    bool areAllPressed(const uint8_t* const indices, const size_t count) const;

    /**
     * Retrieves the total number of sensor fields managed by this container.
     *
     * @return Maximum supported field count (`NUM_FIELDS`).
     */
    constexpr size_t size() const {
        return NUM_FIELDS;
    }

    /**
     * Resets all sensor states to unpressed (`false`).
     */
    void clear();
};
