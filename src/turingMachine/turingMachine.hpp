#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>

#include <algorithm>

// Сырая логика машины тьюринга

namespace Turing {

using state_t = int32_t;

struct Action {
    state_t nextState;
    char32_t setChar;

    enum class ActionsEnum {
        FORWARD,
        BACKWARD,
        STAY,
        EXIT
    } step;
};

class Tape {
    public:

    Tape() : data_m(64, U'_') {
        origin_m = data_m.size() / 2;
        currPos_m = 0;
    }

    // Интерфейс для работы
    char32_t forward(char32_t symbol) {
        write(currPos_m, symbol);

        currPos_m += 1;

        return read(currPos_m);
    }

    char32_t backward(char32_t symbol) {
        write(currPos_m, symbol);

        currPos_m -= 1;

        return read(currPos_m);
    }

    char32_t stay(char32_t symbol) {
        write(currPos_m, symbol);

        return symbol;
    }

    char32_t read(int32_t i) const {
        if (!isValid(i)) {
            return  U'_';
        }

        return data_m[origin_m + i];
    }

    char32_t readCurr() const {
        return read(currPos_m);
    }

    void write(int32_t i, char32_t symbol) {
        ensure(i);

        data_m[origin_m + i] = symbol;
    }

    int32_t currPos() const {
        return currPos_m;
    }

    private:

    // !!!! .size всегда равен максимальному размеру вектора

    bool isValid(int32_t i) const {
        return origin_m + i >= 0 && origin_m + i < data_m.size();
    }

    void ensure(int32_t i) {
        if (isValid(i)) {
            return;
        }

        int32_t index = origin_m + i;

        if (index < 0) {
            growLeft((size_t) (-index));
        } else {
            growRight((size_t) index);
        }
    }

    // Расширить, как минимум, на atLeast
    void growRight(size_t atLeast) {
        size_t old = data_m.size();

        size_t newSize = old > 0 ? old : 64;

        do {
            newSize *= 2;
        } while (atLeast + old > newSize);

        std::vector<char32_t> newData(newSize, U'_');

        std::copy(data_m.begin(), data_m.end(), newData.begin());

        data_m = std::move(newData);
    }

    // Расширить, как минимум, на atLeast
    void growLeft(size_t atLeast) {
        size_t old = data_m.size();

        size_t newSize = old > 0 ? old : 64;

        do {
            newSize *= 2;
        } while (atLeast + old > newSize);

        std::vector<char32_t> newData(newSize, U'_');

        std::copy(data_m.begin(), data_m.end(), newData.end() - old);
        origin_m += (int32_t) (newData.size() - old);

        data_m = std::move(newData);
    }

    int32_t origin_m;
    int32_t currPos_m;

    std::vector<char32_t> data_m;
};

class TuringMachine {
    public:

    TuringMachine(Tape tape, std::vector<std::vector<Action>> actions) :
        tape_m(std::move(tape)),
        actions_m(std::move(actions))
    {

    }

    state_t state() { return state_m; }

    int32_t cursor() { return tape_m.currPos(); }

    const Tape & tape() { return tape_m; }

    bool step() {
        if (state_m == -1) {
            return false;
        }

        char32_t currSimbol = tape_m.readCurr();

        Action action = actions_m[state_m][currSimbol];

        state_m = action.nextState;

        switch (action.step) {
            case Action::ActionsEnum::FORWARD:
                tape_m.forward(action.setChar);
                break;

            case Action::ActionsEnum::BACKWARD:
                tape_m.backward(action.setChar);
                break;

            case Action::ActionsEnum::STAY:
                tape_m.stay(action.setChar);
                break;

            case Action::ActionsEnum::EXIT:
                state_m = -1;
                return false;
        }

        return true;
    }

    private:

    state_t state_m = 1;

    Tape tape_m;

    std::vector<std::vector<Action>> actions_m;
};

}
