#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "key_map.h"
#include <windows.h>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QShortcut>
#include <QMessageBox>
#include <QRandomGenerator>

// 全局钩子句柄
static HHOOK g_mouseHook = nullptr;
MainWindow* mainWindow = nullptr;

// 选择的启停热键
short selectedHotKey = -1;


// 低级鼠标钩子过程函数
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        // 鼠标右键按下事件
        if (selectedHotKey == 1 && wParam == WM_RBUTTONDOWN) {
            // 使用Qt的信号机制确保在主线程中执行
            QMetaObject::invokeMethod(mainWindow, []() {
                mainWindow->startOrStop();
            }, Qt::QueuedConnection);

        }else if (wParam == WM_XBUTTONDOWN) {
            MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;

            // 使用更兼容的方法检测侧键
            UINT xbutton = HIWORD(mouseStruct->mouseData);

            // 第一个侧键（通常为后退按钮）
            if (selectedHotKey == 2 && (xbutton == XBUTTON1)) {
                QMetaObject::invokeMethod(mainWindow, []() {
                    mainWindow->startOrStop();
                }, Qt::QueuedConnection);
            }
            // 第二个侧键（通常为前进按钮）
            else if (selectedHotKey == 3 && (xbutton == XBUTTON2)) {
                QMetaObject::invokeMethod(mainWindow, []() {
                    mainWindow->startOrStop();
                }, Qt::QueuedConnection);
            }
        }
    }
    // 将事件传递给下一个钩子或系统
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainWindow = this;

    // 添加按键到下拉框
    for (auto it = VK_MAP.begin(); it != VK_MAP.end(); ++it) {
        ui->comboBox->addItem(it.key());
    }
    // 默认按键
    ui->comboBox->setCurrentText("E");
    ui->comboBox->installEventFilter(this);


    // 添加快捷启停键到下拉框
    for (auto it = HOT_KEY_MAP.begin(); it != HOT_KEY_MAP.end(); ++it) {
        ui->comboBox_2->addItem(it.key());
    }
    // 默认启停键
    ui->comboBox_2->setCurrentText(DEFAULT_HOT_KEY);
    selectedHotKey = HOT_KEY_MAP[DEFAULT_HOT_KEY];
    ui->comboBox_2->installEventFilter(this);


    // 安装低级鼠标钩子[citation:1]
    g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(nullptr), 0);
    if (!g_mouseHook) {
        QMessageBox::critical(this,"错误", "安装鼠标钩子失败！");
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;

    // 卸载钩子[citation:1]
    if (g_mouseHook) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = nullptr;
    }
}


void MainWindow::setStartStatus(){
    ui->pushButton->setText("停止");
    ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(240, 106, 74);}");

    ui->label_4->setText("正在运行");
    ui->label_4->setStyleSheet("QLabel{color:rgb(6, 200, 99);}");
}
void MainWindow::setStopStatus(){
    ui->pushButton->setText("启动");
    ui->pushButton->setStyleSheet("QPushButton{background-color:rgb(6, 200, 99);}");

    ui->label_4->setText("已停止");
    ui->label_4->setStyleSheet("QLabel{color:rgb(240, 106, 74);}");
}

void MainWindow::startOrStop(){
    // 自动加锁
    QMutexLocker locker(&muteX);
    if(isRunning){
        isRunning = false;
        setStopStatus();
    }else{
        isRunning = true;
        setStartStatus();

        // 键盘按键硬件扫描码
        short scanCode = -1;
        // 按键循环间隔时间
        int sleepTimeMs = 3000;
        // 按键按下到松开的持续时间
        int keepTimeMs = 100;

        if(VK_MAP.contains(ui->comboBox->currentText())){
            scanCode = VK_MAP[ui->comboBox->currentText()];
        }else{
            isRunning = false;
            setStopStatus();
            return;
        }

        QString sleepTimeMsStr = ui->lineEdit->text();
        bool ok;
        int value = sleepTimeMsStr.toInt(&ok);
        if(ok){
            sleepTimeMs = value;
        }

        QString keepTimeMsStr = ui->lineEdit_2->text();
        bool ok2;
        int value2 = keepTimeMsStr.toInt(&ok2);
        if(ok){
            keepTimeMs = value2;
        }

        // 启动一个任务执行 循环按键触发
        QtConcurrent::run([=]() {
            while(true){
                // 加锁
                muteX.lock();
                if(!isRunning){
                    setStopStatus();

                    muteX.unlock();

                    return;
                }
                muteX.unlock();

                int tmpKeepTimeMs = keepTimeMs;
                int tmpSleepTimeMs = sleepTimeMs;

                // 如果开启时间随机模式
                if(ui->checkBox->isChecked()){
                    tmpKeepTimeMs += QRandomGenerator::global()->bounded(-tmpKeepTimeMs/5, tmpKeepTimeMs/5);
                    tmpSleepTimeMs += QRandomGenerator::global()->bounded(-tmpSleepTimeMs/10, tmpSleepTimeMs/10);
                }

                qDebug() << "tmpKeepTimeMs: " << tmpKeepTimeMs << ", tmpSleepTimeMs: "<< tmpSleepTimeMs;

                // 触发按键
                simulateKeyPress(scanCode, false);

                // 等待持续时间
                QThread::msleep(tmpKeepTimeMs);
                // 持续时间后松开按键
                simulateKeyPress(scanCode, true);


                // 等待间隔时间
                QThread::msleep(tmpSleepTimeMs);
            }
        });
    }
}

void MainWindow::on_pushButton_clicked()
{
    startOrStop();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event){
    // 1. 检查事件来源是否为我们的comboBox且事件类型为键盘按下
    if ((obj == ui->comboBox || obj == ui->comboBox_2) && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        // 拦截所有普通按键
        if (keyEvent->key() >= Qt::Key_Space && keyEvent->key() <= Qt::Key_AsciiTilde) {
            return true; // 返回true，表示事件已被处理，不再向下传递
        }
    }

    // 3. 对于其他情况，调用基类的事件过滤器
    return QMainWindow::eventFilter(obj, event);
}

// 模拟按键操作
void MainWindow::simulateKeyPress(short scanCode, bool isKeyRelease) {
    // 模拟键盘操作
    if(scanCode > 0){
        INPUT input = {0};

        short tmpDwFlags;

        // 设置为使用硬件扫描码, 并为某些功能按键添加扩展码
        if(scanCode >= 0xC5 && scanCode <= 0xDF ){
            tmpDwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
        }else{
            tmpDwFlags = KEYEVENTF_SCANCODE;
        }

        // 模拟按下键
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = tmpDwFlags;
        // 设置扫描码
        input.ki.wScan = scanCode;

        // 模拟释放键
        if(isKeyRelease){
            input.ki.dwFlags = tmpDwFlags | KEYEVENTF_KEYUP;
        }

        SendInput(1, &input, sizeof(INPUT));
    }
}


void MainWindow::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    selectedHotKey = HOT_KEY_MAP[arg1];
}

