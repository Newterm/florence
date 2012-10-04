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

#ifndef KEY_H
#define KEY_H

#include <QGraphicsSvgItem>
#include <QDomElement>
#include <QPainter>
#include <QTimer>
#include "style.h"
#include "keymap.h"

/*! \class Key
  * \brief This class represents a key on the keyboard.
  *
  * The Key class is a QGraphicsSvgItem that renders a key on the Florence keyboard.
  * It is loaded from a &lt;key&gt; element in the XML layout file.
  * The &lt;key&gt; element is structured as follow:
  * \code
  * <key>
  *    <code>code</code>
  *    <xpos>xpos</xpos>
  *    <ypos>ypos</ypos>
  *    <width>width</width>
  *    <height>height</height>
  *    <shape>shape name</same>
  *    <bounds>bounds commands</bounds>
  * </key>
  * \endcode
  * where:
  * <ul>
  *    <li><i>code</i> is the key code that is represented in the keymap file.</li>
  *    <li><i>xpos</i> and <i>ypos</i> represent the coordinates of the key in the layout, in layout coordinates.</li>
  *    <li><i>width</i> and <i>height</i> are optional and represent the size of the key in the layout, in layout units.
  *        The default is ( 2, 2 )</li>
  *    <li><i>shape name</i> is optional and is the name of the style shape as found in the style file.
  *        The default is "default"</li>
  *    <li><i>bounds commands</i> is optional and represents the bounds of the key in layout coordinates.
  *        The bounds represents a polygon that contains the key. It is used to detect if a key is hit by touch or not.
  *        It is defined by a list of commands that draw the polygon. There are 2 valid commands:
  *        <ul>
  *           <li>the &lt;m&gt; (move) command is always the first command to start the polygon.
  *               It sets the coordinates to start the polygon as follow: &lt;m x="x" y="y"/&gt;, where x and y are the coordinates to move to, in key coordinates.</li>
  *           <li>the &lt;l&gt; ( line: &lt;l x="x" y="y"/&gt; ) command draws a line from the last position to the new specified position.</li>
  *        </ul>
  *        A final line is drawn from the last point to the first to close the polygon.
  *        If not specified, the bounds of the key is a rectangle at the position and of the size of the key.
  * </ul>
  *
  * The keys are painted in accordance with the Settings object provided in the constructor and redraw themselves when the settings are changed.
  *
  * The keys trigger an action when they are pressed. The action triggered depends on the Keymap configuration for the code of the key.
  * The Keymap dictates the behaviour of the key.
  * There are 3 kinds of keys:
  * <ul>
  *   <li>The non modifier keys are used to input text in an input widget. They can have 2 states: KEY_PRESSED and KEY_RELEASED.
  *       Initially, all keys are released. They become pressed on press for 200ms and return to the released state after that. Pressed keys are displayed in red.</li>
  *   <li>The modifier keys modify the symbol of the non modifier keys. Modifier keys have 4 states: KEY_RELEASED, KEY_PRESSED, KEY_LATCHED and KEY_LOCKED.
          A released modifier becomes latched when pressed. Latched keys are green. It will stay latched until another non modifier key is pressed or until it is pressed again.
          When another key is pressed, it becomes released. If pressed again, it becomes locked. locked keys stay locked until pressed again.</li>
  *   <li>The locker keys are modifier keys with only 2 states: KEY_RELEASED and KEY_LOCKED. They become locked when pressed and released when pressed again.</li>
  * </ul>
  */
class Key : public QGraphicsSvgItem
{
    Q_OBJECT

public:
    /*! \enum key_status
      * Defines the status of the key.
      */
    enum key_status {
        KEY_RELEASED, /*!< The key is released. This is the initial state of all keys. */
        KEY_PRESSED, /*!< The key is pressed. The keys stay pressed for 200ms. */
        KEY_LATCHED, /*!< Only for modifier keys. The key is latched until a non modifier key is pressed. */
        KEY_LOCKED /*!< The key is locked. Locked keys stay locked until pressed again. */
    };

    /*! \fn Key( QDomElement el, Settings *settings )
      * \brief Constuctor.
      *
      * Instantiates a Key object.
      * \param el Pointer to the parsed XML document containing the key description.
      * \param settings Pointer to the settings object of the Keyboard.
      */
    Key( QDomElement el, Settings *settings );

    /*! \fn hoverEnterEvent()
      * \brief Notifies the key that it is pointed either by touch or mouse.
      *
      * The effect of this function is to visually magnify the key.
      * Call this function when the key is hovered.
      */
    void hoverEnterEvent();
    /*! \fn hoverLeaveEvent()
      * \brief Notifies that the pointer has left the key.
      *
      * The effect of this function is to unmagnify he key.
      * Call this function when the pointer has moved and key is no longer hovered.
      */
    void hoverLeaveEvent();
    /*! \fn mouseReleaseEvent()
      * \brief Notifies the key of a press event.
      *
      * This will trigger the action configured in the Keymap for the key code of this key.
      * Call this function when the mouse button is released or on touch end.
      */
    void mouseReleaseEvent();
    /*! \fn unlatch()
      * \brief Unlatches the key.
      *
      * This function has an effect only on modifier keys that are latched.
      * Releases the key.
      * This function is called on all keys when a non modifier key is pressed.
      */
    bool unlatch();
    /*! \fn press
      * \brief Press the key.
      *
      * Changes the state of the key and trigger the action related to the new state.
      * For text keys, the action is to emit the text input signal and trigger the timer to release the key after 200ms.
      */
    void press();

signals:
    /*! \fn void keyPressed( enum Symbol::symbol_role role, QString text )
      * \brief Key pressed signal.
      *
      * This signal is emitted when a non modifier key is pressed.
      * \param role Role of the key
      * \param text Text to input in the target widget for keys with SYMBOL_TEXT role or the name of the symbol.
      */
    void keyPressed( enum Symbol::symbol_role role, QString text );

    /*! \fn latchKey( Key *k )
      * \brief Key latched.
      *
      * This signal is emitted on modifier keys when the state has been switched to latched.
      * It should be used to update all the other keys with a modified symbol.
      */
    void latchKey( Key *k );
    /*! \fn unlatchKey( Key *k )
      * \brief Key unlatched.
      *
      * This signal is emitted when a previously latched key is released.
      * It should be used to update the keyboard with non modified symbols.
      */
    void unlatchKey( Key *k );
    /*! \fn unlatchAll()
      * \brief Unlatch all keys.
      *
      * This is signal is emitted when a non modifier key has been pressed.
      * It notifies the keyboard that all latched keys should be unlatched.
      */
    void unlatchAll();

public slots:
    /*! \fn setStyle( Style *style )
      * \brief Changes the style used to paint the key.
      *
      * \param style The new style.
      */
    void setStyle( Style *style );
    /*! \fn redraw();
      * \brief repaints the key.
      */
    void redraw();

protected:
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );

private:
    Settings *settings;

    quint8 code;
    enum key_status status;

    QTimer releaseTimer;

    qreal width, height;
    QString keyShape;
    Keymap *keymap;

    bool hovered;
    QRectF bounds;
    QPainterPath keyBounds;

    QSvgRenderer *activeRenderer;
    QSvgRenderer *pressedRenderer;
    QSvgRenderer *latchedRenderer;

private slots:
    void release();
};

#endif // KEY_H
