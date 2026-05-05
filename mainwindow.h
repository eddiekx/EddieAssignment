#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    Ui::MainWindow *ui;

    int board[BOARD_HEIGHT][BOARD_WIDTH];
    int timerId;
    int score;

    int currentPiece[4][4];
    int currentX, currentY;

    void initGame();
    void generatePiece();
    bool checkCollision(int x, int y, int piece[4][4]);
    void mergePiece();
    void clearLines();
    void rotatePiece();
};

#endif