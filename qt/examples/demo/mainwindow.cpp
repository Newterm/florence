#include <typeinfo>

#include "mainwindow.h"
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->florence->setOpacity( 0.5 );

    connectLineEdit( ui->lineEdit );
    connectLineEdit( ui->keyColor );
    connectLineEdit( ui->activeColor );
    connectLineEdit( ui->latchedColor );
    connectLineEdit( ui->focusColor );
    connectLineEdit( ui->outlineColor );
    connectLineEdit( ui->symbolColor );
    connectLineEdit( ui->symbolOutlineColor );
    connectLineEdit( ui->font );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectLineEdit( LineEdit *lineEdit )
{
    QObject::connect( lineEdit, SIGNAL(focusIn()), this, SLOT(showFlorence()) );
    QObject::connect( lineEdit, SIGNAL(focusOut()), this, SLOT(hideFlorence()) );
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

void MainWindow::resizeEvent( QResizeEvent *event )
{
    QMainWindow::resizeEvent( event );
    if ( ui->move->checkState() ) {
        ui->florence->resize( QSize( this->size().width() * 0.7, this->size().height() * 0.3 ) );
        moveFlorence( ui->centralwidget->focusWidget() );
    }
}

void MainWindow::moveFlorence( QWidget *widget )
{
    if ( widget && ( typeid( *widget ) == typeid( LineEdit ) ) ) {
        QPoint p = widget->mapTo( ui->florence->parentWidget(), widget->rect().bottomLeft() ) + QPoint( 0, 5 );
        // Make sure the keyboard fits in the central widget
        if ( ( p.y() + ui->florence->height() ) > ui->centralwidget->height() ) {
            p = widget->mapTo( ui->florence->parentWidget(), widget->rect().topLeft() ) + QPoint( 0, -ui->florence->height() - 5 );
        }
        if ( ( p.x() + ui->florence->width() ) > ui->centralwidget->width() ) {
            p.setX( ui->centralwidget->width() - ui->florence->width() );
        }
        ui->florence->move( p );
    }
}

void MainWindow::hideFlorence()
{
    QWidget *widget = static_cast<QWidget *>(QObject::sender());
    if ( ui->autoHide->checkState() && ui->florence->isVisible() ) ui->florence->hide();
    ui->florence->disconnect( widget );
    ui->florence->setFocusProxy( NULL );
}

void MainWindow::showFlorence()
{
    QWidget *widget = static_cast<QWidget *>(QObject::sender());
    QObject::connect( ui->florence, SIGNAL(inputText(QString)), widget, SLOT(insertText(QString)) );
    QObject::connect( ui->florence, SIGNAL(backspace()), widget, SLOT(backSpace()) );
    QObject::connect( ui->florence, SIGNAL(tab()), widget, SLOT(tab()) );
    QObject::connect( ui->florence, SIGNAL(leftTab()), widget, SLOT(leftTab()) );
    QObject::connect( ui->florence, SIGNAL(enter()), widget, SLOT(enter()) );

    // move the keyboard to near the input widget
    if ( ui->move->checkState() ) {
        moveFlorence( widget );
    }

    ui->florence->setFocusProxy( widget );
    if ( ui->florence->isHidden() ) ui->florence->show();
}

void MainWindow::on_keyColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_KEY_COLOR, ui->keyColor->text() );
}

void MainWindow::on_activeColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_ACTIVATED_COLOR, ui->activeColor->text() );
}

void MainWindow::on_latchedColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_LATCHED_COLOR, ui->latchedColor->text() );
}

void MainWindow::on_focusColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_MOUSE_OVER_COLOR, ui->focusColor->text() );
}

void MainWindow::on_outlineColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_OUTLINE_COLOR, ui->outlineColor->text() );
}

void MainWindow::on_symbolColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_TEXT_COLOR, ui->symbolColor->text() );
}

void MainWindow::on_symbolOutlineColor_editingFinished()
{
    ui->florence->setColor( StyleItem::STYLE_TEXT_OUTLINE_COLOR, ui->symbolOutlineColor->text() );
}

void MainWindow::on_defaultStyle_toggled(bool checked)
{
    if (checked) ui->florence->setStyle( "../../data/styles/default.style" );
}

void MainWindow::on_hardStyle_toggled(bool checked)
{
    if (checked) ui->florence->setStyle( "../../data/styles/hard.style" );
}

void MainWindow::on_brightStyle_toggled(bool checked)
{
    if (checked) ui->florence->setStyle( "../../data/styles/bright.style" );
}

void MainWindow::on_opacity_valueChanged(int value)
{
    ui->florence->setOpacity( (qreal)value / 100.0 );
}

void MainWindow::on_keymap_activated(QString text)
{
    ui->florence->setKeymap( "../../data/keymaps/" + text );
}

void MainWindow::on_autoHide_toggled(bool checked)
{
    ui->florence->setVisible( !checked );
}

void MainWindow::on_move_toggled(bool checked)
{
    if ( checked ) {
        ui->placeHolder->removeWidget( ui->florence );
        ui->florence->resize( QSize( this->size().width() * 0.7, this->size().height() * 0.3 ) );
        ui->florence->move( ui->lineEdit->mapTo( ui->florence->parentWidget(), ui->lineEdit->rect().topLeft() ) + QPoint( 0, ui->lineEdit->height() + 5 ) );
    } else ui->placeHolder->addWidget( ui->florence );
}

void MainWindow::on_font_editingFinished()
{
    ui->florence->setFont( ui->font->text() );
}
