#include "lineedit.h"

LineEdit::LineEdit( QWidget *parent )
    : QLineEdit( parent )
{
}

void LineEdit::insertText( const QString &text )
{
    this->insert( text );
}

void LineEdit::backSpace()
{
    this->backspace();
}

void LineEdit::tab()
{
    this->focusNextChild();
}

void LineEdit::leftTab()
{
    this->focusPreviousChild();
}

void LineEdit::enter()
{
    this->clearFocus();
}

void LineEdit::focusInEvent ( QFocusEvent * event )
{
    QLineEdit::focusInEvent( event );
    emit focusIn();
}

void LineEdit::focusOutEvent ( QFocusEvent * event )
{
    QLineEdit::focusOutEvent( event );
    emit focusOut();
}
