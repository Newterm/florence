#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "lineedit.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);
    void resizeEvent( QResizeEvent *event );

private:
    Ui::MainWindow *ui;

    void connectLineEdit( LineEdit *lineEdit );
    void moveFlorence( QWidget *widget );

private slots:
    void on_font_editingFinished();
    void on_move_toggled(bool checked);
    void on_autoHide_toggled(bool checked);
    void on_keymap_activated(QString );
    void on_opacity_valueChanged(int value);
    void on_brightStyle_toggled(bool checked);
    void on_hardStyle_toggled(bool checked);
    void on_defaultStyle_toggled(bool checked);
    void on_symbolOutlineColor_editingFinished();
    void on_symbolColor_editingFinished();
    void on_outlineColor_editingFinished();
    void on_focusColor_editingFinished();
    void on_latchedColor_editingFinished();
    void on_activeColor_editingFinished();
    void on_keyColor_editingFinished();

    void hideFlorence();
    void showFlorence();
};

#endif // MAINWINDOW_H
