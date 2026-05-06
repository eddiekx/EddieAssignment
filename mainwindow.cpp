#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPainter>
#include <QRandomGenerator>
#include <QLinearGradient>
#include <QFont>
#include <QRect>
#include <QPen>

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
    setFixedSize(820, 900);

    timerId = 0;
    score = 0;
    currentTheme = ThemeClassic;
    gameState = StateMenu;

    startButton = new QPushButton("开始游戏", this);
    pauseButton = new QPushButton("暂停游戏", this);
    continueButton = new QPushButton("继续游戏", this);

    startButton->setGeometry(600, 760, 160, 40);
    pauseButton->setGeometry(600, 815, 160, 40);
    continueButton->setGeometry(600, 815, 160, 40);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::startGame);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::pauseGame);
    connect(continueButton, &QPushButton::clicked, this, &MainWindow::continueGame);

    initGame();
    updateButtonState();
    update();
}

MainWindow::~MainWindow()
{
    if (timerId != 0) {
        killTimer(timerId);
    }
    delete ui;
}

void MainWindow::initGame()
{
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            board[i][j] = {false, 0, SpecialNone};
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            currentPiece[i][j] = {false, 0, SpecialNone};
        }
    }

    currentX = 0;
    currentY = 0;
    score = 0;
}

void MainWindow::startGame()
{
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }

    initGame();
    gameState = StatePlaying;
    generatePiece();

    if (gameState == StatePlaying) {
        timerId = startTimer(500);
    }

    updateButtonState();
    setFocus();
    update();
}

void MainWindow::pauseGame()
{
    if (gameState == StatePlaying && timerId != 0) {
        killTimer(timerId);
        timerId = 0;
        gameState = StatePaused;
        updateButtonState();
        setFocus();
        update();
    }
}

void MainWindow::continueGame()
{
    if (gameState == StatePaused) {
        timerId = startTimer(500);
        gameState = StatePlaying;
        updateButtonState();
        setFocus();
        update();
    }
}

void MainWindow::updateButtonState()
{
    if (gameState == StateMenu || gameState == StateGameOver) {
        startButton->show();
        pauseButton->hide();
        continueButton->hide();
    } else if (gameState == StatePlaying) {
        startButton->hide();
        pauseButton->show();
        continueButton->hide();
    } else if (gameState == StatePaused) {
        startButton->hide();
        pauseButton->hide();
        continueButton->show();
    }

    startButton->raise();
    pauseButton->raise();
    continueButton->raise();
}

void MainWindow::generatePiece()
{
    int shapeIndex = QRandomGenerator::global()->bounded(7);
    int colorIndex = QRandomGenerator::global()->bounded(7);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            currentPiece[i][j] = {false, colorIndex, SpecialNone};
            if (SHAPES[shapeIndex][i][j]) {
                currentPiece[i][j].filled = true;
            }
        }
    }

    int specialRoll = QRandomGenerator::global()->bounded(100);
    int specialType = SpecialNone;

    if (specialRoll < 10) {
        specialType = SpecialBomb;
    } else if (specialRoll < 20) {
        specialType = SpecialRainbow;
    } else if (specialRoll < 30) {
        specialType = SpecialClearColumn;
    }

    if (specialType != SpecialNone) {
        QVector<QPair<int, int>> cells;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (currentPiece[i][j].filled) {
                    cells.append(qMakePair(i, j));
                }
            }
        }

        if (!cells.isEmpty()) {
            int pick = QRandomGenerator::global()->bounded(cells.size());
            int r = cells[pick].first;
            int c = cells[pick].second;
            currentPiece[r][c].special = specialType;
        }
    }

    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;

    if (checkCollision(currentX, currentY, currentPiece)) {
        if (timerId != 0) {
            killTimer(timerId);
            timerId = 0;
        }
        gameState = StateGameOver;
        updateButtonState();
        update();
    }
}

bool MainWindow::checkCollision(int x, int y, Cell piece[4][4])
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!piece[i][j].filled) {
                continue;
            }

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

void MainWindow::clearColumn(int col)
{
    if (col < 0 || col >= BOARD_WIDTH) {
        return;
    }

    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        board[i][col] = {false, 0, SpecialNone};
    }
}

void MainWindow::clearArea(int centerX, int centerY, int radius)
{
    for (int i = centerY - radius; i <= centerY + radius; ++i) {
        for (int j = centerX - radius; j <= centerX + radius; ++j) {
            if (i >= 0 && i < BOARD_HEIGHT && j >= 0 && j < BOARD_WIDTH) {
                board[i][j] = {false, 0, SpecialNone};
            }
        }
    }
}

void MainWindow::applySpecialOnLock()
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (currentPiece[i][j].filled) {
                int bx = currentX + j;
                int by = currentY + i;

                if (currentPiece[i][j].special == SpecialBomb) {
                    clearArea(bx, by, 1);
                } else if (currentPiece[i][j].special == SpecialClearColumn) {
                    clearColumn(bx);
                }
            }
        }
    }
}

