#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    LineEdit( QWidget *parent = 0 );

signals:
    void focusIn();
    void focusOut();

protected:
    void focusInEvent ( QFocusEvent * event );
    void focusOutEvent ( QFocusEvent * event );

public slots:
    void insertText( const QString &text );
    void backSpace();
    void tab();
    void leftTab();
    void enter();

};

#endif
