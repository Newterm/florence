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

#ifndef STYLEITEM_H
#define STYLEITEM_H

#include <QDomElement>
#include <QSvgRenderer>

/*! \class StyleItem
  * \brief This class represents a style element
  *
  * This is the base class for style elements.
  * There are two elements that inherits from this class: StyleSymbol and StyleShape.
  *
  * A style element has a QSvgRenderer that can be used to paint the element on a QPaintDevice.
  * The setColors method can be used to change the colors of the renderer.
  *
  * Style items are loaded from elements of the Style file.
  * The XML elements are structured as follow:
  * \code
  * <element>
  *    <name>name</name>
  *    <svg>svg</svg>
  * <element>
  * \endcode
  * <i>element</i> is the super element that inherit from StyleItem. It is either a shape or a symbol.
  * <i>name</i> is the name of the item.
  * <i>svg</i> is the optional svg description of the item.
  * \see StyleShape
  * \see StyleSymbol
  */
class StyleItem : public QObject
{
    Q_OBJECT

public:
    /*! \enum style_colours
      * Defines the style elements with user definable color.
      */
    enum style_colours {
            STYLE_KEY_COLOR, /*!< color of the background of the key */
            STYLE_OUTLINE_COLOR, /*!< color of the outline of the key */
            STYLE_ACTIVATED_COLOR, /*!< color of the background of the key when activated */
            STYLE_LATCHED_COLOR, /*!< color of the background of the key when latched */
            STYLE_TEXT_COLOR, /*!< color of the symbol displayed on the key */
            STYLE_TEXT_OUTLINE_COLOR, /*!< color of the outline of the symbol displayed on the key */
            STYLE_MOUSE_OVER_COLOR, /*!< color of the background of the key when focused */
            STYLE_NUM_COLOURS /*!< number of color elements */
    };

    /*! \fn StyleItem( QDomElement el, QString colors[], QDomElement defs )
      * \brief Constructor
      *
      * Instantiates a StyleItem object.
      * A StyleItem contains a QSvgRenderer that is instantiated from the svg element of the XML style file.
      * The DOM element must contain a name element and may contain a svg element.
      * The SVG is rendered using a css that defines the color of each element.
      * The SVG can use the elements defined in the global defs element.
      * \param el This is the DOM element that comes from the style file.
      * \param colors This is an array of colors that are used in the css of the renderer.
      * \see style_colours
      * \param defs This is the DOM element of the global defs for the style.
      */
    StyleItem( QDomElement el, QString colors[], QDomElement defs );
    /*! \fn ~StyleItem()
      * \brief Destructor
      * Destroys the object.
      */
    ~StyleItem();

    /*! \fn setColors( QString colors[], enum style_colours key_color )
      * \brief sets the colors of the item.
      *
      * Changes the colors used in the css to render the svg.
      * \param colors This is an array of colors that are used in the css of the renderer.
      * \param key_color Either STYLE_KEY_COLOR, STYLE_LATCHED_COLOR, STYLE_ACTIVATED_COLOR or STYLE_MOUSE_OVER_COLOR.
      *                  Defines the color to use in the background of the key.
      *                  Not used for StyleSymbol objects.
      */
    void setColors( QString colors[], enum style_colours key_color );

    /*! \fn getName()
      * \returns The name of the item.
      */
    QString getName();
    /*! \fn getSvg()
      * \returns The SVG description of the item.
      */
    QString getSvg();
    /*! \fn getRenderer()
      * \returns A SVG renderer that can be used to draw the item on a QPaintDevice.
      */
    QSvgRenderer *getRenderer();

protected:
    QString makeSvg( QString colors[], enum style_colours key_color );

private:
    QString name;
    QString svg;
    QSvgRenderer *renderer;
};

#endif // STYLEITEM_H