void MainWindow::mergePiece()
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (currentPiece[i][j].filled) {
                int bx = currentX + j;
                int by = currentY + i;
                if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH) {
                    board[by][bx] = currentPiece[i][j];
                    board[by][bx].filled = true;
                }
            }
        }
    }

    applySpecialOnLock();
}

void MainWindow::clearLines()
{
    for (int i = BOARD_HEIGHT - 1; i >= 0; --i) {
        int filledCount = 0;
        int rainbowCount = 0;

        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j].filled) {
                filledCount++;
                if (board[i][j].special == SpecialRainbow) {
                    rainbowCount++;
                }
            }
        }

        bool canClear = false;
        if (filledCount == BOARD_WIDTH) {
            canClear = true;
        } else if (filledCount == BOARD_WIDTH - 1 && rainbowCount > 0) {
            canClear = true;
        }

        if (canClear) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                if (board[i][j].filled && board[i][j].special == SpecialBomb) {
                    clearArea(j, i, 1);
                }
            }

            for (int k = i; k > 0; --k) {
                for (int j = 0; j < BOARD_WIDTH; ++j) {
                    board[k][j] = board[k - 1][j];
                }
            }

            for (int j = 0; j < BOARD_WIDTH; ++j) {
                board[0][j] = {false, 0, SpecialNone};
            }

            score += 100;
            i++;
        }
    }
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

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_P) {
        if (gameState == StatePaused) {
            continueGame();
        } else if (gameState == StatePlaying) {
            pauseGame();
        }
        return;
    }

    if (event->key() == Qt::Key_R) {
        startGame();
        return;
    }

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

    if (gameState != StatePlaying) {
        return;
    }

    if (event->key() == Qt::Key_Left) {
        if (!checkCollision(currentX - 1, currentY, currentPiece)) {
            currentX--;
        }
    }
    else if (event->key() == Qt::Key_Right) {
        if (!checkCollision(currentX + 1, currentY, currentPiece)) {
            currentX++;
        }
    }
    else if (event->key() == Qt::Key_Down) {
        if (!checkCollision(currentX, currentY + 1, currentPiece)) {
            currentY++;
        }
    }
    else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Space) {
        rotatePiece();
    }

    update();
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (gameState != StatePlaying) {
        return;
    }

    if (event->timerId() == timerId) {
        if (!checkCollision(currentX, currentY + 1, currentPiece)) {
            currentY++;
        } else {
            mergePiece();
            clearLines();
            generatePiece();
        }
        update();
    }
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
    } else if (currentTheme == ThemePixel) {
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
    if (currentTheme == ThemeNeon) {
        return QColor(80, 200, 255, 80);
    }
    if (currentTheme == ThemePixel) {
        return QColor(120, 120, 120, 90);
    }
    return QColor(80, 80, 80, 60);
}

