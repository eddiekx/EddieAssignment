#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QRect>
#include <QPen>
#include <QRandomGenerator>
#include <QVector>
#include <QPoint>
#include <QStringList>
#include <QtGlobal>

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
    setFixedSize(1400, 900);

    currentTheme = ThemeClassic;
    gameState = StateMenu;
    selectedMode = ModeClassic;
    gameMode = ModeClassic;

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
    achievementText.clear();
    endMessage.clear();

    achFirstLine = false;
    achCombo3 = false;
    achHold = false;
    achLevel3 = false;
    achSpecial = false;
    achTimedHalf = false;

    // 这里的路径要和你的 resources.qrc 对上
    menuBg.load(":/images/images/menu_bg.png");
    gameBgWarm.load(":/images/images/game_bg_warm.png");
    gameBgCool.load(":/images/images/game_bg_cool.png");
    introImg.load(":/images/images/intro.png");

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::onGameTick);
    gameTimer->start(50);

    startButton = new QPushButton("开始游戏", this);
    pauseButton = new QPushButton("暂停游戏", this);
    continueButton = new QPushButton("继续游戏", this);
    holdButton = new QPushButton("Hold 暂存", this);
    restartButton = new QPushButton("重新开始", this);
    quitButton = new QPushButton("退出游戏", this);

    classicModeButton = new QPushButton(this);
    timedModeButton = new QPushButton(this);
    challengeModeButton = new QPushButton(this);
    specialModeButton = new QPushButton(this);
    obstacleModeButton = new QPushButton(this);

    const QString arcadeBtnStyle =
        "QPushButton{background:#f0dfbb;border:2px solid #8b6b3f;border-radius:8px;padding:6px 10px;color:#3d2b16;font-weight:bold;}"
        "QPushButton:hover{background:#fff3d9;}";

    startButton->setStyleSheet(arcadeBtnStyle);
    pauseButton->setStyleSheet(arcadeBtnStyle);
    continueButton->setStyleSheet(arcadeBtnStyle);
    holdButton->setStyleSheet(arcadeBtnStyle);
    restartButton->setStyleSheet(arcadeBtnStyle);
    quitButton->setStyleSheet(arcadeBtnStyle);

    classicModeButton->setStyleSheet(arcadeBtnStyle);
    timedModeButton->setStyleSheet(arcadeBtnStyle);
    challengeModeButton->setStyleSheet(arcadeBtnStyle);
    specialModeButton->setStyleSheet(arcadeBtnStyle);
    obstacleModeButton->setStyleSheet(arcadeBtnStyle);

    startButton->setGeometry(1080, 165, 170, 36);
    pauseButton->setGeometry(1080, 215, 170, 36);
    continueButton->setGeometry(1080, 215, 170, 36);
    holdButton->setGeometry(1080, 265, 170, 36);
    restartButton->setGeometry(1080, 315, 170, 36);
    quitButton->setGeometry(1080, 365, 170, 36);

    classicModeButton->setGeometry(1080, 420, 170, 34);
    timedModeButton->setGeometry(1080, 460, 170, 34);
    challengeModeButton->setGeometry(1080, 500, 170, 34);
    specialModeButton->setGeometry(1080, 540, 170, 34);
    obstacleModeButton->setGeometry(1080, 580, 170, 34);

    connect(startButton, &QPushButton::clicked, this, [this]() { startGame(selectedMode); });
    connect(pauseButton, &QPushButton::clicked, this, [this]() { pauseGame(); });
    connect(continueButton, &QPushButton::clicked, this, [this]() { continueGame(); });
    connect(holdButton, &QPushButton::clicked, this, [this]() { holdCurrentPiece(); });
    connect(restartButton, &QPushButton::clicked, this, [this]() { restartCurrentMode(); });
    connect(quitButton, &QPushButton::clicked, this, [this]() { close(); });

    connect(classicModeButton, &QPushButton::clicked, this, [this]() { selectMode(ModeClassic); });
    connect(timedModeButton, &QPushButton::clicked, this, [this]() { selectMode(ModeTimed); });
    connect(challengeModeButton, &QPushButton::clicked, this, [this]() { selectMode(ModeChallenge); });
    connect(specialModeButton, &QPushButton::clicked, this, [this]() { selectMode(ModeSpecial); });
    connect(obstacleModeButton, &QPushButton::clicked, this, [this]() { selectMode(ModeObstacle); });

    refreshModeButtonTexts();
    updateButtonState();
    update();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateButtonState()
{
    const bool inMenu = (gameState == StateMenu);
    const bool inPlay = (gameState == StatePlaying);
    const bool inPause = (gameState == StatePaused);
    const bool inOver = (gameState == StateGameOver);

    if (inMenu) {
        startButton->setVisible(false);
        pauseButton->setVisible(false);
        continueButton->setVisible(false);
        holdButton->setVisible(false);
        restartButton->setVisible(false);
        quitButton->setVisible(false);

        classicModeButton->setVisible(false);
        timedModeButton->setVisible(false);
        challengeModeButton->setVisible(false);
        specialModeButton->setVisible(false);
        obstacleModeButton->setVisible(false);
        return;
    }

    startButton->setVisible(inOver);
    pauseButton->setVisible(inPlay);
    continueButton->setVisible(inPause);
    holdButton->setVisible(inPlay);
    restartButton->setVisible(inPlay || inPause || inOver);
    quitButton->setVisible(false);

    classicModeButton->setVisible(true);
    timedModeButton->setVisible(true);
    challengeModeButton->setVisible(true);
    specialModeButton->setVisible(true);
    obstacleModeButton->setVisible(true);
}

