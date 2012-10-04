//    This file is part of Florence Virtual Keyboard, QT version.
//    Copyright (C) 2011  iKare Corporation
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QtGui/QApplication>
#include <QVBoxLayout>
#include "lineedit.h"
#include "florence.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget *widget = new QWidget();
    widget->resize( 600, 230 );

    LineEdit *input = new LineEdit( widget );

    Florence *keyboard = new Florence( widget );
    QObject::connect( keyboard, SIGNAL(inputText(QString)), input, SLOT(insertText(QString)) );
    QObject::connect( keyboard, SIGNAL(backspace()), input, SLOT(backSpace()) );
    keyboard->setFocusProxy( input );

    QVBoxLayout *layout = new QVBoxLayout( widget );
    layout->addWidget( input );
    layout->addWidget( keyboard );

    widget->setLayout( layout );
    widget->show();

    return a.exec();
}
