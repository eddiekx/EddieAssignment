#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qdialog.h"
#include <QMainWindow>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QPainter>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;
const int BLOCK_SIZE = 30;

enum ThemeType
{
    ThemeClassic = 0,
    ThemeNeon = 1,
    ThemePixel = 2
};

enum SpecialType
{
    SpecialNone = 0,
    SpecialBomb = 1,
    SpecialRainbow = 2,
    SpecialClearColumn = 3
};

struct Cell
{
    bool filled;
    int color;      // 0~6，普通颜色索引
    int special;    // SpecialNone / SpecialBomb / SpecialRainbow / SpecialClearColumn
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    bool gameStarted;
    bool gamePaused;

    QPushButton *startButton;
    QPushButton *pauseButton;
    QPushButton *continueButton;

    void startGame();
    void pauseGame();
    void continueGame();
    Ui::MainWindow *ui;

    Cell board[BOARD_HEIGHT][BOARD_WIDTH];

    Cell currentPiece[4][4];
    int currentX;
    int currentY;

    int timerId;
    int score;
    ThemeType currentTheme;

    void initGame();
    void generatePiece();
    bool checkCollision(int x, int y, Cell piece[4][4]);
    void mergePiece();
    void clearLines();
    void rotatePiece();

    void clearColumn(int col);
    void clearArea(int centerX, int centerY, int radius);
    void applySpecialOnLock();

    QColor getThemeBlockColor(int colorIndex) const;
    QColor getRainbowColor(int step) const;
    QColor getGridColor() const;
    QColor getBackgroundColor() const;
    QColor getTextColor() const;
    QString getThemeName() const;

    void drawBlock(QPainter &p, int x, int y, const Cell &cell);
    void drawCoordinateLabels(QPainter &p, int boardLeft, int boardTop) const;
};

#endif // MAINWINDOW_H