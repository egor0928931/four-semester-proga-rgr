#pragma once

#include <QMainWindow>

#include <turingMachine/turingMachine.hpp>

class MainWindow : public QMainWindow {
    public:
    MainWindow(QWidget * parent = nullptr) : QMainWindow(parent) {

    }

    MainWindow(const char * const * path, QWidget * parent = nullptr) : QMainWindow(parent) {

    }

};

