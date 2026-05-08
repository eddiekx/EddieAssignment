#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QRect>
#include <QPen>
#include <QVector>
#include <QPair>
#include <QPoint>
#include <QRandomGenerator>
#include <QPushButton>

static const int SHAPES[7][4][4] = {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, // I
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, // O
    {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}}, // T
    {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}}, // J
    {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}, // L
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}}, // S
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}  // Z
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setFocusPolicy(Qt::StrongFocus);
    setWindowTitle("俄罗斯方块 - 升级版");
    setFixedSize(1240, 920);

    score = 0;
    level = 1;
    totalLinesCleared = 0;
    comboStreak = 0;
    timeLeftSec = 0;
    dropAccumMs = 0;
    timeAccumMs = 0;
    achievementTicks = 0;
    hasHoldPiece = false;
    holdUsedThisTurn = false;
    usedSpecialThisGame = false;
    currentTheme = ThemeClassic;
    gameState = StateMenu;
    selectedMode = ModeClassic;
    gameMode = ModeClassic;
    achievementText.clear();
    endMessage.clear();

    achFirstLine = false;
    achCombo3 = false;
    achHold = false;
    achLevel3 = false;
    achSpecial = false;
    achTimedHalf = false;

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::onGameTick);
    gameTimer->start(50);

    startButton = new QPushButton("开始游戏", this);
    pauseButton = new QPushButton("暂停游戏", this);
    continueButton = new QPushButton("继续游戏", this);
    holdButton = new QPushButton("Hold 暂存", this);
    restartButton = new QPushButton("重新开始", this);

    classicModeButton = new QPushButton(this);
    timedModeButton = new QPushButton(this);
    challengeModeButton = new QPushButton(this);
    specialModeButton = new QPushButton(this);
    obstacleModeButton = new QPushButton(this);

    startButton->setGeometry(1005, 165, 170, 36);
    pauseButton->setGeometry(1005, 215, 170, 36);
    continueButton->setGeometry(1005, 215, 170, 36);
    holdButton->setGeometry(1005, 265, 170, 36);
    restartButton->setGeometry(1005, 315, 170, 36);

    classicModeButton->setGeometry(1005, 420, 170, 34);
    timedModeButton->setGeometry(1005, 460, 170, 34);
    challengeModeButton->setGeometry(1005, 500, 170, 34);
    specialModeButton->setGeometry(1005, 540, 170, 34);
    obstacleModeButton->setGeometry(1005, 580, 170, 34);

    connect(startButton, &QPushButton::clicked, this, [this]() { startGame(selectedMode); });
    connect(pauseButton, &QPushButton::clicked, this, [this]() { pauseGame(); });
    connect(continueButton, &QPushButton::clicked, this, [this]() { continueGame(); });
    connect(holdButton, &QPushButton::clicked, this, [this]() { holdCurrentPiece(); });
    connect(restartButton, &QPushButton::clicked, this, [this]() { restartCurrentMode(); });

    connect(classicModeButton, &QPushButton::clicked, this, [this]() {
        selectMode(ModeClassic);
    });
    connect(timedModeButton, &QPushButton::clicked, this, [this]() {
        selectMode(ModeTimed);
    });
    connect(challengeModeButton, &QPushButton::clicked, this, [this]() {
        selectMode(ModeChallenge);
    });
    connect(specialModeButton, &QPushButton::clicked, this, [this]() {
        selectMode(ModeSpecial);
    });
    connect(obstacleModeButton, &QPushButton::clicked, this, [this]() {
        selectMode(ModeObstacle);
    });

    connect(timedModeButton, &QPushButton::clicked, this, [this]() {
        selectedMode = ModeTimed;
        refreshModeButtonTexts();
        update();
    });
    connect(challengeModeButton, &QPushButton::clicked, this, [this]() {
        selectedMode = ModeChallenge;
        refreshModeButtonTexts();
        update();
    });
    connect(specialModeButton, &QPushButton::clicked, this, [this]() {
        selectedMode = ModeSpecial;
        refreshModeButtonTexts();
        update();
    });
    connect(obstacleModeButton, &QPushButton::clicked, this, [this]() {
        selectedMode = ModeObstacle;
        refreshModeButtonTexts();
        update();
    });

    refreshModeButtonTexts();
    updateButtonState();
    update();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetBoard()
{
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            board[i][j] = Cell{};
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            currentPiece[i][j] = Cell{};
            nextPiece[i][j] = Cell{};
            holdPiece[i][j] = Cell{};
        }
    }

    currentX = 0;
    currentY = 0;
}

void MainWindow::setupObstacleField()
{
    const int obs[][2] = {
        {12, 2}, {12, 7},
        {13, 3}, {13, 6},
        {14, 4}, {14, 5},
        {15, 2}, {15, 7},
        {16, 3}, {16, 6}
    };

    for (const auto &p : obs) {
        int r = p[0];
        int c = p[1];
        if (r >= 0 && r < BOARD_HEIGHT && c >= 0 && c < BOARD_WIDTH) {
            board[r][c].filled = true;
            board[r][c].color = 6;
            board[r][c].special = SpecialObstacle;
        }
    }
}

