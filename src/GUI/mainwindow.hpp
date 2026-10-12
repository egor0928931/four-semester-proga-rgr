#pragma once

#include <QMainWindow>

#include <DataModel/dataModel.hpp>

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>

#include <QVector>
#include <QString>
#include <QChar>

#include <QStatusBar>

#include <QTableWidget>
#include <QHeaderView>

#include <QListWidget>

#include <QColor>

#include <QComboBox>

#include <QFileDialog>

#include <QMenuBar>

#include <QTimer>

class TapeWidget : public QWidget {
    Q_OBJECT
public:
    explicit TapeWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_rowLayout = new QHBoxLayout(this);
        m_rowLayout->setSpacing(2);
        m_rowLayout->setContentsMargins(0, 0, 0, 0);
        // Прижимаем колонки влево, справа пустота:
        buildCells();
    }

    void setMachine(TuringQt::TuringMachineQtV *m) {
        m_machine = m;
        refresh();
    }

    void setViewCenter(int32_t center) {
        m_viewCenter = center;
        refresh();
    }

    int32_t viewCenter() const { return m_viewCenter; }

    void setCellCount(int n) {
        if (n <= 0 || n == m_cellCount) return;
        m_cellCount = n;
        buildCells();
        refresh();
    }

    void refresh() {
        if (!m_machine) return;

        const int32_t cursor = m_machine->cursor();
        const int     half   = m_cellCount / 2;

        for (int i = 0; i < m_cellCount; ++i) {
            const int32_t  abs = m_viewCenter - half + i;
            const char32_t raw = m_machine->cellAt(abs);
            const QChar    c   = charToQChar(raw);

            m_indexLabels[i]->setText(QString::number(abs));
            m_indexLabels[i]->setStyleSheet(
                abs == cursor ? kHeadIndexStyle : kNormalIndexStyle);

            auto *cell = m_cells[i];
            cell->blockSignals(true);
            cell->setText(QString(c));
            cell->blockSignals(false);
            cell->setStyleSheet(abs == cursor ? kHeadCellStyle : kNormalCellStyle);
        }
    }

signals:
    void cellEdited(int32_t absoluteIndex, char32_t symbol);

private:
    static QChar charToQChar(char32_t c) {
        return (c <= 0xFFFF) ? QChar(static_cast<char16_t>(c)) : QChar(u'?');
    }

    void buildCells() {
        // Чистим предыдущие колонки (вместе с вложенными виджетами)
        for (auto *col : m_columns) {
            m_rowLayout->removeWidget(col);
            col->deleteLater();
        }
        m_columns.clear();
        m_cells.clear();
        m_indexLabels.clear();

        static constexpr int kColWidth = 34;

        for (int i = 0; i < m_cellCount; ++i) {
            // Колонка = QWidget с QVBoxLayout
            auto *col = new QWidget(this);
            col->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto *colLayout = new QVBoxLayout(col);
            colLayout->setSpacing(2);
            colLayout->setContentsMargins(0, 0, 0, 0);

            // --- индекс ---
            auto *idx = new QLabel(col);
            idx->setAlignment(Qt::AlignCenter);
            idx->setFixedHeight(14);
            idx->setStyleSheet(kNormalIndexStyle);
            colLayout->addWidget(idx);

            // --- клетка ---
            auto *cell = new QLineEdit(col);
            cell->setMaxLength(1);
            cell->setAlignment(Qt::AlignCenter);
            cell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            cell->setStyleSheet(kNormalCellStyle);

            connect(cell, &QLineEdit::textChanged, this,
                    [this, i](const QString &t) { onCellTextChanged(i, t); });

            colLayout->addWidget(cell);

            m_indexLabels.append(idx);
            m_cells.append(cell);
            m_columns.append(col);

            // Вставляем перед правой растяжкой
            m_rowLayout->addWidget(col);
        }
    }

    void onCellTextChanged(int visibleIndex, const QString &text) {
        if (!m_machine) return;

        const int     half = m_cellCount / 2;
        const int32_t abs  = m_viewCenter - half + visibleIndex;

        if (text.isEmpty()) {
            m_machine->setCellAt(abs, U'_');
            emit cellEdited(abs, U'_');
            return;
        }

        const char32_t c = text.at(0).unicode();

        if (!m_machine->isInAlphabet(c)) {
            const char32_t prev = m_machine->cellAt(abs);
            auto *cell = m_cells[visibleIndex];
            cell->blockSignals(true);
            cell->setText(QString(charToQChar(prev)));
            cell->blockSignals(false);
            return;
        }

        m_machine->setCellAt(abs, c);
        emit cellEdited(abs, c);
    }

    static constexpr const char *kNormalIndexStyle =
        "QLabel { color:#555; font-size:10px; }";
    static constexpr const char *kHeadIndexStyle   =
        "QLabel { color:#8a5a00; font-size:10px; font-weight:bold; }";

    static constexpr const char *kNormalCellStyle =
        "QLineEdit { background:white; border:1px solid #888; }";
    static constexpr const char *kHeadCellStyle   =
        "QLineEdit { background:#ffe08a; border:2px solid #c48a00; }";

    TuringQt::TuringMachineQtV *m_machine    = nullptr;
    QHBoxLayout                *m_rowLayout  = nullptr;
    QVector<QWidget*>           m_columns;      // каждая колонка
    QVector<QLineEdit*>         m_cells;
    QVector<QLabel*>            m_indexLabels;
    int                         m_cellCount  = 11;
    int32_t                     m_viewCenter = 0;
};

class ActionCellWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActionCellWidget(QWidget *parent = nullptr) : QWidget(parent) {
        using namespace Qt::StringLiterals;

        auto *l = new QHBoxLayout(this);
        l->setContentsMargins(2, 2, 2, 2);
        l->setSpacing(2);

        m_stateCombo = new QComboBox(this);
        m_setCombo   = new QComboBox(this);
        m_stepCombo  = new QComboBox(this);

        m_stepCombo->addItem(u"→"_s, int(Turing::Action::ActionsEnum::FORWARD));
        m_stepCombo->addItem(u"←"_s, int(Turing::Action::ActionsEnum::BACKWARD));
        m_stepCombo->addItem(u"•"_s, int(Turing::Action::ActionsEnum::STAY));
        m_stepCombo->addItem(u"■"_s, int(Turing::Action::ActionsEnum::EXIT));

        l->addWidget(m_stateCombo);
        l->addWidget(m_setCombo);
        l->addWidget(m_stepCombo);

        connect(m_stateCombo, &QComboBox::currentIndexChanged,
                this, &ActionCellWidget::onChanged);
        connect(m_setCombo,   &QComboBox::currentIndexChanged,
                this, &ActionCellWidget::onChanged);
        connect(m_stepCombo,  &QComboBox::currentIndexChanged,
                this, &ActionCellWidget::onChanged);
    }

    void setStateCount(int n) {
        m_stateCombo->blockSignals(true);
        m_stateCombo->clear();
        m_stateCombo->addItem("halt", -1);
        for (int i = 0; i < n; ++i)
            m_stateCombo->addItem(QString("q%1").arg(i), i);
        m_stateCombo->blockSignals(false);
    }

    void setAlphabet(const std::vector<char32_t> &symbols) {
        m_setCombo->blockSignals(true);
        m_setCombo->clear();
        for (char32_t c : symbols)
            m_setCombo->addItem(QString(charToQChar(c)), QVariant::fromValue<uint>(c));
        m_setCombo->blockSignals(false);
    }

    void setAction(const Turing::Action &a) {
        int s = m_stateCombo->findData(int(a.nextState));
        m_stateCombo->setCurrentIndex(s >= 0 ? s : 0);

        int c = m_setCombo->findData(QVariant::fromValue<uint>(a.setChar));
        m_setCombo->setCurrentIndex(c >= 0 ? c : 0);

        int st = m_stepCombo->findData(int(a.step));
        m_stepCombo->setCurrentIndex(st >= 0 ? st : 0);
    }

    Turing::Action action() const {
        Turing::Action a;
        a.nextState = Turing::state_t(m_stateCombo->currentData().toInt());
        a.setChar   = char32_t(m_setCombo->currentData().toUInt());
        a.step      = Turing::Action::ActionsEnum(m_stepCombo->currentData().toInt());
        return a;
    }

signals:
    void changed();

