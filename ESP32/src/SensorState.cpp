#include "SensorState.h"

SensorState::SensorState() {
    clear();
}

void SensorState::setPressed(const uint8_t index, const bool pressed) {
    if (index >= NUM_FIELDS) {
        return;
    }

    states[index].store(pressed, std::memory_order_relaxed);
}

bool SensorState::isPressed(const uint8_t index) const {
    if (index >= NUM_FIELDS) {
        return false;
    }

    return states[index].load(std::memory_order_relaxed);
}

bool SensorState::areAllPressed(const std::initializer_list<uint8_t> indices) const {
    for (const uint8_t index : indices) {
        if (!isPressed(index)) {
            return false;
        }
    }
    return true;
}

bool SensorState::areAllPressed(const uint8_t* const indices, const size_t count) const {
    if (indices == nullptr || count == 0) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        if (!isPressed(indices[i])) {
            return false;
        }
    }
    return true;
}

void SensorState::clear() {
    for (size_t i = 0; i < NUM_FIELDS; ++i) {
        states[i].store(false, std::memory_order_release);
    }
}