QString MainWindow::themeName() const
{
    switch (currentTheme) {
    case ThemeNeon: return "霓虹主题";
    case ThemePixel: return "像素主题";
    default: return "经典主题";
    }
}

QString MainWindow::stateName() const
{
    switch (gameState) {
    case StateMenu: return "菜单界面";
    case StatePlaying: return "游戏中";
    case StatePaused: return "暂停中";
    case StateGameOver: return "游戏结束";
    default: return "";
    }
}

QString MainWindow::modeName(GameMode mode) const
{
    switch (mode) {
    case ModeTimed: return "限时模式";
    case ModeChallenge: return "闯关模式";
    case ModeSpecial: return "特殊方块模式";
    case ModeObstacle: return "障碍赛模式";
    default: return "经典模式";
    }
}

QString MainWindow::modeDescription(GameMode mode) const
{
    switch (mode) {
    case ModeTimed:
        return "180 秒倒计时，时间与速度双重压力。";
    case ModeChallenge:
        return "消满目标行数即可通关，速度会逐级提升。";
    case ModeSpecial:
        return "炸弹 / 彩虹 / 清除方块出现更频繁。";
    case ModeObstacle:
        return "底部带固定障碍，像拼图一样规划路线。";
    default:
        return "标准俄罗斯方块，适合练手与展示。";
    }
}

QString MainWindow::currentModeName() const
{
    return modeName(gameMode);
}

QString MainWindow::currentModeDescription() const
{
    return modeDescription(gameMode);
}

void MainWindow::refreshModeButtonTexts()
{
    classicModeButton->setText(selectedMode == ModeClassic ? "▶ 经典模式" : "经典模式");
    timedModeButton->setText(selectedMode == ModeTimed ? "▶ 限时模式" : "限时模式");
    challengeModeButton->setText(selectedMode == ModeChallenge ? "▶ 闯关模式" : "闯关模式");
    specialModeButton->setText(selectedMode == ModeSpecial ? "▶ 特殊方块模式" : "特殊方块模式");
    obstacleModeButton->setText(selectedMode == ModeObstacle ? "▶ 障碍赛模式" : "障碍赛模式");
}

void MainWindow::selectMode(GameMode mode)
{
    selectedMode = mode;
    refreshModeButtonTexts();

    // 如果正在游戏中或暂停中，直接切换并重开当前局
    if (gameState == StatePlaying || gameState == StatePaused || gameState == StateGameOver) {
        startGame(mode);
    } else {
        updateButtonState();
        update();
    }
}

void MainWindow::updateButtonState()
{
    const bool inMenu = (gameState == StateMenu);
    const bool inPlay = (gameState == StatePlaying);
    const bool inPause = (gameState == StatePaused);
    const bool inOver = (gameState == StateGameOver);

    startButton->setVisible(inMenu || inOver);
    pauseButton->setVisible(inPlay);
    continueButton->setVisible(inPause);
    holdButton->setVisible(inPlay);
    restartButton->setVisible(inPlay || inPause);

    // 模式按钮始终可见，方便游戏中切换
    classicModeButton->setVisible(true);
    timedModeButton->setVisible(true);
    challengeModeButton->setVisible(true);
    specialModeButton->setVisible(true);
    obstacleModeButton->setVisible(true);

    startButton->raise();
    pauseButton->raise();
    continueButton->raise();
    holdButton->raise();
    restartButton->raise();
    classicModeButton->raise();
    timedModeButton->raise();
    challengeModeButton->raise();
    specialModeButton->raise();
    obstacleModeButton->raise();
}

int MainWindow::specialSpawnChance() const
{
    switch (gameMode) {
    case ModeSpecial: return 30;
    case ModeObstacle: return 15;
    case ModeTimed: return 12;
    case ModeChallenge: return 10;
    default: return 8;
    }
}

int MainWindow::challengeTargetLines() const
{
    switch (gameMode) {
    case ModeChallenge: return 20;
    case ModeObstacle: return 15;
    default: return 0;
    }
}

int MainWindow::currentDropIntervalMs() const
{
    int base = 520;

    switch (gameMode) {
    case ModeTimed: base = 440; break;
    case ModeChallenge: base = 500; break;
    case ModeSpecial: base = 480; break;
    case ModeObstacle: base = 470; break;
    default: base = 520; break;
    }

    int interval = base - (level - 1) * 35;
    if (gameMode == ModeSpecial) interval -= 20;
    if (gameMode == ModeObstacle) interval -= 10;

    return qBound(80, interval, 520);
}

int MainWindow::scoreForLines(int lines) const
{
    switch (lines) {
    case 1: return 100;
    case 2: return 300;
    case 3: return 500;
    case 4: return 800;
    default: return 0;
    }
}