private:
    static QChar charToQChar(char32_t c) {
        return (c <= 0xFFFF) ? QChar(static_cast<char16_t>(c)) : QChar(u'?');
    }
    void onChanged() { emit changed(); }

    QComboBox *m_stateCombo = nullptr;
    QComboBox *m_setCombo   = nullptr;
    QComboBox *m_stepCombo  = nullptr;
};

class ActionsTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit ActionsTableWidget(QWidget *parent = nullptr) : QWidget(parent) {
        auto *l = new QVBoxLayout(this);
        l->setContentsMargins(0, 0, 0, 0);

        // ── шапка: [States: N]   [new symbol: __] [+]
        auto *bar = new QHBoxLayout();
        bar->addWidget(new QLabel("States:"));
        m_statesSpin = new QSpinBox(this);
        m_statesSpin->setMinimum(1);
        m_statesSpin->setMaximum(1000);
        bar->addWidget(m_statesSpin);

        bar->addSpacing(12);

        bar->addWidget(new QLabel("Symbol:"));
        m_symbolInput = new QLineEdit(this);
        m_symbolInput->setMaxLength(1);
        m_symbolInput->setFixedWidth(40);
        m_symbolInput->setAlignment(Qt::AlignCenter);
        bar->addWidget(m_symbolInput);

        m_addSymbolBtn = new QPushButton("+", this);
        m_addSymbolBtn->setFixedWidth(28);
        bar->addWidget(m_addSymbolBtn);

        bar->addStretch(1);
        l->addLayout(bar);

        m_table = new QTableWidget(this);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        l->addWidget(m_table);

        connect(m_statesSpin,   &QSpinBox::valueChanged,
                this, &ActionsTableWidget::onStatesSpinChanged);
        connect(m_addSymbolBtn, &QPushButton::clicked,
                this, &ActionsTableWidget::onAddSymbolClicked);
        connect(m_symbolInput,  &QLineEdit::returnPressed,
                this, &ActionsTableWidget::onAddSymbolClicked);
    }

    void setMachine(TuringQt::TuringMachineQtV *m) {
        m_machine = m;
        rebuild();
    }

    void rebuild() {
        if (!m_machine) return;

        const auto &actions = m_machine->actions();
        const auto  symbols = m_machine->alphabetOrdered();

        const int rows = (int)actions.size();
        const int cols = (int)symbols.size();

        m_table->setRowCount(0);
        m_table->setColumnCount(0);

        m_statesSpin->blockSignals(true);
        m_statesSpin->setValue(rows);
        m_statesSpin->blockSignals(false);

        m_table->setRowCount(rows);
        m_table->setColumnCount(cols);

        QStringList hHeaders;
        for (char32_t c : symbols) hHeaders << QString(charToQChar(c));
        m_table->setHorizontalHeaderLabels(hHeaders);

        QStringList vHeaders;
        for (int i = 0; i < rows; ++i) vHeaders << QString("q%1").arg(i);
        m_table->setVerticalHeaderLabels(vHeaders);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                auto *cell = new ActionCellWidget(m_table);
                cell->setStateCount(rows);
                cell->setAlphabet(symbols);
                cell->setAction(actions[r][c]);

                connect(cell, &ActionCellWidget::changed, this,
                        [this, r, c, cell]() {
                    if (!m_machine) return;
                    m_machine->setAction(r, c, cell->action());
                    updateHighlight();
                });

                m_table->setCellWidget(r, c, cell);
            }
        }

        applyHighlight();
    }

    void updateHighlight() { applyHighlight(); }

signals:
    void alphabetChanged();   // наружу, чтобы MainWindow обновил ленту
    void stateCountChanged();

private slots:
    void onStatesSpinChanged(int n) {
        if (!m_machine) return;

        int cur = m_machine->stateCount();
        while (cur < n) { m_machine->addState();           ++cur; }
        while (cur > n) { m_machine->removeState(cur - 1); --cur; }

        rebuild();
        emit stateCountChanged();   // ← добавить
    }

    void onAddSymbolClicked() {
        if (!m_machine) return;
        if (m_symbolInput->text().isEmpty()) return;

        const char32_t c = m_symbolInput->text().at(0).unicode();

        // Дубликат? — ничего не делаем
        for (char32_t s : m_machine->alphabetOrdered())
            if (s == c) { m_symbolInput->clear(); return; }

        m_machine->addSymbol(c);
        m_symbolInput->clear();
        rebuild();
        emit alphabetChanged();
    }

