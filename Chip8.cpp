#include "Chip8.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
constexpr std::array<uint8_t, 80> FONT = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xF0, 0x10, 0xF0, 0x80, 0xF0,
    0xF0, 0x10, 0xF0, 0x10, 0xF0,
    0x90, 0x90, 0xF0, 0x10, 0x10,
    0xF0, 0x80, 0xF0, 0x10, 0xF0,
    0xF0, 0x80, 0xF0, 0x90, 0xF0,
    0xF0, 0x10, 0x20, 0x40, 0x40,
    0xF0, 0x90, 0xF0, 0x90, 0xF0,
    0xF0, 0x90, 0xF0, 0x10, 0xF0,
    0xF0, 0x90, 0xF0, 0x90, 0x90,
    0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0,
    0xE0, 0x90, 0x90, 0x90, 0xE0,
    0xF0, 0x80, 0xF0, 0x80, 0xF0,
    0xF0, 0x80, 0xF0, 0x80, 0x80
};
}

Chip8::Chip8() : randomEngine(std::random_device{}()) {
    reset();
}

void Chip8::reset() {
    memory.fill(0);
    registers.fill(0);
    stack.fill(0);
    display.fill(0);
    keypad.fill(false);

    pc = PROGRAM_START;
    index = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;

    std::copy(FONT.begin(), FONT.end(), memory.begin() + 0x50);
}

void Chip8::loadROM(const std::string& path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Cannot open ROM: " + path);
    }

    std::vector<uint8_t> rom{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    if (file.bad()) {
        throw std::runtime_error("Error reading ROM");
    }

    loadProgram(rom);
}

void Chip8::loadProgram(const std::vector<uint8_t>& program) {
    if (program.empty()) {
        throw std::runtime_error("ROM is empty");
    }

    if (program.size() > MEMORY_SIZE - PROGRAM_START) {
        throw std::runtime_error("ROM is too large");
    }

    reset();

    std::copy(
        program.begin(),
        program.end(),
        memory.begin() + PROGRAM_START
    );
}

uint16_t Chip8::fetch() {
    if (pc > MEMORY_SIZE - 2) {
        throw std::runtime_error("PC outside executable memory");
    }

    uint16_t opcode =
        (static_cast<uint16_t>(memory[pc]) << 8) |
        memory[pc + 1];

    pc += 2;
    return opcode;
}

void Chip8::cycle() {
    execute(fetch());
}

void Chip8::tickTimers() {
    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
}

void Chip8::setKey(uint8_t key, bool pressed) {
    if (key >= keypad.size()) {
        throw std::out_of_range("Invalid key");
    }

    keypad[key] = pressed;
}

void Chip8::draw(uint8_t x, uint8_t y, uint8_t height) {
    if (static_cast<std::size_t>(index) + height > memory.size()) {
        throw std::runtime_error("Sprite exceeds memory");
    }

    registers[0xF] = 0;

    for (uint8_t row = 0; row < height; ++row) {
        uint8_t sprite = memory[index + row];

        for (uint8_t col = 0; col < 8; ++col) {
            if ((sprite & (0x80 >> col)) == 0) continue;

            std::size_t px = (x + col) % SCREEN_WIDTH;
            std::size_t py = (y + row) % SCREEN_HEIGHT;
            std::size_t pos = py * SCREEN_WIDTH + px;

            if (display[pos] == 1) {
                registers[0xF] = 1;
            }

            display[pos] ^= 1;
        }
    }
}