void MainWindow::showAchievement(const QString &message)
{
    achievementText = message;
    achievementTicks = 120;
}

void MainWindow::finishGame(const QString &message)
{
    endMessage = message;
    gameState = StateGameOver;
    updateButtonState();
    update();
    QApplication::beep();
}

void MainWindow::startGame(GameMode mode)
{
    selectedMode = mode;
    gameMode = mode;

    resetBoard();
    if (gameMode == ModeObstacle) {
        setupObstacleField();
    }

    score = 0;
    level = 1;
    totalLinesCleared = 0;
    comboStreak = 0;
    timeLeftSec = (gameMode == ModeTimed) ? 180 : 0;
    dropAccumMs = 0;
    timeAccumMs = 0;
    achievementTicks = 0;
    achievementText.clear();
    endMessage.clear();
    hasHoldPiece = false;
    holdUsedThisTurn = false;
    usedSpecialThisGame = false;
    achFirstLine = false;
    achCombo3 = false;
    achHold = false;
    achLevel3 = false;
    achSpecial = false;
    achTimedHalf = false;
    gameState = StatePlaying;

    generateRandomPiece(nextPiece);
    spawnNextPiece();

    refreshModeButtonTexts();
    updateButtonState();
    setFocus();
    update();

    QApplication::beep();
}

void MainWindow::pauseGame()
{
    if (gameState == StatePlaying) {
        gameState = StatePaused;
        updateButtonState();
        update();
    }
    QApplication::beep();
}

void MainWindow::continueGame()
{
    if (gameState == StatePaused) {
        gameState = StatePlaying;
        updateButtonState();
        setFocus();
        update();
    }
}

void MainWindow::generateRandomPiece(Cell piece[4][4])
{
    int shapeIndex = QRandomGenerator::global()->bounded(7);
    int colorIndex = QRandomGenerator::global()->bounded(7);
    int specialChance = specialSpawnChance();
    int specialType = SpecialNone;

    int roll = QRandomGenerator::global()->bounded(100);
    if (roll < specialChance) {
        int kindRoll = QRandomGenerator::global()->bounded(3);
        if (kindRoll == 0) specialType = SpecialBomb;
        else if (kindRoll == 1) specialType = SpecialRainbow;
        else specialType = SpecialClearColumn;
    }

    QVector<QPoint> filledCells;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            piece[i][j] = Cell{};
            if (SHAPES[shapeIndex][i][j]) {
                piece[i][j].filled = true;
                piece[i][j].color = colorIndex;
                filledCells.append(QPoint(i, j));
            }
        }
    }

    if (specialType != SpecialNone && !filledCells.isEmpty()) {
        int pick = QRandomGenerator::global()->bounded(filledCells.size());
        QPoint pos = filledCells[pick];
        piece[pos.x()][pos.y()].special = specialType;
    }
}

void MainWindow::spawnNextPiece()
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            currentPiece[i][j] = nextPiece[i][j];
        }
    }

    generateRandomPiece(nextPiece);

    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    holdUsedThisTurn = false;

    if (checkCollision(currentX, currentY, currentPiece)) {
        finishGame("游戏结束！");
    }
}

bool MainWindow::checkCollision(int x, int y, Cell piece[4][4]) const
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!piece[i][j].filled) continue;

            int bx = x + j;
            int by = y + i;

            if (bx < 0 || bx >= BOARD_WIDTH || by >= BOARD_HEIGHT) {
                return true;
            }

            if (by >= 0 && board[by][bx].filled) {
                return true;
            }
        }
    }
    return false;
}

void MainWindow::mergePiece()
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!currentPiece[i][j].filled) continue;

            int bx = currentX + j;
            int by = currentY + i;
            if (bx >= 0 && bx < BOARD_WIDTH && by >= 0 && by < BOARD_HEIGHT) {
                board[by][bx] = currentPiece[i][j];
                board[by][bx].filled = true;
            }
        }
    }
}

void MainWindow::applySpecialOnLock()
{
    bool bombTriggered = false;
    bool clearColumnTriggered = false;
    bool anySpecialUsed = false;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!currentPiece[i][j].filled) continue;

            int bx = currentX + j;
            int by = currentY + i;

            if (currentPiece[i][j].special == SpecialBomb) {
                anySpecialUsed = true;
                bombTriggered = true;
                for (int r = by - 1; r <= by + 1; ++r) {
                    for (int c = bx - 1; c <= bx + 1; ++c) {
                        if (r >= 0 && r < BOARD_HEIGHT && c >= 0 && c < BOARD_WIDTH) {
                            board[r][c] = Cell{};
                        }
                    }
                }
            }
            else if (currentPiece[i][j].special == SpecialClearColumn) {
                anySpecialUsed = true;
                clearColumnTriggered = true;
                for (int r = 0; r < BOARD_HEIGHT; ++r) {
                    board[r][bx] = Cell{};
                }
            }
        }
    }

    if (anySpecialUsed && !achSpecial) {
        achSpecial = true;
        showAchievement("成就解锁：首次使用特殊方块！");
    }

    if (bombTriggered) {
        score += 40;
    }
    if (clearColumnTriggered) {
        score += 60;
    }

    usedSpecialThisGame = usedSpecialThisGame || anySpecialUsed;
}

