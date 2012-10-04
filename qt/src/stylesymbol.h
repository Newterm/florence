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

#ifndef STYLESYMBOL_H
#define STYLESYMBOL_H

#include <QDomElement>
#include <QPainter>
#include <QSvgRenderer>
#include "styleitem.h"

/*! \class StyleSymbol
  * \brief A StyleSymbol describes the graphic reprensentation of a symbol.
  *
  * A StyleSymbol is a StyleItem that is used to draw the symbols of the keys.
  * A StyleSymbol has either a SVG or a label.
  * If a SVG is present, it is used to draw the symbol on the key.
  * Otherwise, the label is printed over the key.
  *
  * The symbol styles are loaded from the Style file.
  * \see StyleItem
  */
class StyleSymbol : public StyleItem
{
public:
    /*! \fn StyleSymbol( QDomElement el, QString colors[], QDomElement defs )
      * \brief Constructor.
      *
      * Instantiates a StyleSymbol object.
      * \param el This is the DOM element that comes from the style file.
      * \param colors This is an array of colors that are used in the css of the renderer.
      * \see StyleItem::style_colours
      * \param defs This is the DOM element of the global defs for the symbols.
      */
    StyleSymbol( QDomElement el, QString colors[], QDomElement defs );

    /*! \fn getLabel()
      * \returns The label of the symbol style if there is one.
      */
    QString getLabel();

    /*! \fn paint( QPainter *painter, QRectF &bounds )
      * \brief Renders the symbol.
      * \param painter A pointer to the QPainter objet to be used to render the symbol.
      * \param bounds A rectangle inside which the symbol must fit.
      */
    void paint( QPainter *painter, QRectF &bounds );

private:
    QString label;
};

#endif // STYLESYMBOL_H
