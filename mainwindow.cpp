#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QRandomGenerator>

const int SHAPES[7][4][4] = {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setFocusPolicy(Qt::StrongFocus); // ⭐关键
    this->setFixedSize(400, 650);

    timerId = 0;
    initGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initGame()
{
    for(int i=0;i<BOARD_HEIGHT;i++)
        for(int j=0;j<BOARD_WIDTH;j++)
            board[i][j]=0;

    score = 0;

    if(timerId != 0)
        killTimer(timerId);

    timerId = startTimer(500);

    generatePiece();
}

void MainWindow::generatePiece()
{
    int index = QRandomGenerator::global()->bounded(7);

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            currentPiece[i][j] = SHAPES[index][i][j];

    currentX = BOARD_WIDTH/2 - 2;
    currentY = 0;

    if(checkCollision(currentX,currentY,currentPiece)){
        killTimer(timerId);
        QMessageBox::information(this,"Game Over","得分："+QString::number(score));
        initGame();
    }
}

bool MainWindow::checkCollision(int x,int y,int piece[4][4])
{
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            if(piece[i][j]){
                int bx = x+j;
                int by = y+i;

                if(bx<0 || bx>=BOARD_WIDTH || by>=BOARD_HEIGHT)
                    return true;

                if(by>=0 && board[by][bx])
                    return true;
            }
        }
    }
    return false;
}

void MainWindow::mergePiece()
{
    for(int i=0;i<4;i++){
        for(int j=0;j<4;j++){
            if(currentPiece[i][j]){
                int bx = currentX+j;
                int by = currentY+i;
                if(by>=0)
                    board[by][bx] = 1;
            }
        }
    }
}

void MainWindow::clearLines()
{
    for(int i=BOARD_HEIGHT-1;i>=0;i--){
        bool full=true;
        for(int j=0;j<BOARD_WIDTH;j++){
            if(!board[i][j]){
                full=false;
                break;
            }
        }

        if(full){
            for(int k=i;k>0;k--)
                for(int j=0;j<BOARD_WIDTH;j++)
                    board[k][j]=board[k-1][j];

            for(int j=0;j<BOARD_WIDTH;j++)
                board[0][j]=0;

            score += 100;
            i++;
        }
    }
}

void MainWindow::rotatePiece()
{
    int temp[4][4] = {0};

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            temp[j][3-i] = currentPiece[i][j];

    if(!checkCollision(currentX,currentY,temp)){
        for(int i=0;i<4;i++)
            for(int j=0;j<4;j++)
                currentPiece[i][j]=temp[i][j];
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key()==Qt::Key_Left){
        if(!checkCollision(currentX-1,currentY,currentPiece))
            currentX--;
    }
    else if(event->key()==Qt::Key_Right){
        if(!checkCollision(currentX+1,currentY,currentPiece))
            currentX++;
    }
    else if(event->key()==Qt::Key_Down){
        if(!checkCollision(currentX,currentY+1,currentPiece))
            currentY++;
    }
    else if(event->key()==Qt::Key_Up){
        rotatePiece();
    }

    update();
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if(event->timerId()==timerId){
        if(!checkCollision(currentX,currentY+1,currentPiece)){
            currentY++;
        }else{
            mergePiece();
            clearLines();
            generatePiece();
        }
        update();
    }
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    int offsetX=20;
    int offsetY=20;

    p.drawRect(offsetX,offsetY,BOARD_WIDTH*BLOCK_SIZE,BOARD_HEIGHT*BLOCK_SIZE);

    p.setBrush(Qt::blue);
    for(int i=0;i<BOARD_HEIGHT;i++)
        for(int j=0;j<BOARD_WIDTH;j++)
            if(board[i][j])
                p.drawRect(offsetX+j*BLOCK_SIZE,offsetY+i*BLOCK_SIZE,BLOCK_SIZE,BLOCK_SIZE);

    p.setBrush(Qt::red);
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            if(currentPiece[i][j])
                p.drawRect(offsetX+(currentX+j)*BLOCK_SIZE,
                           offsetY+(currentY+i)*BLOCK_SIZE,
                           BLOCK_SIZE,BLOCK_SIZE);

    p.drawText(320,50,"Score:"+QString::number(score));
}