void MainWindow::lockCurrentPiece()
{
    mergePiece();
    applySpecialOnLock();

    int cleared = clearLines();

    if (cleared > 0) {
        comboStreak++;
        totalLinesCleared += cleared;

        score += scoreForLines(cleared);
        score += qMax(0, comboStreak - 1) * 80;

        if (!achFirstLine) {
            achFirstLine = true;
            showAchievement("成就解锁：首次消行！");
        }
        if (comboStreak >= 3 && !achCombo3) {
            achCombo3 = true;
            showAchievement("成就解锁：连续消行 3 次！");
        }
        if (totalLinesCleared >= 10 && level >= 2 && !achLevel3) {
            achLevel3 = true;
            showAchievement("成就解锁：达到高等级！");
        }

        if (gameMode == ModeTimed) {
            timeLeftSec += cleared;
        }

        QApplication::beep();
    } else {
        comboStreak = 0;
    }

    level = 1 + totalLinesCleared / 10;

    if (gameMode == ModeTimed && timeLeftSec <= 0) {
        finishGame("时间到！");
        return;
    }

    if ((gameMode == ModeChallenge || gameMode == ModeObstacle) &&
        challengeTargetLines() > 0 &&
        totalLinesCleared >= challengeTargetLines()) {
        finishGame("挑战成功！");
        return;
    }

    spawnNextPiece();
    update();

    if (gameState == StateGameOver) {
        return;
    }
}

bool MainWindow::movePieceDown(bool softDrop)
{
    if (gameState != StatePlaying) {
        return false;
    }

    if (!checkCollision(currentX, currentY + 1, currentPiece)) {
        currentY++;
        if (softDrop) {
            score += 1;
        }
        return true;
    }

    lockCurrentPiece();
    return false;
}

void MainWindow::rotatePiece()
{
    Cell temp[4][4];

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i][j] = Cell{};
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[j][3 - i] = currentPiece[i][j];
        }
    }

    if (!checkCollision(currentX, currentY, temp)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                currentPiece[i][j] = temp[i][j];
            }
        }
    }
}

int MainWindow::clearLines()
{
    int cleared = 0;

    for (int i = BOARD_HEIGHT - 1; i >= 0; --i) {
        int filledCount = 0;
        int rainbowCount = 0;
        bool hasObstacle = false;

        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j].filled) {
                filledCount++;
                if (board[i][j].special == SpecialRainbow) {
                    rainbowCount++;
                }
                if (board[i][j].special == SpecialObstacle) {
                    hasObstacle = true;
                }
            }
        }

        bool canClear = false;
        if (!hasObstacle && filledCount == BOARD_WIDTH) {
            canClear = true;
        } else if (!hasObstacle && filledCount == BOARD_WIDTH - 1 && rainbowCount > 0) {
            canClear = true;
        }

        if (canClear) {
            cleared++;

            for (int k = i; k > 0; --k) {
                for (int j = 0; j < BOARD_WIDTH; ++j) {
                    board[k][j] = board[k - 1][j];
                }
            }

            for (int j = 0; j < BOARD_WIDTH; ++j) {
                board[0][j] = Cell{};
            }

            i++;
        }
    }

    return cleared;
}

void MainWindow::holdCurrentPiece()
{
    if (gameState != StatePlaying || holdUsedThisTurn) {
        return;
    }

    Cell oldCurrent[4][4];
    Cell oldHold[4][4];
    bool oldHasHold = hasHoldPiece;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            oldCurrent[i][j] = currentPiece[i][j];
            oldHold[i][j] = holdPiece[i][j];
        }
    }

    if (!hasHoldPiece) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                holdPiece[i][j] = currentPiece[i][j];
                currentPiece[i][j] = nextPiece[i][j];
            }
        }
        generateRandomPiece(nextPiece);
        currentX = BOARD_WIDTH / 2 - 2;
        currentY = 0;
        hasHoldPiece = true;
    } else {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                currentPiece[i][j] = holdPiece[i][j];
                holdPiece[i][j] = oldCurrent[i][j];
            }
        }
        currentX = BOARD_WIDTH / 2 - 2;
        currentY = 0;

        if (checkCollision(currentX, currentY, currentPiece)) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    currentPiece[i][j] = oldCurrent[i][j];
                    holdPiece[i][j] = oldHold[i][j];
                }
            }
            hasHoldPiece = oldHasHold;
            return;
        }
    }

    holdUsedThisTurn = true;

    if (!achHold) {
        achHold = true;
        showAchievement("成就解锁：首次使用 Hold 暂存！");
    }

    update();
}

void MainWindow::restartCurrentMode()
{
    startGame(gameMode);
}



