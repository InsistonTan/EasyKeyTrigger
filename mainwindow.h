#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QMutex>
#include<QMouseEvent>

#define DEFAULT_HOT_KEY "鼠标侧键1(后退)"

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

    void on_comboBox_2_currentTextChanged(const QString &arg1);

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
