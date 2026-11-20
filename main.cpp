#include "Chip8.h"
#include "RuntimeFaults.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

void printHelp() {
    std::cout
        << "CHIP-8 Emulator Core\n\n"
        << "Usage:\n"
        << "  CHIP8Emulator <rom> [--steps N]\n"
        << "  CHIP8Emulator --help\n";
}

int main(int argc, char* argv[]) {

    //implementing D1-3
    if (argc == 3 && std::string(argv[1]) == "--fault") {
        const std::string scenario = argv[2];

        if (scenario == "bounds") {
            RuntimeFaults::triggerBoundsError();
        } else if (scenario == "race") {
            RuntimeFaults::triggerDataRace();
        } else if (scenario == "overflow") {
            RuntimeFaults::triggerIntegerOverflow();
        } else {
            std::cerr << "Unknown fault scenario\n";
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    if (argc == 2 && std::string(argv[1]) == "--help") {
        printHelp();
        return EXIT_SUCCESS;
    }

    if (argc != 2 && argc != 4) {
        printHelp();
        return EXIT_FAILURE;
    }

    try {
        int steps = 20;

        if (argc == 4) {
            if (std::string(argv[2]) != "--steps") {
                printHelp();
                return EXIT_FAILURE;
            }

            std::size_t parsed = 0;
            std::string value = argv[3];
            steps = std::stoi(value, &parsed);

            if (parsed != value.size() || steps <= 0 || steps > 1000000) {
                throw std::runtime_error("Invalid step count");
            }
        }

        Chip8 emulator;
        emulator.loadROM(argv[1]);

        for (int i = 0; i < steps; ++i) {
            emulator.cycle();
        }

        std::cout << "Executed " << steps << " cycles\n\n";
        emulator.printState();

        std::cout << "\nDISPLAY:\n";
        emulator.printDisplay();

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}