QColor MainWindow::getThemeBlockColor(int colorIndex) const
{
    static const QColor classicColors[7] = {
        QColor(240, 90, 90),
        QColor(90, 200, 255),
        QColor(255, 190, 80),
        QColor(140, 110, 255),
        QColor(100, 220, 140),
        QColor(255, 120, 220),
        QColor(255, 210, 90)
    };

    static const QColor neonColors[7] = {
        QColor(255, 70, 70),
        QColor(0, 255, 255),
        QColor(255, 230, 0),
        QColor(160, 80, 255),
        QColor(0, 255, 120),
        QColor(255, 0, 200),
        QColor(255, 140, 0)
    };

    static const QColor pixelColors[7] = {
        QColor(220, 80, 80),
        QColor(70, 160, 220),
        QColor(220, 180, 70),
        QColor(140, 100, 220),
        QColor(80, 180, 120),
        QColor(220, 100, 180),
        QColor(220, 160, 60)
    };

    int idx = qBound(0, colorIndex, 6);

    if (currentTheme == ThemeNeon) {
        return neonColors[idx];
    }
    if (currentTheme == ThemePixel) {
        return pixelColors[idx];
    }
    return classicColors[idx];
}

QColor MainWindow::getRainbowColor(int step) const
{
    static const QColor rainbow[7] = {
        QColor(255, 80, 80),
        QColor(255, 170, 60),
        QColor(255, 240, 80),
        QColor(90, 230, 120),
        QColor(80, 190, 255),
        QColor(140, 110, 255),
        QColor(255, 100, 220)
    };
    return rainbow[step % 7];
}

QColor MainWindow::getBackgroundColor() const
{
    if (currentTheme == ThemeNeon) {
        return QColor(8, 8, 12);
    }
    if (currentTheme == ThemePixel) {
        return QColor(236, 227, 208);   // 复古米黄色，更像老游戏
    }
    return QColor(245, 247, 250);       // 经典主题保持干净柔和
}

QColor MainWindow::getGridColor() const
{
    if (currentTheme == ThemeNeon) {
        return QColor(80, 200, 255, 110);
    }
    if (currentTheme == ThemePixel) {
        return QColor(120, 100, 70, 115); // 像素风更暖、更粗
    }
    return QColor(80, 80, 80, 55);
}

QColor MainWindow::getTextColor() const
{
    if (currentTheme == ThemeNeon) {
        return QColor(220, 250, 255);
    }
    if (currentTheme == ThemePixel) {
        return QColor(40, 40, 40);
    }
    return QColor(30, 30, 30);
}

void MainWindow::drawBlock(QPainter &p, int x, int y, const Cell &cell)
{
    QRect rect(x, y, BLOCK_SIZE, BLOCK_SIZE);

    if (!cell.filled) {
        return;
    }

    QColor baseColor;
    if (cell.special == SpecialRainbow) {
        int step = (x / BLOCK_SIZE + y / BLOCK_SIZE) % 7;
        baseColor = getRainbowColor(step);
    } else if (cell.special == SpecialObstacle) {
        baseColor = QColor(110, 110, 110);
    } else {
        baseColor = getThemeBlockColor(cell.color);
    }

    if (currentTheme == ThemeClassic) {
        QLinearGradient grad(rect.topLeft(), rect.bottomRight());
        grad.setColorAt(0.0, baseColor.lighter(145));
        grad.setColorAt(1.0, baseColor.darker(150));
        p.setPen(QPen(baseColor.darker(220), 1));
        p.setBrush(grad);
        p.drawRect(rect.adjusted(0, 0, -1, -1));
    }
    else if (currentTheme == ThemeNeon) {
        QColor glow = baseColor;
        glow.setAlpha(70);

        for (int i = 6; i >= 1; --i) {
            QColor g = glow;
            g.setAlpha(12 * i);
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRect(rect.adjusted(-i, -i, i, i));
        }

        QLinearGradient grad(rect.topLeft(), rect.bottomRight());
        grad.setColorAt(0.0, baseColor.lighter(150));
        grad.setColorAt(1.0, baseColor.darker(120));
        p.setPen(QPen(baseColor.lighter(180), 1));
        p.setBrush(grad);
        p.drawRect(rect.adjusted(0, 0, -1, -1));
    }
    else {
        // 像素主题：硬边、无渐变、复古方块感更强
        p.setPen(QPen(baseColor.darker(220), 2));
        p.setBrush(baseColor);
        p.drawRect(rect.adjusted(0, 0, -1, -1));

        // 做一个简单的“像素高光”，让它和经典主题差异更大
        QRect topBand(rect.left() + 2, rect.top() + 2, rect.width() - 4, 5);
        QRect leftBand(rect.left() + 2, rect.top() + 2, 5, rect.height() - 4);
        p.setPen(Qt::NoPen);
        p.setBrush(baseColor.lighter(130));
        p.drawRect(topBand);
        p.drawRect(leftBand);

        QRect shadowBand(rect.left() + 4, rect.bottom() - 6, rect.width() - 8, 4);
        p.setBrush(baseColor.darker(130));
        p.drawRect(shadowBand);
    }

    if (cell.special != SpecialNone) {
        if (currentTheme == ThemeNeon) {
            p.setPen(QPen(QColor(255, 255, 255), 3));
        } else {
            p.setPen(QPen(QColor(30, 30, 30), 2));
        }
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect.adjusted(2, 2, -3, -3));
    }

    if (cell.special == SpecialBomb) {
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(QColor(40, 40, 40));
        p.drawEllipse(rect.adjusted(5, 5, -5, -5));

        p.setBrush(QColor(220, 40, 40));
        p.drawEllipse(rect.center().x() - 4, rect.center().y() - 4, 8, 8);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(10);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(rect, Qt::AlignCenter, "B");
    }
    else if (cell.special == SpecialRainbow) {
        for (int k = 0; k < 5; ++k) {
            QColor c = getRainbowColor(k);
            p.setPen(QPen(c, 2));
            p.drawLine(rect.left() + 4, rect.top() + 4 + k * 5,
                       rect.right() - 4, rect.top() + 10 + k * 5);
        }

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(10);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(rect, Qt::AlignCenter, "R");
    }
    else if (cell.special == SpecialClearColumn) {
        p.setPen(QPen(Qt::white, 4));
        p.drawLine(rect.center().x(), rect.top() + 4, rect.center().x(), rect.bottom() - 4);

        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(10);
        p.setFont(f);
        p.setPen(Qt::black);
        p.drawText(rect, Qt::AlignCenter, "C");
    }
    else if (cell.special == SpecialObstacle) {
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(Qt::NoBrush);
        p.drawLine(rect.left() + 4, rect.top() + 4, rect.right() - 4, rect.bottom() - 4);
        p.drawLine(rect.left() + 4, rect.bottom() - 4, rect.right() - 4, rect.top() + 4);
    }
}

