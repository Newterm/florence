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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "style.h"
#include "keymap.h"

/*! \class Settings
  * \brief This class records all the settings of the Florence widget.
  *
  * There is a getter and a setter per configuration option.
  * The Settings object emits signals when a configuration value changes to notify the other interested objects of the change.
  * The configuration options are:
  * <ul>
  *    <li><b>Opacity</b>: The transparency of the widget. This is a real number from 0 (fully transparent) to 1 (opaque).</li>
  *    <li><b>Font</b>: The font of the key symbols. This is the font used to display the symbols over the keys.</li>
  *    <li><b>Colors</b>: The colors used to draw the keyboard.</li>
  *    <li><b>Style</b>: The style used to draw the keyboard.</li>
  *    <li><b>Keymap</b>: The language dependant Keymap used to map the symbols to the key codes in the layout.</li>
  * </ul>
  * Note it is not necessary to put the layout in the Settings object because only the Florence object is interested in this option.
  */
class Settings : public QObject
{
    Q_OBJECT

public:
    /*! \fn Settings()
      * \brief Constructor.
      *
      * Instantiates a Settings object.
      */
    Settings();
    /*! \fn ~Settings()
      * \brief Destructor.
      *
      * Destroys a Settings object.
      */
    ~Settings();

    /*! \fn getOpacity()
      * \returns The opacity of the widget.
      */
    qreal getOpacity();
    /*! \fn getFont()
      * \returns The font of the symbols.
      */
    QString getFont();
    /*! \fn getColor( enum StyleItem::style_colours color )
      * \param color The style element to get the color of.
      * \returns The color of the style item.
      */
    QString getColor( enum StyleItem::style_colours color );
    /*! \fn getColors()
      * \returns An array of colors representing all the configurable colors.
      */
    QString *getColors();
    /*! \fn getStyle()
      * \returns The style used to draw the keyboard.
      */
    Style *getStyle();
    /*! \fn getKeymap()
      * \returns The keymap used to associate key codes to symbols.
      */
    Keymap *getKeymap();

    /*! \fn setOpacity( qreal opacity )
      * \param opacity The opacity of the widget.
      */
    void setOpacity( qreal opacity );
    /*! \fn setFont( QString font )
      * \param font The font of the symbols.
      */
    void setFont( QString font );
    /*! \fn setColor( enum StyleItem::style_colours color, QString value )
      * \param color The style element to set the color to.
      * \param value A string that represents the value of the color in one of those format:
      * <ul>
      *    <li> \#RGB ( \#0FF )
      *    <li> \#RRGGBB ( \#00FFFF )
      *    <li> \#RRRRGGGGBBBB ( \#0000FFFFFFFF )
      *    <li> color name ( cyan )
      *    <li> transparent
      * </ul>
      */
    void setColor( enum StyleItem::style_colours color, QString value );
    /*! \fn setStyle( QString file )
      * \param file The new style file name.
      * \see Style
      */
    bool setStyle( QString file );
    /*! \fn setKeymap( QString file )
      * \param file The new Keymap file name.
      * \see Keymap
      */
    bool setKeymap( QString file );

signals:
    /*! \fn colorChanged( enum StyleItem::style_colours color, QString value )
      * \brief This signal is emitted when a color has been changed with the Settings::setColor() method.
      * \see Settings::setColor()
      */
    void colorChanged( enum StyleItem::style_colours color, QString value );
    /*! \fn styleChanged( Style *style )
      * \brief This signal is emitted when the style has been changed with the Settings::setStyle() method.
      * \see Settings::setStyle()
      */
    void styleChanged( Style *style );
    /*! \fn keymapChanged( Keymap *keymap )
      * \brief This signal is emitted when the keymap has been changed with the Settings::setKeymap() method.
      * \see Settings::setKeymap()
      */
    void keymapChanged( Keymap *keymap );
    /*! \fn opacityChanged( qreal opacity )
      * \brief This signal is emitted when the opacity has been changed with the Settings::setOpacity() method.
      * \see Settings::setKeymap()
      */
    void opacityChanged( qreal opacity );
    /*! \fn fontChanged( QString font )
      * \brief This signal is emitted when the font has been changed with the Settings::setFont() method.
      * \see Settings::setFont()
      */
    void fontChanged( QString font );

private:
    qreal opacity;
    QString font;
    QString colors[StyleItem::STYLE_NUM_COLOURS];
    Style *style;
    Keymap *keymap;
};

#endif // SETTINGS_H