void Chip8::execute(uint16_t opcode) {
    const uint16_t nnn = opcode & 0x0FFF;
    const uint8_t kk = opcode & 0x00FF;
    const uint8_t n = opcode & 0x000F;
    const uint8_t x = (opcode >> 8) & 0x0F;
    const uint8_t y = (opcode >> 4) & 0x0F;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                display.fill(0);
            } else if (opcode == 0x00EE) {
                if (sp == 0) {
                    throw std::runtime_error("Stack underflow");
                }
                pc = stack[--sp];
            } else {
                throw std::runtime_error("Unsupported 0x0 instruction");
            }
            break;

        case 0x1000:
            pc = nnn;
            break;

        case 0x2000:
            if (sp >= stack.size()) {
                throw std::runtime_error("Stack overflow");
            }
            stack[sp++] = pc;
            pc = nnn;
            break;

        case 0x3000:
            if (registers[x] == kk) pc += 2;
            break;

        case 0x4000:
            if (registers[x] != kk) pc += 2;
            break;

        case 0x5000:
            if (n != 0) {
                throw std::runtime_error("Invalid 5XYN instruction");
            }
            if (registers[x] == registers[y]) pc += 2;
            break;

        case 0x6000:
            registers[x] = kk;
            break;

        case 0x7000:
            registers[x] += kk;
            break;

        case 0x8000:
            switch (n) {
                case 0x0: registers[x] = registers[y]; break;
                case 0x1: registers[x] |= registers[y]; break;
                case 0x2: registers[x] &= registers[y]; break;
                case 0x3: registers[x] ^= registers[y]; break;

                case 0x4: {
                    uint16_t result = registers[x] + registers[y];
                    registers[x] = result & 0xFF;
                    registers[0xF] = result > 255;
                    break;
                }

                case 0x5: {
                    bool noBorrow = registers[x] >= registers[y];
                    registers[x] -= registers[y];
                    registers[0xF] = noBorrow;
                    break;
                }

                case 0x6: {
                    uint8_t value = registers[x];
                    registers[x] = value >> 1;
                    registers[0xF] = value & 1;
                    break;
                }

                case 0x7: {
                    bool noBorrow = registers[y] >= registers[x];
                    registers[x] = registers[y] - registers[x];
                    registers[0xF] = noBorrow;
                    break;
                }

                case 0xE: {
                    uint8_t value = registers[x];
                    registers[x] = value << 1;
                    registers[0xF] = (value >> 7) & 1;
                    break;
                }

                default:
                    throw std::runtime_error("Unknown arithmetic instruction");
            }
            break;

        case 0x9000:
            if (n != 0) {
                throw std::runtime_error("Invalid 9XYN instruction");
            }
            if (registers[x] != registers[y]) pc += 2;
            break;

        case 0xA000:
            index = nnn;
            break;

        case 0xB000:
            pc = nnn + registers[0];
            break;

        case 0xC000: {
            std::uniform_int_distribution<int> distribution(0, 255);
            registers[x] = distribution(randomEngine) & kk;
            break;
        }

        case 0xD000:
            draw(registers[x], registers[y], n);
            break;

        case 0xE000:
            if (registers[x] >= keypad.size()) {
                throw std::runtime_error("Invalid key register");
            }

            if (kk == 0x9E) {
                if (keypad[registers[x]]) pc += 2;
            } else if (kk == 0xA1) {
                if (!keypad[registers[x]]) pc += 2;
            } else {
                throw std::runtime_error("Unknown keyboard instruction");
            }
            break;

        case 0xF000:
            switch (kk) {
                case 0x07:
                    registers[x] = delayTimer;
                    break;

                case 0x0A: {
                    auto it = std::find(keypad.begin(), keypad.end(), true);
                    if (it == keypad.end()) {
                        pc -= 2;
                    } else {
                        registers[x] =
                            static_cast<uint8_t>(it - keypad.begin());
                    }
                    break;
                }

                case 0x15:
                    delayTimer = registers[x];
                    break;

                case 0x18:
                    soundTimer = registers[x];
                    break;

                case 0x1E:
                    index += registers[x];
                    break;

                case 0x29:
                    index = 0x50 + registers[x] * 5;
                    break;

                case 0x33: {
                    if (index > MEMORY_SIZE - 3) {
                        throw std::runtime_error("BCD memory overflow");
                    }

                    uint8_t value = registers[x];
                    memory[index] = value / 100;
                    memory[index + 1] = (value / 10) % 10;
                    memory[index + 2] = value % 10;
                    break;
                }

                case 0x55:
                    if (static_cast<std::size_t>(index) + x >= MEMORY_SIZE) {
                        throw std::runtime_error("Register store overflow");
                    }
                    for (unsigned i = 0; i <= x; ++i) {
                        memory[index + i] = registers[i];
                    }
                    break;

                case 0x65:
                    if (static_cast<std::size_t>(index) + x >= MEMORY_SIZE) {
                        throw std::runtime_error("Register load overflow");
                    }
                    for (unsigned i = 0; i <= x; ++i) {
                        registers[i] = memory[index + i];
                    }
                    break;

                default:
                    throw std::runtime_error("Unknown FX instruction");
            }
            break;

        default:
            throw std::runtime_error("Unknown opcode");
    }
}

uint8_t Chip8::getRegister(std::size_t reg) const {
    return registers.at(reg);
}

uint16_t Chip8::getPC() const {
    return pc;
}

uint16_t Chip8::getIndex() const {
    return index;
}

void Chip8::printState() const {
    std::cout << "PC: 0x" << std::hex << pc
              << "  I: 0x" << index << '\n';

    for (int i = 0; i < 16; ++i) {
        std::cout << "V" << std::hex << i
                  << ": " << std::setw(2)
                  << std::setfill('0')
                  << static_cast<int>(registers[i]) << "  ";

        if (i % 4 == 3) std::cout << '\n';
    }

    std::cout << std::dec << std::setfill(' ');
}

void Chip8::printDisplay() const {
    for (std::size_t y = 0; y < SCREEN_HEIGHT; ++y) {
        for (std::size_t x = 0; x < SCREEN_WIDTH; ++x) {
            std::cout << (display[y * SCREEN_WIDTH + x] ? "██" : "  ");
        }
        std::cout << '\n';
    }
}