void MainWindow::drawMiniPiece(QPainter &p, const Cell piece[4][4], int left, int top, int cellSize, const QString &label)
{
    p.setPen(getTextColor());
    QFont f = p.font();
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(left, top - 8, label);

    QRect frame(left, top, cellSize * 4, cellSize * 4);
    p.setPen(QPen(getGridColor(), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(frame);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!piece[i][j].filled) continue;

            QRect r(left + j * cellSize, top + i * cellSize, cellSize, cellSize);
            QColor c = piece[i][j].special == SpecialRainbow ? getRainbowColor(i + j) :
                           piece[i][j].special == SpecialObstacle ? QColor(110, 110, 110) :
                           getThemeBlockColor(piece[i][j].color);

            QLinearGradient grad(r.topLeft(), r.bottomRight());
            grad.setColorAt(0.0, c.lighter(140));
            grad.setColorAt(1.0, c.darker(150));
            p.setPen(QPen(c.darker(220), 1));
            p.setBrush(grad);
            p.drawRect(r.adjusted(0, 0, -1, -1));

            if (piece[i][j].special == SpecialBomb) {
                p.setPen(QPen(Qt::white, 2));
                p.drawEllipse(r.adjusted(4, 4, -4, -4));
            }
            else if (piece[i][j].special == SpecialClearColumn) {
                p.setPen(QPen(Qt::white, 3));
                p.drawLine(r.center().x(), r.top() + 3, r.center().x(), r.bottom() - 3);
            }
            else if (piece[i][j].special == SpecialRainbow) {
                p.setPen(QPen(Qt::white, 2));
                p.drawText(r, Qt::AlignCenter, "R");
            }
        }
    }
}

