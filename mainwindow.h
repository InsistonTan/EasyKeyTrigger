#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QMutex>
#include<QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void startOrStop();

private slots:
    void on_pushButton_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    bool isRunning = false;
    QMutex muteX;


    void simulateKeyPress(short vkey, bool isKeyRelease);
    void setStartStatus();
    void setStopStatus();
};
#endif // MAINWINDOW_H
