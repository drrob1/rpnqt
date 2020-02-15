#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "macros.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_lineEdit_returnPressed();

    void on_pushButton_exit_clicked();

    void on_pushButton_quit_clicked();

    void on_action2_triggered();

    void on_action4_triggered();

    void on_action6_triggered();

    void on_action8_triggered();

    void on_action_1_triggered();

    void on_pushButton_enter_clicked();

    void on_actionfixed_triggered();

    void on_actionfloat_triggered();

    void on_actiongen_triggered();

    void on_actionClear_Output_triggered();

    void on_action0_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
