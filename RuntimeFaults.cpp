
#include "RuntimeFaults.h"

#include <cstdint>
#include <limits>
#include <thread>

// D1: Выход за границы массива
void RuntimeFaults::triggerBoundsError() {
    auto* data = new int[2]{10, 20};

    volatile int index = 2;
    volatile int value = data[index];

    (void)value;
    delete[] data;
}

// D2: Гонка данных
void RuntimeFaults::triggerDataRace() {
    int counter = 0;

    auto increment = [&counter]() {
        for (int i = 0; i < 100000; ++i) {
            ++counter;
        }
    };

    std::thread first(increment);
    std::thread second(increment);

    first.join();
    second.join();
}

// D3: Переполнение знакового целого числа
void RuntimeFaults::triggerIntegerOverflow() {
    volatile int value = std::numeric_limits<int>::max();
    volatile int increment = 1;

    int result = value + increment;
    (void)result;
}
