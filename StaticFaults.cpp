
#include <cstddef>
#include <cstdint>

// S1: Использование неинициализированной переменной
int staticFaultUninitialized(bool condition) {
    int value;

    if (condition) {
        value = 42;
    }

    return value; // S1: value может быть не инициализирована
}

// S2: Утечка динамической памяти
void staticFaultMemoryLeak() {
    int* data = new int[100];

    data[0] = 42;

    // S2: отсутствует delete[] data
}

// S3: Выход за границы массива
int staticFaultOutOfBounds() {
    int values[3] = {10, 20, 30};

    // S3: допустимые индексы 0, 1, 2
    return values[5];
}

// S4: Деление на ноль
int staticFaultDivisionByZero() {
    int divisor = 0;
    int value = 100;

    // S4: делитель равен нулю
    return value / divisor;
}

// S5: Использование памяти после освобождения
int staticFaultUseAfterFree() {
    int* value = new int(42);

    delete value;

    // S5: обращение к освобождённой памяти
    return *value;
}