QColor MainWindow::getBackgroundColor() const
{
    if (currentTheme == ThemeNeon) {
        return QColor(10, 10, 14);
    }
    if (currentTheme == ThemePixel) {
        return QColor(235, 235, 230);
    }
    return QColor(245, 247, 250);
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

QString MainWindow::getThemeName() const
{
    if (currentTheme == ThemeNeon) return "霓虹主题";
    if (currentTheme == ThemePixel) return "像素主题";
    return "经典主题";
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
    } else {
        baseColor = getThemeBlockColor(cell.color);
    }

    if (currentTheme == ThemeClassic) {
        QLinearGradient grad(rect.topLeft(), rect.bottomRight());
        grad.setColorAt(0.0, baseColor.lighter(140));
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
        p.setPen(QPen(baseColor.darker(180), 1));
        p.setBrush(baseColor);
        p.drawRect(rect.adjusted(0, 0, -1, -1));

        p.setPen(QPen(baseColor.lighter(150), 1));
        p.drawLine(rect.topLeft(), rect.topRight());
        p.drawLine(rect.topLeft(), rect.bottomLeft());

        p.setPen(QPen(baseColor.darker(170), 1));
        p.drawLine(rect.bottomLeft(), rect.bottomRight());
        p.drawLine(rect.topRight(), rect.bottomRight());

        QRect inner = rect.adjusted(6, 6, -6, -6);
        if (inner.width() > 0 && inner.height() > 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(baseColor.lighter(115));
            p.drawRect(inner);
        }
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
}

void MainWindow::drawCoordinateLabels(QPainter &p, int boardLeft, int boardTop) const
{
    QFont f = p.font();
    f.setPointSize(9);
    p.setFont(f);

    p.setPen(getTextColor());

    for (int col = 0; col < BOARD_WIDTH; ++col) {
        QRect r(boardLeft + col * BLOCK_SIZE, boardTop - 22, BLOCK_SIZE, 18);
        p.drawText(r, Qt::AlignCenter, QString::number(col));
    }

    for (int row = 0; row < BOARD_HEIGHT; ++row) {
        QRect r(boardLeft - 25, boardTop + row * BLOCK_SIZE, 18, BLOCK_SIZE);
        p.drawText(r, Qt::AlignCenter, QString::number(row));
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, currentTheme != ThemePixel);

    if (currentTheme == ThemeNeon) {
        QLinearGradient bg(0, 0, width(), height());
        bg.setColorAt(0.0, QColor(20, 20, 26));
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

    const int boardLeft = 55;
    const int boardTop = 55;
    const int boardWidthPx = BOARD_WIDTH * BLOCK_SIZE;
    const int boardHeightPx = BOARD_HEIGHT * BLOCK_SIZE;

    if (currentTheme == ThemeNeon) {
        p.setPen(QPen(QColor(0, 255, 255, 160), 2));
        p.setBrush(QColor(0, 0, 0, 80));
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

    drawCoordinateLabels(p, boardLeft, boardTop);

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

    int infoX = 390;

    p.setPen(getTextColor());
    QFont titleFont = p.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(infoX, 80, "俄罗斯方块");

    QFont normalFont = p.font();
    normalFont.setPointSize(10);
    normalFont.setBold(false);
    p.setFont(normalFont);
    p.drawText(infoX, 125, "得分: " + QString::number(score));
    p.drawText(infoX, 155, "当前主题: " + getThemeName());
    p.drawText(infoX, 185, "当前状态: " +
                               QString(
                                   gameState == StateMenu ? "菜单界面" :
                                       gameState == StatePlaying ? "游戏中" :
                                       gameState == StatePaused ? "暂停中" :
                                       "游戏结束"));

    QFont sectionFont = p.font();
    sectionFont.setPointSize(10);
    sectionFont.setBold(true);
    p.setFont(sectionFont);
    p.drawText(infoX, 235, "操作说明：");

    QFont smallFont = p.font();
    smallFont.setPointSize(9);
    smallFont.setBold(false);
    p.setFont(smallFont);

    QRect controlRect(infoX, 255, 320, 140);
    p.drawText(controlRect, Qt::TextWordWrap,
               "← → 移动方块\n"
               "↓ 加速下落\n"
               "↑ / 空格 旋转方块\n"
               "1 经典主题\n"
               "2 霓虹主题\n"
               "3 像素主题\n"
               "P 暂停 / 继续\n"
               "R 重新开始");

    p.setFont(sectionFont);
    p.drawText(infoX, 420, "特殊方块说明：");

    p.setFont(smallFont);
    QRect ruleRect(infoX, 440, 320, 250);
    p.drawText(ruleRect, Qt::TextWordWrap,
               "B 炸弹方块：落地后立刻爆炸，清除周围 3x3 范围。\n\n"
               "R 彩虹方块：万能补位方块，能帮助差 1 格的行也消除。\n\n"
               "C 清除方块：落地后立即清除当前所在整列。\n\n"
               "提示：彩虹块是“补位工具”，不是自动清除块。");

    QRect overlayRect(boardLeft + 25, boardTop + 200, boardWidthPx - 50, 125);

    if (gameState == StateMenu || gameState == StatePaused || gameState == StateGameOver) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 120));

        QFont overlayFont = p.font();
        overlayFont.setPointSize(15);
        overlayFont.setBold(true);
        p.setFont(overlayFont);
        p.setPen(Qt::white);

        if (gameState == StateMenu) {
            QRect overlayRect(boardLeft + 70, boardTop + 180, 300, 140);

            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 140));
            p.drawRoundedRect(overlayRect, 18, 18);

            QFont overlayFont = p.font();
            overlayFont.setPointSize(16);
            overlayFont.setBold(true);
            p.setFont(overlayFont);
            p.setPen(Qt::white);

            p.drawText(overlayRect.adjusted(0, 20, 0, 0), Qt::AlignHCenter, "欢迎来到俄罗斯方块");
            p.drawText(overlayRect.adjusted(0, 60, 0, 0), Qt::AlignHCenter, "点击右下角“开始游戏”进入");
            p.drawText(overlayRect.adjusted(0, 98, 0, 0), Qt::AlignHCenter, "主题 / 特殊方块 / 网格辅助");
        }
        else if (gameState == StatePaused) {
            p.drawText(overlayRect.adjusted(0, 28, 0, 0), Qt::AlignHCenter, "游戏已暂停");
            p.drawText(overlayRect.adjusted(0, 66, 0, 0), Qt::AlignHCenter, "点击“继续游戏”恢复");
        }
        else {
            p.drawText(overlayRect.adjusted(0, 28, 0, 0), Qt::AlignHCenter, "游戏结束");
            p.drawText(overlayRect.adjusted(0, 66, 0, 0), Qt::AlignHCenter, "点击“开始游戏”重新开始");
        }
    }
}