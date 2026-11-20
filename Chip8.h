#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

class Chip8 {
public:
    static constexpr std::size_t MEMORY_SIZE = 4096;
    static constexpr std::size_t SCREEN_WIDTH = 64;
    static constexpr std::size_t SCREEN_HEIGHT = 32;
    static constexpr uint16_t PROGRAM_START = 0x200;

    Chip8();

    void reset();
    void loadROM(const std::string& path);
    void loadProgram(const std::vector<uint8_t>& program);

    void cycle();
    void tickTimers();

    void setKey(uint8_t key, bool pressed);
    void printState() const;
    void printDisplay() const;

    uint8_t getRegister(std::size_t index) const;
    uint16_t getPC() const;
    uint16_t getIndex() const;

private:
    std::array<uint8_t, MEMORY_SIZE> memory{};
    std::array<uint8_t, 16> registers{};
    std::array<uint16_t, 16> stack{};
    std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT> display{};
    std::array<bool, 16> keypad{};

    uint16_t pc = PROGRAM_START;
    uint16_t index = 0;

    uint8_t sp = 0;
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;

    std::mt19937 randomEngine;

    uint16_t fetch();
    void execute(uint16_t opcode);
    void draw(uint8_t x, uint8_t y, uint8_t height);
};