void MainWindow::refreshModeButtonTexts()
{
    classicModeButton->setText(selectedMode == ModeClassic ? "▶ 经典模式" : "经典模式");
    timedModeButton->setText(selectedMode == ModeTimed ? "▶ 限时模式" : "限时模式");
    challengeModeButton->setText(selectedMode == ModeChallenge ? "▶ 闯关模式" : "闯关模式");
    specialModeButton->setText(selectedMode == ModeSpecial ? "▶ 特殊方块模式" : "特殊方块模式");
    obstacleModeButton->setText(selectedMode == ModeObstacle ? "▶ 障碍赛模式" : "障碍赛模式");
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
    case ThemeCool: return "冷色主题";
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

QString MainWindow::currentModeName() const
{
    return modeName(gameMode);
}

QPixmap MainWindow::currentGameBackground() const
{
    if (currentTheme == ThemeCool) {
        return gameBgCool;   // 冷色调底图
    }
    return gameBgWarm;       // 经典/像素共用暖色调底图
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
}

void MainWindow::selectMode(GameMode mode)
{
    selectedMode = mode;
    refreshModeButtonTexts();

    if (gameState == StatePlaying || gameState == StatePaused || gameState == StateGameOver) {
        startGame(mode);
    } else {
        update();
    }
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
}

void MainWindow::pauseGame()
{
    if (gameState == StatePlaying) {
        gameState = StatePaused;
        updateButtonState();
        update();
    }
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

void MainWindow::restartCurrentMode()
{
    startGame(gameMode);
}

void MainWindow::generateRandomPiece(Cell piece[4][4])
{
    int shapeIndex = QRandomGenerator::global()->bounded(7);
    int colorIndex = QRandomGenerator::global()->bounded(7);
    int specialType = SpecialNone;

    int roll = QRandomGenerator::global()->bounded(100);
    if (roll < specialSpawnChance()) {
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
    bool anySpecialUsed = false;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!currentPiece[i][j].filled) continue;

            int bx = currentX + j;
            int by = currentY + i;

            if (currentPiece[i][j].special == SpecialBomb) {
                anySpecialUsed = true;
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
                if (bx >= 0 && bx < BOARD_WIDTH) {
                    for (int r = 0; r < BOARD_HEIGHT; ++r) {
                        board[r][bx] = Cell{};
                    }
                }
            }
        }
    }

    if (anySpecialUsed && !achSpecial) {
        achSpecial = true;
        showAchievement("成就解锁：首次使用特殊方块！");
    }

    usedSpecialThisGame = usedSpecialThisGame || anySpecialUsed;
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
        }
        else if (!hasObstacle && filledCount == BOARD_WIDTH - 1 && rainbowCount > 0) {
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
        if (level >= 3 && !achLevel3) {
            achLevel3 = true;
            showAchievement("成就解锁：进入高等级！");
        }

        if (gameMode == ModeTimed) {
            timeLeftSec += 1;
        }
    }
    else {
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
    Cell temp[4][4] = {};

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
    }
    else {
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

    static const QColor coolColors[7] = {
        QColor(255, 70, 70),
        QColor(0, 255, 255),
        QColor(255, 230, 0),
        QColor(160, 80, 255),
        QColor(0, 255, 120),
        QColor(255, 0, 200),
        QColor(255, 140, 0)
    };

    static const QColor pixelColors[7] = {
        QColor(214, 136, 46),
        QColor(97, 144, 196),
        QColor(165, 92, 200),
        QColor(194, 85, 85),
        QColor(87, 170, 102),
        QColor(214, 120, 170),
        QColor(74, 104, 188)
    };

    int idx = qBound(0, colorIndex, 6);

    if (currentTheme == ThemeCool) {
        return coolColors[idx];
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

QColor MainWindow::getGridColor() const
{
    if (currentTheme == ThemeCool) {
        return QColor(80, 200, 255, 110);
    }
    if (currentTheme == ThemePixel) {
        return QColor(120, 100, 70, 115);
    }
    return QColor(80, 80, 80, 55);
}

QColor MainWindow::getBackgroundColor() const
{
    if (currentTheme == ThemeCool) {
        return QColor(8, 8, 12);
    }
    if (currentTheme == ThemePixel) {
        return QColor(236, 227, 208);
    }
    return QColor(245, 247, 250);
}

QColor MainWindow::getTextColor() const
{
    if (currentTheme == ThemeCool) {
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

    if (currentTheme == ThemeCool) {
        QColor glow = baseColor;
        glow.setAlpha(65);

        for (int i = 6; i >= 1; --i) {
            QColor g = glow;
            g.setAlpha(10 * i);
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRect(rect.adjusted(-i, -i, i, i));
        }

        QLinearGradient grad(rect.topLeft(), rect.bottomRight());
        grad.setColorAt(0.0, baseColor.lighter(150));
        grad.setColorAt(1.0, baseColor.darker(115));
        p.setPen(QPen(baseColor.lighter(170), 1));
        p.setBrush(grad);
        p.drawRect(rect.adjusted(0, 0, -1, -1));
    }
    else {
        QLinearGradient grad(rect.topLeft(), rect.bottomRight());
        grad.setColorAt(0.0, baseColor.lighter(145));
        grad.setColorAt(1.0, baseColor.darker(150));
        p.setPen(QPen(baseColor.darker(220), 1));
        p.setBrush(grad);
        p.drawRect(rect.adjusted(0, 0, -1, -1));

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
        if (currentTheme == ThemeCool) {
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

    if (gameState == StateMenu) {
        if (!menuBg.isNull()) {
            p.drawPixmap(rect(), menuBg);
        } else {
            QLinearGradient bg(0, 0, width(), height());
            bg.setColorAt(0.0, QColor(240, 232, 214));
            bg.setColorAt(1.0, QColor(225, 214, 190));
            p.fillRect(rect(), bg);
        }

        p.fillRect(rect(), QColor(255, 250, 240, 30));

        QFont titleFont("Microsoft YaHei", 42, QFont::Bold);
        p.setFont(titleFont);
        QLinearGradient titleGrad(0, 0, 420, 0);
        titleGrad.setColorAt(0, QColor(255, 180, 0));
        titleGrad.setColorAt(0.5, QColor(255, 0, 255));
        titleGrad.setColorAt(1, QColor(0, 255, 255));
        p.setPen(QPen(titleGrad, 2));
        p.drawText(70, 120, "俄罗斯方块");

        QRect menuRect(510, 180, 260, 430);
        p.setBrush(QColor(0, 0, 0, 180));
        p.setPen(QPen(QColor(0, 255, 255), 3));
        p.drawRoundedRect(menuRect, 16, 16);

        QStringList modes = {
            "经典模式",
            "限时模式",
            "闯关模式",
            "特殊方块模式",
            "障碍赛模式"
        };

        QFont menuFont("Microsoft YaHei", 18, QFont::Bold);
        p.setFont(menuFont);

        for (int i = 0; i < modes.size(); ++i) {
            QRect r(535, 225 + i * 58, 210, 40);
            GameMode mode = static_cast<GameMode>(i);
            if (mode == selectedMode) {
                p.setPen(Qt::yellow);
                p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, "▶ " + modes[i]);
            } else {
                p.setPen(Qt::white);
                p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, modes[i]);
            }
        }

        QRect introRect(890, 120, 370, 520);
        p.setBrush(QColor(0, 0, 0, 180));
        p.setPen(QPen(QColor(0, 255, 255), 3));
        p.drawRoundedRect(introRect, 16, 16);

        p.setPen(Qt::white);
        QFont introTitle("Microsoft YaHei", 15, QFont::Bold);
        p.setFont(introTitle);
        p.drawText(960, 170, "—— 游戏说明 ——");

        QFont introText("Microsoft YaHei", 12);
        p.setFont(introText);
        QString intro =
            "移动、旋转、摆放不同形状的方块，\n"
            "在底部完整填满一行即可消除得分！\n\n"
            "支持 Hold 暂存 / 下一方块预览\n"
            "多种模式 / 成就系统 / 障碍挑战";
        p.drawText(QRect(920, 220, 300, 120), Qt::TextWordWrap, intro);

        if (!introImg.isNull()) {
            p.drawPixmap(QRect(940, 345, 230, 180), introImg);
        } else {
            p.setPen(QColor(255, 230, 150));
            p.drawText(QRect(930, 350, 280, 160), Qt::AlignCenter, "intro.png");
        }

        p.setPen(Qt::white);
        QFont bottomFont("Consolas", 16, QFont::Bold);
        p.setFont(bottomFont);
        p.drawText(390, 760, "↑↓ 选择模式   Enter 开始   1经典 2冷色 3像素");

        updateButtonState();
        return;
    }

    // 游戏底图：冷色主题专用，经典/像素用暖色
    const QPixmap bg = currentGameBackground();
    if (!bg.isNull()) {
        p.drawPixmap(rect(), bg);
    } else {
        if (currentTheme == ThemeCool) {
            QLinearGradient g(0, 0, width(), height());
            g.setColorAt(0.0, QColor(10, 10, 14));
            g.setColorAt(1.0, QColor(0, 0, 0));
            p.fillRect(rect(), g);
        } else {
            QLinearGradient g(0, 0, width(), height());
            g.setColorAt(0.0, QColor(248, 240, 220));
            g.setColorAt(1.0, QColor(232, 220, 196));
            p.fillRect(rect(), g);
        }
    }

    if (currentTheme == ThemeCool) {
        p.fillRect(rect(), QColor(0, 0, 0, 55));
    } else {
        p.fillRect(rect(), QColor(255, 245, 225, 25));
    }

    const int boardLeft = 60;
    const int boardTop = 70;
    const int boardWidthPx = BOARD_WIDTH * BLOCK_SIZE;
    const int boardHeightPx = BOARD_HEIGHT * BLOCK_SIZE;

    p.setPen(QPen(getGridColor(), 1));
    if (currentTheme == ThemeCool) {
        p.setBrush(QColor(0, 0, 0, 70));
    } else {
        p.setBrush(QColor(255, 255, 255, 70));
    }
    p.drawRect(boardLeft - 1, boardTop - 1, boardWidthPx + 1, boardHeightPx + 1);

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

    int infoX = 460;
    p.setPen(getTextColor());

    QFont titleFont = p.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(infoX, 95, "俄罗斯方块");

    QFont normalFont = p.font();
    normalFont.setPointSize(10);
    normalFont.setBold(false);
    p.setFont(normalFont);

    p.drawText(infoX, 140, "得分: " + QString::number(score));
    p.drawText(infoX, 170, "等级: " + QString::number(level));
    p.drawText(infoX, 200, "累计消行: " + QString::number(totalLinesCleared));
    p.drawText(infoX, 230, "当前主题: " + themeName());
    p.drawText(infoX, 260, "当前状态: " + stateName());
    p.drawText(infoX, 290, "当前模式: " + currentModeName());

    if (gameMode == ModeTimed) {
        p.drawText(infoX, 320, "剩余时间: " + QString::number(timeLeftSec) + " 秒");
    } else if (gameMode == ModeChallenge || gameMode == ModeObstacle) {
        p.drawText(infoX, 320, "目标行数: " + QString::number(challengeTargetLines()));
    } else {
        p.drawText(infoX, 320, "目标: 尽量拿高分");
    }

    p.drawText(infoX, 350, "连消: " + QString::number(comboStreak));
    p.drawText(infoX, 380, "下落速度: " + QString::number(currentDropIntervalMs()) + " ms");

    QFont sectionFont = p.font();
    sectionFont.setPointSize(10);
    sectionFont.setBold(true);
    p.setFont(sectionFont);
    p.drawText(infoX, 420, "操作说明：");

    QFont smallFont = p.font();
    smallFont.setPointSize(9);
    smallFont.setBold(false);
    p.setFont(smallFont);

    QRect controlRect(infoX, 440, 280, 160);
    p.drawText(controlRect, Qt::TextWordWrap,
               "← → 移动方块\n"
               "↓ 加速下落\n"
               "↑ / 空格 旋转方块\n"
               "H Hold 暂存\n"
               "P 暂停 / 继续\n"
               "R 重新开始\n"
               "1 经典主题\n"
               "2 冷色主题\n"
               "3 像素主题");

    p.setFont(sectionFont);
    p.drawText(infoX, 610, "特殊方块说明：");

    p.setFont(smallFont);
    QRect ruleRect(infoX, 630, 300, 170);
    p.drawText(ruleRect, Qt::TextWordWrap,
               "B 炸弹方块：落地后清除周围 3x3。\n\n"
               "R 彩虹方块：可补齐差 1 格的行。\n\n"
               "C 清除方块：落地后清除当前整列。\n\n"
               "X 障碍：固定，不参与普通消行。");

    drawMiniPiece(p, nextPiece, 930, 120, 22, "下一个方块");

    if (hasHoldPiece) {
        drawMiniPiece(p, holdPiece, 930, 260, 22, "Hold 暂存");
    } else {
        p.setPen(getTextColor());
        p.drawText(930, 252, "Hold 暂存");
        QRect holdBox(930, 260, 88, 88);
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

    QRect overlayRect(boardLeft + 22, boardTop + 175, boardWidthPx - 44, 170);

    if (gameState == StatePaused) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 165));
        p.drawRoundedRect(overlayRect, 18, 18);
        drawOutlinedText(p, overlayRect.adjusted(0, 20, 0, 0), "游戏已暂停", 22, true);
        p.setPen(Qt::white);
        p.drawText(overlayRect.adjusted(0, 78, 0, 0), Qt::AlignCenter, "按 P 继续");
        p.drawText(overlayRect.adjusted(0, 118, 0, 0), Qt::AlignCenter, "按 R 重新开始");
    }
    else if (gameState == StateGameOver) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 165));
        p.drawRoundedRect(overlayRect, 18, 18);
        drawOutlinedText(p, overlayRect.adjusted(0, 20, 0, 0), endMessage.isEmpty() ? "游戏结束" : endMessage, 22, true);
        p.setPen(Qt::white);
        p.drawText(overlayRect.adjusted(0, 78, 0, 0), Qt::AlignCenter, "按 Enter / R 重新开始");
        p.drawText(overlayRect.adjusted(0, 118, 0, 0), Qt::AlignCenter, "按 Esc 返回菜单");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    if (key == Qt::Key_1) {
        currentTheme = ThemeClassic;
        update();
        return;
    }
    if (key == Qt::Key_2) {
        currentTheme = ThemeCool;
        update();
        return;
    }
    if (key == Qt::Key_3) {
        currentTheme = ThemePixel;
        update();
        return;
    }

    if (gameState == StateMenu) {
        if (key == Qt::Key_Up) {
            selectedMode = static_cast<GameMode>((static_cast<int>(selectedMode) + 4) % 5);
            refreshModeButtonTexts();
            update();
            return;
        }
        if (key == Qt::Key_Down) {
            selectedMode = static_cast<GameMode>((static_cast<int>(selectedMode) + 1) % 5);
            refreshModeButtonTexts();
            update();
            return;
        }
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            startGame(selectedMode);
            return;
        }
        if (key == Qt::Key_Escape) {
            close();
            return;
        }
        return;
    }

    if (gameState == StatePaused) {
        if (key == Qt::Key_P || key == Qt::Key_Return || key == Qt::Key_Enter) {
            continueGame();
            return;
        }
        if (key == Qt::Key_R) {
            restartCurrentMode();
            return;
        }
        if (key == Qt::Key_Escape) {
            gameState = StateMenu;
            updateButtonState();
            update();
            return;
        }
        return;
    }

    if (gameState == StateGameOver) {
        if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_R) {
            restartCurrentMode();
            return;
        }
        if (key == Qt::Key_Escape) {
            gameState = StateMenu;
            updateButtonState();
            update();
            return;
        }
        return;
    }

    if (key == Qt::Key_P) {
        pauseGame();
        return;
    }
    if (key == Qt::Key_R) {
        restartCurrentMode();
        return;
    }
    if (key == Qt::Key_H) {
        holdCurrentPiece();
        return;
    }
    if (key == Qt::Key_Escape) {
        gameState = StateMenu;
        updateButtonState();
        update();
        return;
    }

    if (gameState != StatePlaying) {
        return;
    }

    if (key == Qt::Key_Left) {
        if (!checkCollision(currentX - 1, currentY, currentPiece)) {
            currentX--;
            update();
        }
    }
    else if (key == Qt::Key_Right) {
        if (!checkCollision(currentX + 1, currentY, currentPiece)) {
            currentX++;
            update();
        }
    }
    else if (key == Qt::Key_Down) {
        if (movePieceDown(true)) {
            update();
        }
    }
    else if (key == Qt::Key_Up || key == Qt::Key_Space) {
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