private:
    static QChar charToQChar(char32_t c) {
        return (c <= 0xFFFF) ? QChar(static_cast<char16_t>(c)) : QChar(u'?');
    }

    void applyHighlight() {
        if (!m_machine) return;

        const int cur = m_machine->state();

        for (int r = 0; r < m_table->rowCount(); ++r) {
            auto *h = m_table->verticalHeaderItem(r);
            if (h)
                h->setBackground(r == cur ? QColor("#ffe08a") : QColor(Qt::white));
        }

        for (int r = 0; r < m_table->rowCount(); ++r) {
            const bool isCur = (r == cur);
            for (int c = 0; c < m_table->columnCount(); ++c) {
                auto *w = m_table->cellWidget(r, c);
                if (!w) continue;
                w->setStyleSheet(isCur
                                      ? "ActionCellWidget { background:#fff4c8; }"
                                      : "ActionCellWidget { background:transparent; }");
            }
        }
    }

    TuringQt::TuringMachineQtV *m_machine      = nullptr;
    QTableWidget               *m_table        = nullptr;
    QSpinBox                   *m_statesSpin   = nullptr;
    QLineEdit                  *m_symbolInput  = nullptr;
    QPushButton                *m_addSymbolBtn = nullptr;
};

class MainWindow : public QMainWindow {
    public:

