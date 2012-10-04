#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWebFrame>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->florence->setFocusProxy(ui->webView);
    QObject::connect( ui->florence, SIGNAL(inputText(QString)), this, SLOT(insertText(QString)) );
    QObject::connect( ui->florence, SIGNAL(backspace()), this, SLOT(backspace()) );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::insertText( QString text )
{
    int pos = ui->webView->inputMethodQuery(Qt::ImCursorPosition).toInt();
    ui->webView->page()->mainFrame()->evaluateJavaScript(
            "var oldValue = document.activeElement.value;"
            "document.activeElement.value = "
            "oldValue.substring(0, " + QString::number(pos) + ") + "
            "\"" + text + "\" + "
            "oldValue.substring(" + QString::number(pos) + ");"
            "document.activeElement.setSelectionRange(" + QString::number(pos+1) + ", " + QString::number(pos+1) + ");");
}

void MainWindow::backspace()
{
    int pos = ui->webView->inputMethodQuery(Qt::ImCursorPosition).toInt();
    ui->webView->page()->mainFrame()->evaluateJavaScript(
            "var oldValue = document.activeElement.value;"
            "document.activeElement.value = "
            "oldValue.substring(0, " + QString::number(pos-1) + ") + "
            "oldValue.substring(" + QString::number(pos) + ");"
            "document.activeElement.setSelectionRange(" + QString::number(pos-1) + ", " + QString::number(pos-1) + ");");
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
