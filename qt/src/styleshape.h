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

#ifndef SHAPE_H
#define SHAPE_H

#include "styleitem.h"

/*! \class StyleShape
  * \brief A StyleShape describes the graphic reprensentation of a key.
  *
  * A StyleShape is a StyleItem that is used to draw the keys of the keyboard.
  * A StyleShape object has 4 renderers that can be used to draw the key depending on its state.
  * The StyleItem renderer is used to draw the key in the released state.
  * The active renderer is used to render the key that is focused.
  * The pressed renderer is used to render keys in the pressed and locked state.
  * Finally, the latched renderer can be used to render the keys in the latched state.
  * \see StyleItem
  */
class StyleShape : public StyleItem
{
public:
    /*! \fn StyleShape( QDomElement el, QString colors[], QDomElement defs )
      * \brief Constructor.
      *
      * Instantiates a StyleShape object.
      * \param el This is the DOM element that comes from the style file.
      * \param colors This is an array of colors that are used in the css of the renderer.
      * \see StyleItem::style_colours
      * \param defs This is the DOM element of the global defs for the shapes.
      */
    StyleShape( QDomElement el, QString colors[], QDomElement defs );
    /*! \fn ~StyleShape()
      * \brief Destructor.
      *
      * Destroys the object.
      */
    ~StyleShape();

    /*! \fn setColors( QString colors[], enum style_colours key_color )
      * \brief sets the colors of the shape
      *
      * Changes the colors used in the css to render the shape.
      * \param colors This is an array of colors that are used in the css of the renderer.
      * \param key_color Either STYLE_KEY_COLOR, STYLE_LATCHED_COLOR, STYLE_ACTIVATED_COLOR or STYLE_MOUSE_OVER_COLOR.
      *                  Defines the color to use in the background of the key.
      */
    void setColors( QString colors[], enum StyleItem::style_colours key_color );

    /*! \fn getActiveRenderer()
      * \returns The active renderer.
      */
    QSvgRenderer *getActiveRenderer();
    /*! \fn getPressedRenderer()
      * \returns The pressed renderer.
      */
    QSvgRenderer *getPressedRenderer();
    /*! \fn getLatchedRenderer()
      * \returns The latched renderer.
      */
    QSvgRenderer *getLatchedRenderer();

private:
    QSvgRenderer *activeRenderer;
    QSvgRenderer *pressedRenderer;
    QSvgRenderer *latchedRenderer;
};

#endif // SHAPE_H