    MainWindow(QWidget * parent = nullptr) : QMainWindow(parent) {
        // root
        auto * root = new QWidget(this);
        auto * rootLayout  = new QVBoxLayout(root);
        setCentralWidget(root);

        auto *fileMenu = menuBar()->addMenu("&File");

        fileMenu->addAction("&Open...", QKeySequence::Open, this, [this]{
            const QString path = QFileDialog::getOpenFileName(
                this, "Open Turing Machine", QString(),
                "Turing machine (*.tm);;All files (*)");
            if (!path.isEmpty()) openFile(path);
        });

        fileMenu->addAction("&Save As...", QKeySequence::SaveAs, this, [this]{
            QFileDialog dlg(this, "Save Turing Machine");
            dlg.setAcceptMode(QFileDialog::AcceptSave);
            dlg.setNameFilter("Turing machine (*.tm);;All files (*)");
            dlg.setDefaultSuffix("tm");

            if (dlg.exec() != QDialog::Accepted) return;
            const QStringList files = dlg.selectedFiles();
            if (files.isEmpty()) return;

            QString path = files.first();

            // Страховка: если всё же без расширения и фильтр был "All files" —
            // всё равно допишем .tm (можно убрать, если не хочешь навязывать)
            if (!path.endsWith(".tm", Qt::CaseInsensitive))
                path += ".tm";

            saveFile(path);
        });

        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", QKeySequence::Quit, this, &QWidget::close);

        // -- simLayout
        auto * simLayout = new QVBoxLayout();
        rootLayout->addLayout(simLayout);

        // -- simControlLayout
        auto * simControlLayout = new QHBoxLayout();
        simLayout->addLayout(simControlLayout);

        buttonMakeOneStep_m = new QPushButton("Step", root);
        connect(buttonMakeOneStep_m, &QPushButton::clicked, this, &MainWindow::doOneStep);
        simControlLayout->addWidget(buttonMakeOneStep_m);

        buttonSwitch_m = new QPushButton("Start", root);
        connect(buttonSwitch_m, &QPushButton::clicked, this, [this](int ms) {
            if (timer_m->isActive())
                stopSimulation();
            else
                startSimulation();
        });
        simControlLayout->addWidget(buttonSwitch_m);

        spinBoxSimTickMs_m = new QSpinBox(root);
        spinBoxSimTickMs_m->setSuffix(" Ms");
        spinBoxSimTickMs_m->setMaximum(10000);
        spinBoxSimTickMs_m->setValue(1000);
        simControlLayout->addWidget(spinBoxSimTickMs_m);

        timer_m = new QTimer(this);
        timer_m->setInterval(spinBoxSimTickMs_m->value());
        connect(timer_m, &QTimer::timeout, this, &MainWindow::doOneStep);

        auto * StateLayout = new QHBoxLayout();
        simControlLayout->addLayout(StateLayout);

        StateLayout->addWidget(new QLabel("State:"));

        spinBoxState_m = new QSpinBox(root);
        spinBoxState_m->setMinimum(-1);          // -1 = halt
        spinBoxState_m->setMaximum(0);           // пока состояний нет
        spinBoxState_m->setSpecialValueText("halt");
        spinBoxState_m->setValue(-1);
        StateLayout->addWidget(spinBoxState_m);

        connect(spinBoxState_m, &QSpinBox::valueChanged, this, [this](int v){
            machine_m.setState(v);
            actions_m->updateHighlight();
            if (machine_m.isHalted() && timer_m->isActive())
                stopSimulation();
        });

        connect(spinBoxState_m, &QSpinBox::valueChanged, this, [this](int v) {
            machine_m.setState(v);
            actions_m->updateHighlight();
        });

        // -- simViewControlLayout
        auto *simViewControlLayout = new QHBoxLayout();
        simLayout->addLayout(simViewControlLayout);

        // View center (то, что у тебя уже есть)
        buttonMoveLeft_m  = new QPushButton("View ◄", root);
        labelCurrPos_m    = new QLabel("0");
        labelCurrPos_m->setAlignment(Qt::AlignCenter);
        labelCurrPos_m->setFixedWidth(60);
        buttonMoveRight_m = new QPushButton("View ►", root);

        simViewControlLayout->addWidget(buttonMoveLeft_m);
        simViewControlLayout->addWidget(labelCurrPos_m);
        simViewControlLayout->addWidget(buttonMoveRight_m);

        simViewControlLayout->addSpacing(20);

        // Head (каретка)
        buttonHeadLeft_m  = new QPushButton("◄ Head", root);
        spinBoxHead_m     = new QSpinBox(root);
        spinBoxHead_m->setRange(-1000000, 1000000);   // практический диапазон
        spinBoxHead_m->setFixedWidth(80);
        buttonHeadRight_m = new QPushButton("Head ►", root);

        simViewControlLayout->addWidget(buttonHeadLeft_m);
        simViewControlLayout->addWidget(spinBoxHead_m);
        simViewControlLayout->addWidget(buttonHeadRight_m);

        simViewControlLayout->addStretch(1);

        connect(buttonMoveLeft_m, &QPushButton::clicked, this, [this]{
            pos -= 1;
            labelCurrPos_m->setNum(pos);
            tape_m->setViewCenter(pos);
        });

        connect(buttonMoveRight_m, &QPushButton::clicked, this, [this]{
            pos += 1;
            labelCurrPos_m->setNum(pos);
            tape_m->setViewCenter(pos);
        });

        connect(buttonHeadLeft_m, &QPushButton::clicked, this, [this]{
            const bool follow = (tape_m->viewCenter() == machine_m.cursor());
            machine_m.setCursor(machine_m.cursor() - 1);
            afterCursorMove(follow);
        });

        connect(buttonHeadRight_m, &QPushButton::clicked, this, [this]{
            const bool follow = (tape_m->viewCenter() == machine_m.cursor());
            machine_m.setCursor(machine_m.cursor() + 1);
            afterCursorMove(follow);
        });

        connect(spinBoxHead_m, &QSpinBox::valueChanged, this, [this](int v){
            if (v == machine_m.cursor()) return;
            const bool follow = (tape_m->viewCenter() == machine_m.cursor());
            machine_m.setCursor(v);
            afterCursorMove(follow);
        });

        // -- simDisplayLayout
        auto * simDisplayLayout = new QHBoxLayout();
        simLayout->addLayout(simDisplayLayout);

        tape_m = new TapeWidget(root);
        tape_m->setViewCenter(pos);           // pos = 0 стартово
        simDisplayLayout->addWidget(tape_m);

        // -- simActionsLayout
        auto *simActionsLayout = new QHBoxLayout();
        simLayout->addLayout(simActionsLayout);

        actions_m = new ActionsTableWidget(root);
        simActionsLayout->addWidget(actions_m, /*stretch*/ 3);

        connect(actions_m, &ActionsTableWidget::stateCountChanged, this, &MainWindow::syncStateSpin);

        machine_m.reset();
        tape_m->setMachine(&machine_m);
        actions_m->setMachine(&machine_m);
        syncStateSpin();
        syncHeadSpin();

        rootLayout->addStretch(1);
    }

