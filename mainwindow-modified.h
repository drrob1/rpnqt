#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "macros.h"
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_lineEdit_returnPressed();

    void on_pushButton_exit_clicked();

    void on_pushButton_quit_clicked();

    void on_action2_triggered();

    void on_action4_triggered();

    void on_action4_2_triggered();

    void on_action6_triggered();

    void on_action8_triggered();

    void on_action_1_triggered();

    void on_pushButton_enter_clicked();

    void on_actionfixed_triggered();

    void on_actionfloat_triggered();

    void on_actiongen_triggered();

    void on_actionClear_Output_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
