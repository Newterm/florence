//    This file is part of Florence Virtual Keyboard for Qt.
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

#ifndef FLORENCE_H
#define FLORENCE_H

#include <QGraphicsView>
#include "settings.h"
#include "key.h"

/*! \mainpage
  * \section main Reference documentation of Florence Virtual Keyboard for Qt.
  * \subsection intro Introduction
  * Florence-QT is a virtual keyboard that can be used to input text in an input widget.
  * This document is the reference documentation for developers.
  * \subsection start Getting started
  * The only class you need to use is the Florence class.
  * Florence is a QWidget that can be integrated in your QT application.
  * You can connect your input widget's slots to Florence's signals.
  *
  * Read the <a href="../tutorial/tutorial.html">hello world tutorial</a> for step by step instructions.
  * Compile and try the demo application to learn more about the possibilities offered.
  */

/*! \class Florence
  * \brief This class represents a keyboard widget
  *
  * The Florence class is a QGraphicsView and inherits QWidget.
  * Florence renders a visually appealing keyboard inside a QWidget that can be controlled by the application.
  * Florence provides a way to input text in another text edit widget.
  *
  * The keyboard is optimized for touch input. Keys are magnified when touched and a signal is emitted on touch end.
  * The signal is also sent when keys are held down for longer than 1 second.
  * The keys are auto repeated every 0.2 second for as long as they are held down.
  *
  * One can show and hide the keyboard with the show() and hide() slots (inherited from QWidget),
  * which can be connected to the focusInEvent() and focusOutEvent() methods of the target input widget.
  * Florence sends an inputText() signals to input text in the target widget.
  * The backspace(), enter(), tab() and leftTab() signals are sent when their respective keys are activated.
  *
  * You can find an example of target widget class here:
  * <a href="../../examples/helloworld/lineedit.h">lineedit.h</a>.
  * Class implementation:
  * <a href="../../examples/helloworld/lineedit.cpp">lineedit.cpp</a>.
  *
  * The keymap, the layout and the style can be read from an xml file
  * by calling the setKeymap(), setLayout() and setStyle() member functions, respectively.
  * The style is what dictates the visual appearance of the keyboard. It defines the shape of the keys and symbols.
  * The layout defines the how the keys are disposed on the keyboard.
  * The keymap assigns a symbol value to each key that is language dependant.
  * The default files are included in a binary resource inside the library.
  * On some platforms, if Florence is compiled as a static library and you use the default file (by not calling the functions above),
  * it may be necessary to call the Q_INIT_RESOURCE(florence_qt) macro for Florence to display something.
  *
  * Here is an code example for instantiating Florence:
  *
  * \code
  * Florence *f = new Florence( this );
  * f->setLayout( "florence.layout" );
  * f->setStyle( "florence.style" );
  * f->setKeymap( "us.xml" );
  *
  * LineEdit l = new LineEdit( this );
  * QObject::connect( f, SIGNAL(inputText(QString)), l, SLOT(insertText(QString)) );
  * QObject::connect( f, SIGNAL(backspace()), l, SLOT(backSpace()) );
  * f->setFocusProxy( l );
  * \endcode
  *
  * Note the setFocusProxy makes sure Florence doesn't steal the keyboard focus
  * from the input widget so it keeps the cursor displayed.
  * \implements QGraphicsView
  * \implements QDesignerCustomWidgetInterface
  */
class Florence : public QGraphicsView
{
    Q_OBJECT

public:
    /*! \fn Florence( QWidget *parent = 0 )
      * \brief Constructor.
      *
      * Instantiates a Florence object.
      * \param parent the parent widget
      */
    Florence( QWidget *parent = 0 );
    /*! \fn ~Florence
      * \brief Destructor.
      *
      * Destroys the object.
      */
    ~Florence();

    /*! \fn setLayout( QString file )
      * \brief Sets the layout of the keyboard.
      *
      * Reads the keyboard layout from the file.
      * \param file Name of the file that contains the XML description of the layout.
      * \returns true on success and false on failure.
      */
    bool setLayout( QString file );
    /*! \fn setStyle( QString file )
      * \brief Sets the style of the keyboard.
      *
      * Reads the keyboard style from the file.
      * \param file Name of the file that contains the XML description of the style.
      * \returns true on success and false on failure.
      */
    bool setStyle( QString file );
    /*! \fn setKeymap( QString file )
      * \brief Sets the keymap of the keyboard.
      *
      * Reads the keyboard keymap from the file.
      * \param file Name of the file that contains the XML description of the keymap.
      * \returns true on success and false on failure.
      */
    bool setKeymap( QString file );
    /*! \fn setColor( enum StyleItem::style_colours color, QString value )
      * \brief Changes a style color.
      *
      * Sets the color of a style element.
      * The color is a string that represents the value of the color in one of those format:
      * <ul>
      * <li> \#RGB ( \#0FF )
      * <li> \#RRGGBB ( \#00FFFF )
      * <li> \#RRRRGGGGBBBB ( \#0000FFFFFFFF )
      * <li> color name ( cyan )
      * <li> transparent
      * </ul>
      * \param color The style element to change.
      * \param value The string representation of the color to set the style element to.
      */
    void setColor( enum StyleItem::style_colours color, QString value );
    /*! \fn setOpacity( qreal opacity )
      * \brief Sets the keyboard opacity.
      *
      * Only the background of the key's opacity is affecter by this function.
      * This is a design decision so the labels of the key are always clearly visible.
      * \param opacity A number from 0.0 (transparent) to 1.0 (opaque)
      */
    void setOpacity( qreal opacity );
    /*! \fn setFont( QString font )
      * \brief Sets the font of the symbols.
      *
      * Changes the font used to draw the key symbols.
      * \param font The name of the font.
      */
    void setFont( QString font );

signals:
    /*! \fn inputText( QString text )
      * \brief Input text signal.
      *
      * This signal is emitted when a key that should input text is activated.
      * It should be connected to the slot that inserts text in the target widget.
      * \param text The text value of the key that has been activated.
      */
    void inputText( QString text );
    /*! \fn backspace()
      * \brief Back space signal.
      *
      * This signal is emitted when the BackSpace key has been activated.
      * It should be connected to the slot that deletes the character to the left of the text cursor in the target widget.
      */
    void backspace();
    /*! \fn enter()
      * \brief Return signal.
      *
      * This signal is emitted when the Return key has been activated.
      * It can be used to hide the keyboard, or input a carriage return, depending on the context.
      */
    void enter();
    /*! \fn tab()
      * \brief Tab signal.
      *
      * This signal is emitted when the Tab key has been activated.
      * This can be used to move the focus to the next widget, or input a tab in the target widget.
      */
    void tab();
    /*! \fn leftTab()
      * \brief Left tab signal.
      *
      * This signal is emitted when the Left tab key has been activated.
      * This can be used to move the focus to the previous widget, or to unindent text in the target widget.
      */
    void leftTab();

protected:
    void resizeEvent( QResizeEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseDoubleClickEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void leaveEvent( QEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );

private:
    QGraphicsScene *graphicsScene;
    qreal sceneWidth, sceneHeight;
    Settings *settings;
    Key *focusKey;
    QVector<Key *> latchedKeys;
    QTimer *autoRepeatTimer;

private slots:
    void keyPressed( enum Symbol::symbol_role role, QString text );

    void repeat();

    void latchKey( Key *key );
    void unlatchKey( Key *key );
    void unlatchAll();
};

#endif // FLORENCE_H