    // Сразу пытается загрузить файл
    MainWindow(const QString &path, QWidget *parent)
        : MainWindow(parent)
    {
        stopSimulation();
        openFile(path);
    }

    void afterCursorMove(bool wasAttached) {
        if (wasAttached) {
            pos = machine_m.cursor();
            labelCurrPos_m->setNum(pos);
            tape_m->setViewCenter(pos);   // внутри refresh()
        } else {
            tape_m->refresh();            // перерисовка без сдвига view
        }
        syncHeadSpin();
    }

    void syncStateSpin() {
        spinBoxState_m->blockSignals(true);
        spinBoxState_m->setMaximum(machine_m.stateCount() - 1);
        spinBoxState_m->setValue(machine_m.state());
        spinBoxState_m->blockSignals(false);
    }

    void syncHeadSpin() {
        spinBoxHead_m->blockSignals(true);
        spinBoxHead_m->setValue(machine_m.cursor());
        spinBoxHead_m->blockSignals(false);
    }

    void openFile(const QString &path) {
        try {
            machine_m = TuringQt::TuringMachineQtV(path);
        } catch (const std::exception &e) {
            statusBar()->showMessage(QString("Load failed: %1").arg(e.what()), 5000);
            return;
        }

        // Перецепить все виджеты на новую машину
        tape_m->setMachine(&machine_m);
        actions_m->setMachine(&machine_m);        // внутри rebuild()

        // Синхронизация UI с новым состоянием
        pos = machine_m.cursor();
        labelCurrPos_m->setNum(pos);
        tape_m->setViewCenter(pos);

        syncStateSpin();
        syncHeadSpin();
        actions_m->updateHighlight();

        statusBar()->showMessage(QString("Loaded: %1").arg(path), 3000);
    }

    void saveFile(const QString &path) {
        try {
            machine_m.writeToFile(path);          // поправь опечатку в TuringMachineQtV
        } catch (const std::exception &e) {
            statusBar()->showMessage(QString("Save failed: %1").arg(e.what()), 5000);
            return;
        }
        statusBar()->showMessage(QString("Saved: %1").arg(path), 3000);
    }

    void doOneStep() {
        if (machine_m.isHalted()) {
            statusBar()->showMessage("Halted", 3000);

            return;
        }

        const bool follow = (tape_m->viewCenter() == machine_m.cursor());

        try {
            machine_m.step();
        } catch (const std::exception &e) {
            statusBar()->showMessage(e.what(), 5000);
            stopSimulation();
            return;
        }

        afterCursorMove(follow);
        actions_m->updateHighlight();
        syncStateSpin();

        // Авто-стоп при halt
        if (machine_m.isHalted()) {
            stopSimulation();

            statusBar()->showMessage("Halted", 3000);
        }
    }

    void startSimulation() {
        if (machine_m.isHalted()) return;              // нечего запускать
        timer_m->start(spinBoxSimTickMs_m->value());
        buttonSwitch_m->setText("Stop");
    }

    void stopSimulation() {
        timer_m->stop();
        buttonSwitch_m->setText("Start");
    }

    QPushButton * buttonMakeOneStep_m = nullptr;
    QPushButton * buttonSwitch_m = nullptr;

    QSpinBox * spinBoxSimTickMs_m = nullptr;
    QSpinBox * spinBoxState_m = nullptr;

    QPushButton * buttonMoveLeft_m = nullptr;
    QPushButton * buttonMoveRight_m = nullptr;

    QPushButton * buttonHeadLeft_m = nullptr;
    QPushButton * buttonHeadRight_m = nullptr;

    QSpinBox * spinBoxHead_m = nullptr;

    QLabel * labelCurrPos_m = nullptr;

    int32_t pos = 0;

    TuringQt::TuringMachineQtV machine_m;
    TapeWidget * tape_m = nullptr;

    ActionsTableWidget * actions_m = nullptr;

    QTimer * timer_m = nullptr;
};

