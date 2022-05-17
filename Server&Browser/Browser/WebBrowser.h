#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_WebBrowser.h"

class WebBrowser : public QMainWindow
{
    Q_OBJECT

public:
    WebBrowser(QWidget *parent = Q_NULLPTR);

private:
    Ui::WebBrowserClass ui;

private slots:
    void on_button1_clicked();
    void on_button2_clicked();
    void on_button3_clicked();
};