void MainWindow::drawOutlinedText(QPainter &p, const QRect &rect, const QString &text, int fontSize, bool bold)
{
    QFont f = p.font();
    f.setPointSize(fontSize);
    f.setBold(bold);
    p.setFont(f);

    p.setPen(QColor(0, 0, 0, 180));
    p.drawText(rect.adjusted(2, 2, 2, 2), Qt::AlignCenter | Qt::TextWordWrap, text);

    p.setPen(Qt::white);
    p.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, currentTheme != ThemePixel);

    if (currentTheme == ThemeNeon) {
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(10, 10, 14));
        bg.setColorAt(1.0, QColor(0, 0, 0));
        p.fillRect(rect(), bg);
    } else if (currentTheme == ThemePixel) {
        p.fillRect(rect(), getBackgroundColor());
    } else {
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(255, 255, 255));
        bg.setColorAt(1.0, QColor(230, 236, 245));
        p.fillRect(rect(), bg);
    }

    const int boardLeft = 45;
    const int boardTop = 60;
    const int boardWidthPx = BOARD_WIDTH * BLOCK_SIZE;
    const int boardHeightPx = BOARD_HEIGHT * BLOCK_SIZE;

    if (currentTheme == ThemeNeon) {
        p.setPen(QPen(QColor(0, 255, 255, 170), 2));
        p.setBrush(QColor(0, 0, 0, 70));
        p.drawRect(boardLeft - 1, boardTop - 1, boardWidthPx + 1, boardHeightPx + 1);
    } else {
        p.setPen(QPen(QColor(90, 90, 90, 150), 1));
        p.setBrush(QColor(255, 255, 255, 80));
        p.drawRect(boardLeft - 1, boardTop - 1, boardWidthPx + 1, boardHeightPx + 1);
    }

    p.setPen(QPen(getGridColor(), 1));
    for (int col = 0; col <= BOARD_WIDTH; ++col) {
        int x = boardLeft + col * BLOCK_SIZE;
        p.drawLine(x, boardTop, x, boardTop + boardHeightPx);
    }
    for (int row = 0; row <= BOARD_HEIGHT; ++row) {
        int y = boardTop + row * BLOCK_SIZE;
        p.drawLine(boardLeft, y, boardLeft + boardWidthPx, y);
    }

    p.setFont(QFont("", 9));
    p.setPen(getTextColor());
    for (int col = 0; col < BOARD_WIDTH; ++col) {
        QRect r(boardLeft + col * BLOCK_SIZE, boardTop - 24, BLOCK_SIZE, 18);
        p.drawText(r, Qt::AlignCenter, QString::number(col));
    }
    for (int row = 0; row < BOARD_HEIGHT; ++row) {
        QRect r(boardLeft - 28, boardTop + row * BLOCK_SIZE, 20, BLOCK_SIZE);
        p.drawText(r, Qt::AlignCenter, QString::number(row));
    }

    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j].filled) {
                int x = boardLeft + j * BLOCK_SIZE;
                int y = boardTop + i * BLOCK_SIZE;
                drawBlock(p, x, y, board[i][j]);
            }
        }
    }

    if (gameState == StatePlaying || gameState == StatePaused) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (currentPiece[i][j].filled) {
                    int x = boardLeft + (currentX + j) * BLOCK_SIZE;
                    int y = boardTop + (currentY + i) * BLOCK_SIZE;
                    drawBlock(p, x, y, currentPiece[i][j]);
                }
            }
        }
    }

    int infoX = 385;
    p.setPen(getTextColor());

    QFont titleFont = p.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(infoX, 85, "俄罗斯方块");

    QFont normalFont = p.font();
    normalFont.setPointSize(10);
    normalFont.setBold(false);
    p.setFont(normalFont);

    p.drawText(infoX, 130, "得分: " + QString::number(score));
    p.drawText(infoX, 160, "等级: " + QString::number(level));
    p.drawText(infoX, 190, "累计消行: " + QString::number(totalLinesCleared));
    p.drawText(infoX, 220, "当前主题: " + themeName());
    p.drawText(infoX, 250, "当前状态: " + stateName());
    p.drawText(infoX, 280, "当前模式: " + currentModeName());

    if (gameMode == ModeTimed) {
        p.drawText(infoX, 310, "剩余时间: " + QString::number(timeLeftSec) + " 秒");
    } else if (gameMode == ModeChallenge || gameMode == ModeObstacle) {
        p.drawText(infoX, 310, "目标行数: " + QString::number(challengeTargetLines()));
    } else {
        p.drawText(infoX, 310, "目标: 尽量拿高分");
    }

    p.drawText(infoX, 340, "连消: " + QString::number(comboStreak));
    p.drawText(infoX, 370, "下落速度: " + QString::number(currentDropIntervalMs()) + " ms");

    QFont sectionFont = p.font();
    sectionFont.setPointSize(10);
    sectionFont.setBold(true);
    p.setFont(sectionFont);
    p.drawText(infoX, 410, "操作说明：");

    QFont smallFont = p.font();
    smallFont.setPointSize(9);
    smallFont.setBold(false);
    p.setFont(smallFont);

    QRect controlRect(infoX, 430, 250, 150);
    p.drawText(controlRect, Qt::TextWordWrap,
               "← → 移动方块\n"
               "↓ 加速下落\n"
               "↑ / 空格 旋转\n"
               "H Hold 暂存\n"
               "P 暂停 / 继续\n"
               "R 重新开始\n"
               "1 经典主题\n"
               "2 霓虹主题\n"
               "3 像素主题");

    p.setFont(sectionFont);
    p.drawText(infoX, 595, "特殊方块说明：");

    p.setFont(smallFont);
    QRect ruleRect(infoX, 615, 250, 165);
    p.drawText(ruleRect, Qt::TextWordWrap,
               "B 炸弹方块：落地后立刻清除周围 3x3。\n\n"
               "R 彩虹方块：可当“补位工具”，帮助差 1 格的行消除。\n\n"
               "C 清除方块：落地后直接清除当前整列。\n\n"
               "X 障碍：固定障碍，不参与普通消行。");

    drawMiniPiece(p, nextPiece, 760, 120, 22, "下一个方块");
    if (hasHoldPiece) {
        drawMiniPiece(p, holdPiece, 760, 260, 22, "Hold 暂存");
    } else {
        p.setPen(getTextColor());
        p.drawText(760, 252, "Hold 暂存");
        QRect holdBox(760, 260, 88, 88);
        p.setPen(QPen(getGridColor(), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(holdBox);
        p.drawText(holdBox, Qt::AlignCenter, "空");
    }

    if (achievementTicks > 0 && !achievementText.isEmpty()) {
        QRect achRect(boardLeft + 20, boardTop + 160, boardWidthPx - 40, 70);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 160));
        p.drawRoundedRect(achRect, 16, 16);
        drawOutlinedText(p, achRect, achievementText, 15, true);
    }

    QRect overlayRect(boardLeft + 20, boardTop + 175, boardWidthPx - 40, 170);

    if (gameState == StateMenu) {
        QRect overlayRect(boardLeft + 28, boardTop + 180, boardWidthPx - 56, 145);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 145));
        p.drawRoundedRect(overlayRect, 18, 18);

        drawOutlinedText(p, overlayRect.adjusted(0, 14, 0, 0), "欢迎来到俄罗斯方块", 16, true);

        p.setPen(Qt::white);
        QFont menuFont = p.font();
        menuFont.setPointSize(10);
        p.setFont(menuFont);

        p.drawText(overlayRect.adjusted(18, 58, -18, 0), Qt::AlignCenter | Qt::TextWordWrap,
                   "点击右侧先选择模式，再点“开始游戏”进入。\n"
                   "支持 Hold 暂存 / 下一个方块预览 / 成就提醒 / 障碍赛。");
    }
    else if (gameState == StatePaused) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 165));
        p.drawRoundedRect(overlayRect, 18, 18);

        drawOutlinedText(p, overlayRect.adjusted(0, 20, 0, 0), "游戏已暂停", 22, true);
        p.setPen(Qt::white);
        p.drawText(overlayRect.adjusted(0, 74, 0, 0), Qt::AlignCenter, "点击“继续游戏”恢复");
        p.drawText(overlayRect.adjusted(0, 112, 0, 0), Qt::AlignCenter, "按 P 也可以暂停 / 继续");
    }
    else if (gameState == StateGameOver) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 155));
        p.drawRoundedRect(overlayRect, 18, 18);

        drawOutlinedText(p, overlayRect.adjusted(0, 25, 0, 0), endMessage.isEmpty() ? "游戏结束" : endMessage, 18, true);
        p.drawText(overlayRect.adjusted(0, 74, 0, 0), Qt::AlignCenter, "点击“开始游戏”重新开始");
    }

    if (gameState == StateMenu) {
        p.setPen(getTextColor());
        p.drawText(760, 418, "已选模式：");
        p.drawText(760, 438, modeName(selectedMode));
        QRect descRect(760, 460, 220, 90);
        p.drawText(descRect, Qt::TextWordWrap, modeDescription(selectedMode));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_1) {
        currentTheme = ThemeClassic;
        update();
        return;
    }
    if (event->key() == Qt::Key_2) {
        currentTheme = ThemeNeon;
        update();
        return;
    }
    if (event->key() == Qt::Key_3) {
        currentTheme = ThemePixel;
        update();
        return;
    }
    if (event->key() == Qt::Key_R) {
        restartCurrentMode();
        return;
    }
    if (event->key() == Qt::Key_P) {
        if (gameState == StatePaused) {
            continueGame();
        } else if (gameState == StatePlaying) {
            pauseGame();
        }
        return;
    }
    if (event->key() == Qt::Key_H) {
        holdCurrentPiece();
        return;
    }

    if (gameState != StatePlaying) {
        return;
    }

    if (event->key() == Qt::Key_Left) {
        if (!checkCollision(currentX - 1, currentY, currentPiece)) {
            currentX--;
            update();
        }
    }
    else if (event->key() == Qt::Key_Right) {
        if (!checkCollision(currentX + 1, currentY, currentPiece)) {
            currentX++;
            update();
        }
    }
    else if (event->key() == Qt::Key_Down) {
        if (movePieceDown(true)) {
            update();
        }
    }
    else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Space) {
        rotatePiece();
        update();
    }
}

void MainWindow::onGameTick()
{
    if (gameState != StatePlaying) {
        return;
    }

    if (achievementTicks > 0) {
        achievementTicks--;
        if (achievementTicks == 0) {
            achievementText.clear();
        }
    }

    dropAccumMs += 50;
    timeAccumMs += 50;

    if (gameMode == ModeTimed) {
        while (timeAccumMs >= 1000) {
            timeAccumMs -= 1000;
            timeLeftSec--;

            if (timeLeftSec <= 0) {
                finishGame("时间到！");
                return;
            }

            if (timeLeftSec <= 90 && !achTimedHalf) {
                achTimedHalf = true;
                showAchievement("成就提示：时间过半！");
            }
        }
    }

    if (dropAccumMs >= currentDropIntervalMs()) {
        dropAccumMs = 0;
        movePieceDown(false);
    }

    update();
}