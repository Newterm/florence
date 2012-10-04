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

#ifndef STYLE_H
#define STYLE_H

#include <QFile>
#include <QVector>
#include "styleshape.h"
#include "stylesymbol.h"

class Settings;

/*! \class Style
  * \brief Represents the Style used to paint the keyboard.
  *
  * The keyboard uses a style file to paint itself.
  * This files defines the SVG elements used to draw each item.
  * There are two kinds of items: the shapes are used to draw the background of the key and the symbols are used to draw the symbol over the key.
  * The style file is an XML file structured as follow:
  * \code
  * <style>
  *    <shapes>
  *       <defs>SVG defs</defs>
  *       list of shapes
  *    </shapes>
  *    <symbols>
  *       <defs>SVG defs</defs>
  *       list of symbols
  *    </symbols>
  * </style>
  * \endcode
  * where:
  * <ul>
  *    <li><i>SVG defs</i> is the svg definitions to include in each svg element in the current node.</li>
  *    <li><i>list of shapes</i> is a list of shape elements. See the StyleShape class for a complete definition of the shape elements.</li>
  *    <li><i>list of symbols</i> is a list of symbol elements. See the StyleSymbol class for a complete definition of the symbol elements.</li>
  * </ul>
  * The SVG defs are useful not to repeat the same SVG definitions in every single element of the node.
  * \see StyleShape
  * \see StyleSymbol
  */
class Style : public QObject
{
    Q_OBJECT

public:
    /*! \fn Style()
      * \brief Constructor.
      *
      * Instantiates a Style object.
      */
    Style();
    /*! \fn ~Style()
      * \brief Destructor.
      *
      * Destroys a Style object.
      */
    ~Style();

    /*! \fn load( QString file, Settings *settings )
      * \brief Loads the Style from a XML description file.
      * \param file The file name of the XML style description.
      * \param settings The settings object used to get the colors.
      * \returns true for success, false for failure.
      */
    bool load( QString file, Settings *settings );
    /*! \fn setColor( enum StyleItem::style_colours color, Settings *settings )
      * Updates the color of an element.
      * \param color The style element to set the color of.
      * \param settings The settings object used to change the color.
      */
    void setColor( enum StyleItem::style_colours color, Settings *settings );

    /*! \fn getShape( QString name )
      * \brief Returns a StyleShape object from its name.
      * \param name The name of the shape, as defined in the XML style description.
      * \returns A pointer to the StyleShape with a matching name.
      */
    StyleShape *getShape( QString name );
    /*! \fn getSymbol( QString name )
      * \brief Returns a StyleSymbol object from its name.
      * \param name The name of the symbol, as defined in the XML style description.
      * \returns A pointer to the StyleSymbol with a matching name.
      */
    StyleSymbol *getSymbol( QString name );

private:
    QVector<StyleShape *> shapes;
    QVector<StyleSymbol *> symbols;

    void unload();
};

#endif // STYLE_H
