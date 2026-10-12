#pragma once

#include <cstdint>
#include <cstddef>

#include <vector>

#include <algorithm>

#include <stdexcept>

#include <unordered_map>

// Сырая логика машины тьюринга

namespace Turing {

using state_t = int32_t;

struct Action {
    state_t nextState;
    char32_t setChar;

    enum class ActionsEnum : uint32_t {
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

    Tape(std::vector<char32_t> && data, int32_t origin, int32_t currPos) {
        if (data.size() <= origin || origin < 0) {
            throw std::invalid_argument("Origin outside tape data");
        }

        int32_t index = origin + currPos;
        if (data.size() <= index || index < 0) {
            throw std::invalid_argument("Current position outside tape data");
        }

        data_m = std::move(data);

        origin_m = origin;

        currPos_m = currPos;
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

    int32_t origin() const {
        return origin_m;
    }

    uint32_t size() const {
        return data_m.size();
    }

    const char32_t * rawData() const {
        return data_m.data();
    }

    void setCurrPos(int32_t p) { currPos_m = p; }

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

    TuringMachine() {
        state_m = -1;
    }

    TuringMachine(Tape && tape,
                  std::vector<std::vector<Action>> && actions,
                  std::unordered_map<char32_t, uint32_t> && alphabet)
    :
        tape_m(std::move(tape)),
        actions_m(std::move(actions)),
        alphabet_m(std::move(alphabet))
    {
        state_m = 1;
    }

    state_t state() { return state_m; }

    int32_t cursor() { return tape_m.currPos(); }

    const Tape & tape() { return tape_m; }

    bool step() {
        if (state_m == -1) {
            return false;
        }

        char32_t currSimbol = tape_m.readCurr();

        auto iter = alphabet_m.find(currSimbol);

        if (iter == alphabet_m.end()) {
            throw std::logic_error("Unknown charaster in tape");
        }

        uint32_t index = iter->second;

        Action action = actions_m[state_m][index];

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

    //!!!
    // Добавить символ в алфавит. Расширяет все строки actions_m
    // на один столбец с дефолтным Action.
    void addSymbol(char32_t c) {
        if (alphabet_m.count(c)) return;

        const uint32_t idx = static_cast<uint32_t>(alphabet_m.size());
        alphabet_m[c] = idx;

        Action def;
        def.nextState = -1;                            // halt по умолчанию
        def.setChar   = c;                             // «оставить как есть» — самое безопасное
        def.step      = Action::ActionsEnum::STAY;

        for (auto &row : actions_m) {
            row.push_back(def);
        }
    }

    // Удалить символ. Если он был не последним — переиндексировать alphabet_m.
    void removeSymbol(char32_t c) {
        auto it = alphabet_m.find(c);
        if (it == alphabet_m.end()) return;

        const uint32_t removedIdx = it->second;

        // Удалить столбец из каждой строки
        for (auto &row : actions_m) {
            if (removedIdx < row.size()) {
                row.erase(row.begin() + removedIdx);
            }
        }

        // Удалить из алфавита и сдвинуть индексы больше removedIdx
        alphabet_m.erase(it);
        for (auto &p : alphabet_m) {
            if (p.second > removedIdx) p.second -= 1;
        }
    }

    int stateCount()  const { return static_cast<int>(actions_m.size()); }

    int symbolCount() const { return static_cast<int>(alphabet_m.size()); }

    bool isHalted()    const { return state_m == -1; }

    char32_t cellAt(int32_t i) const {
        return tape_m.read(i);
    }

    void setCellAt(int32_t i, char32_t c) {
        tape_m.write(i, c);
    }

    bool isInAlphabet(char32_t c) const {
        return alphabet_m.find(c) != alphabet_m.end();
    }

    const std::unordered_map<char32_t, uint32_t> &alphabetMap() const {
        return alphabet_m;
    }

    const std::vector<std::vector<Turing::Action>> &actions() const {
        return actions_m;
    }

    void setAction(int row, int col, const Action &a) {
        if (row < 0 || row >= (int)actions_m.size()) return;
        if (col < 0 || col >= (int)actions_m[row].size()) return;
        actions_m[row][col] = a;
    }

    // Добавить одно состояние (строку) с дефолтными действиями.
    void addState() {
        std::vector<Action> row(alphabet_m.size());
        for (const auto &p : alphabet_m) {
            Action def;
            def.nextState = -1;               // halt по умолчанию
            def.setChar   = p.first;
            def.step      = Action::ActionsEnum::STAY;
            row[p.second] = def;
        }
        actions_m.push_back(std::move(row));
    }

    // Удалить состояние. Ссылки на него становятся halt, ссылки на бо́льшие — сдвигаются.
    void removeState(int idx) {
        if (idx < 0 || idx >= (int)actions_m.size()) return;
        if (actions_m.size() <= 1) return;    // минимум одно состояние

        actions_m.erase(actions_m.begin() + idx);

        for (auto &row : actions_m) {
            for (auto &a : row) {
                if (a.nextState == idx)      a.nextState = -1;
                else if (a.nextState > idx)  a.nextState -= 1;
            }
        }

        if      (state_m == idx) state_m = -1;
        else if (state_m >  idx) state_m -= 1;
    }

    // Полный сброс в «пустую» машину: одно состояние, один символ '_'
    void reset() {
        tape_m = Tape();
        alphabet_m.clear();
        alphabet_m[U'_'] = 0;
        actions_m.clear();
        actions_m.resize(1);
        actions_m[0].resize(1);
        actions_m[0][0] = Action{ -1, U'_', Action::ActionsEnum::EXIT };
        state_m = 0;
    }

    void setState(state_t s) {
        state_m = s;
    }

    void setCursor(int32_t p) { tape_m.setCurrPos(p); }

    protected:

    state_t state_m = 1;

    Tape tape_m;

    std::vector<std::vector<Action>> actions_m;

    std::unordered_map<char32_t, uint32_t> alphabet_m;
};

}
