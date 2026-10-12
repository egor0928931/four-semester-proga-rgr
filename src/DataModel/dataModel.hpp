#pragma once

#include <cstddef>
#include <cstdint>

#include <QString>
#include <QFile>
#include <QByteArray>

#include <turingMachine/turingMachine.hpp>

#include <algorithm>

// Расширение класса машины тьюринга для IO через Qt
// Сохранение/загрузка состояния из бинарного файла
// Представление char32_t в виде UTF-32
// Преобразования между UTF-32 и UTF-16 для работы с GUI

namespace TuringQt {

#pragma pack(push, 1)

struct TapeHeader {
    int32_t origin_m;

    int32_t currPos_m;

    uint32_t dataLen;
};

struct ActionsHeader {
    uint32_t rows;
    uint32_t cols;
};

struct AlphabetHeader {
    // В парах
    uint32_t size;
};

struct AlphabetPair {
    char32_t simbol;
    uint32_t index;
};

// Общий бинарный формат
struct RawTuringMachine {
    uint32_t state_m;
    TapeHeader tapeH;
    ActionsHeader actionH;
    AlphabetHeader alphabetH;

    // Данные идут дальше

    /* ==================================
     * Tape Data
     * Данные лежат в UTF-32.
     * Запись/Чтения/Преобразование ложится на Qt.
     * ================================== */

    // Плотно, сразу После Tape Data

    /* ==================================
     * ActionsTable
     * Плотно Структуры Action
     * ================================== */

    // Плотно, сразу После ActionsTable

    /* ==================================
     * Alphabet
     * Плотно Структуры AlphabetPair
     * ================================== */

    // UNSAFE. Предполагает что за все проверки выполнены

    static char32_t * TapeData(RawTuringMachine * ptr) {
        return (char32_t * ) ( (char * ) ptr + sizeof(RawTuringMachine));
    }

    static Turing::Action * ActionsTable(RawTuringMachine * ptr) {
        return (Turing::Action * ) (
            (char * ) ptr +
            sizeof(RawTuringMachine) +
            ptr->tapeH.dataLen * sizeof(char32_t)
        );
    }

    static AlphabetPair * Alphabet(RawTuringMachine * ptr) {
        return (AlphabetPair * ) (
            (char * ) ptr +
            sizeof(RawTuringMachine) +
            ptr->tapeH.dataLen * sizeof(char32_t) +
            ptr->actionH.cols * ptr->actionH.rows * sizeof(Turing::Action)
        );
    }
};

#pragma pack(pop)

// Используем наследование что бы получить доступ к полям TuringMachine
class TuringMachineQtV : public Turing::TuringMachine {
    public:

    TuringMachineQtV() {

    }

    TuringMachineQtV(const QString & path) {
        /* =====================================
         * Чтение из файла
         * ===================================== */

        QFile file(path);

        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("File open failed!");
        }

        QByteArray bytes = file.readAll();

        file.close();

        if (bytes.size() < sizeof(RawTuringMachine)) {
            throw std::invalid_argument("Invalid Turing Machine file!");
        }

        RawTuringMachine * rawTM = (RawTuringMachine * ) bytes.data();

        RawTuringMachine tmHeader;

        std::copy(rawTM, rawTM + 1, &tmHeader);

        state_m = tmHeader.state_m;

        /* =====================================
         * Валидируем
         * ===================================== */

        /* =====================================
         * Копируем Tape из файла
         * ===================================== */

        std::vector<char32_t> tapeVec(tmHeader.tapeH.dataLen, U'_');

        char32_t * tapeData = RawTuringMachine::TapeData(rawTM);

        std::copy(tapeData, tapeData + tmHeader.tapeH.dataLen, tapeVec.data());

        Turing::Tape tape{std::move(tapeVec), tmHeader.tapeH.origin_m, tmHeader.tapeH.currPos_m};

        /* =====================================
         * Копируем Actions из файла
         * ===================================== */

        std::vector<std::vector<Turing::Action>> actions;

        actions.resize(tmHeader.actionH.rows);

        Turing::Action * actionsData = RawTuringMachine::ActionsTable(rawTM);

        for (int i = 0; i < tmHeader.actionH.rows; ++i) {
            actions[i].resize(tmHeader.actionH.cols);

            Turing::Action * start = actionsData + tmHeader.actionH.cols * i;

            std::copy(start, start + tmHeader.actionH.cols, actions[i].begin());
        }

        /* =====================================
         * Копируем Alphabet из файла
         * ===================================== */

        std::unordered_map<char32_t, uint32_t> alphabet;
        AlphabetPair * alphabetPtr = RawTuringMachine::Alphabet(rawTM);

        for (int i = 0; i < tmHeader.alphabetH.size; ++i) {
            AlphabetPair pair;

            std::copy(alphabetPtr, alphabetPtr + 1, &pair);

            alphabet[pair.simbol] = pair.index;

            alphabetPtr += 1;
        }

        tape_m = std::move(tape);
        actions_m = std::move(actions);
        alphabet_m = std::move(alphabet);
    }

    void writeToFile(const QString & path) {
        RawTuringMachine tmHeader;

        tmHeader.tapeH.currPos_m = tape_m.currPos();
        tmHeader.tapeH.origin_m = tape_m.origin();
        tmHeader.tapeH.dataLen = tape_m.size();

        tmHeader.actionH.rows = actions_m.size();
        tmHeader.actionH.cols = actions_m[0].size();

        tmHeader.alphabetH.size = alphabet_m.size();

        tmHeader.state_m = state_m;

        QByteArray data;

        data.append((const char * ) &tmHeader, sizeof(tmHeader));

        data.append((const char * ) tape_m.rawData(), tape_m.size() * sizeof(char32_t));

        for (auto & action : actions_m) {
            data.append((const char * ) action.data(), action.size() * sizeof(Turing::Action));
        }

        for (auto & value : alphabet_m) {
            AlphabetPair pair;

            pair.index = value.second;
            pair.simbol = value.first;

            data.append((const char * ) &pair, sizeof(AlphabetPair));
        }

        QFile file(path);

        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("File open failed!");
        }

        file.write(data);
        file.flush();

        file.close();
    }

    //!!!




    // Символы, разложенные по их индексам (0..N-1)
    std::vector<char32_t> alphabetOrdered() const {
        std::vector<char32_t> out(alphabet_m.size(), U'?');
        for (const auto &p : alphabet_m) {
            if (p.second < out.size())
                out[p.second] = p.first;
        }
        return out;
    }
};

}