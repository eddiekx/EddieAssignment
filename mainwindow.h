#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QColor>
#include <QVector>
#include <QPair>
#include <QPoint>

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
    SpecialClearColumn = 3,
    SpecialObstacle = 4
};

enum GameState
{
    StateMenu = 0,
    StatePlaying = 1,
    StatePaused = 2,
    StateGameOver = 3
};

enum GameMode
{
    ModeClassic = 0,
    ModeTimed = 1,
    ModeChallenge = 2,
    ModeSpecial = 3,
    ModeObstacle = 4
};

struct Cell
{
    bool filled = false;
    int color = 0;
    int special = SpecialNone;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onGameTick();

private:
    Ui::MainWindow *ui;

    Cell board[BOARD_HEIGHT][BOARD_WIDTH];
    Cell currentPiece[4][4];
    Cell nextPiece[4][4];
    Cell holdPiece[4][4];

    int currentX;
    int currentY;

    int score;
    int level;
    int totalLinesCleared;
    int comboStreak;
    int timeLeftSec;

    int dropAccumMs;
    int timeAccumMs;
    int achievementTicks;

    bool hasHoldPiece;
    bool holdUsedThisTurn;
    bool usedSpecialThisGame;

    QString achievementText;
    QString endMessage;

    ThemeType currentTheme;
    GameState gameState;
    GameMode selectedMode;
    GameMode gameMode;

    QTimer *gameTimer;

    QPushButton *startButton;
    QPushButton *pauseButton;
    QPushButton *continueButton;
    QPushButton *holdButton;
    QPushButton *restartButton;

    QPushButton *classicModeButton;
    QPushButton *timedModeButton;
    QPushButton *challengeModeButton;
    QPushButton *specialModeButton;
    QPushButton *obstacleModeButton;

    bool achFirstLine;
    bool achCombo3;
    bool achHold;
    bool achLevel3;
    bool achSpecial;
    bool achTimedHalf;

private:
    void selectMode(GameMode mode);

    void startGame(GameMode mode);
    void pauseGame();
    void continueGame();
    void holdCurrentPiece();
    void restartCurrentMode();
    void updateButtonState();
    void refreshModeButtonTexts();

    void resetBoard();
    void setupObstacleField();
    void generateRandomPiece(Cell piece[4][4]);
    void spawnNextPiece();

    bool checkCollision(int x, int y, Cell piece[4][4]) const;
    bool movePieceDown(bool softDrop);
    void lockCurrentPiece();
    void mergePiece();
    void applySpecialOnLock();
    int clearLines();

    void rotatePiece();
    void finishGame(const QString &message);
    void showAchievement(const QString &message);

    int currentDropIntervalMs() const;
    int scoreForLines(int lines) const;
    int specialSpawnChance() const;
    int challengeTargetLines() const;

    QString themeName() const;
    QString modeName(GameMode mode) const;
    QString modeDescription(GameMode mode) const;
    QString stateName() const;
    QString currentModeName() const;
    QString currentModeDescription() const;

    QColor getThemeBlockColor(int colorIndex) const;
    QColor getRainbowColor(int step) const;
    QColor getGridColor() const;
    QColor getBackgroundColor() const;
    QColor getTextColor() const;

    void drawBlock(QPainter &p, int x, int y, const Cell &cell);
    void drawMiniPiece(QPainter &p, const Cell piece[4][4], int left, int top, int cellSize, const QString &label);
    void drawOutlinedText(QPainter &p, const QRect &rect, const QString &text, int fontSize, bool bold = true);
};

#endif // MAINWINDOW_H