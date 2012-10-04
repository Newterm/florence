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

#include <QTextStream>
#include "styleitem.h"

static const QString css (""
    "<style type=\"text/css\">\n"
    ".shape {\n"
    "    fill: @KEY_COLOR@;\n"
    "    stroke: @OUTLINE_COLOR@;\n"
    "    stroke-opacity: 0.35;\n"
    "    stroke-width: 4px;\n"
    "}\n"
    "\n"
    ".symbol {\n"
    "    fill: @TEXT_COLOR@;\n"
    "    stroke: @TEXT_OUTLINE_COLOR@;\n"
    "    stroke-width: 2px;\n"
    "}\n"
    "</style>");

StyleItem::StyleItem( QDomElement el, QString colors[], QDomElement defs )
{
    QDomElement name = el.firstChildElement("name");
    this->name = name.text();
    QDomElement svg = el.firstChildElement("svg");
    if ( svg.isNull() ) {
        this->renderer = NULL;
    } else {
        if ( !defs.isNull() ) svg.insertBefore( defs, QDomNode() );
        QDomDocument doc;
        doc.setContent( css );
        svg.insertBefore( doc, QDomNode() );
        QTextStream stream(&(this->svg));
        svg.save(stream, 0);
        this->renderer = new QSvgRenderer( this->makeSvg( colors, STYLE_KEY_COLOR ).toAscii() );
    }
}

StyleItem::~StyleItem()
{
    delete this->renderer;
}

QString StyleItem::makeSvg( QString colors[], enum style_colours key_color )
{
    QString tmp = this->svg;
    tmp = tmp.replace( "@KEY_COLOR@", colors[key_color] );
    tmp = tmp.replace( "@OUTLINE_COLOR@", colors[STYLE_OUTLINE_COLOR]);
    tmp = tmp.replace( "@TEXT_COLOR@", colors[STYLE_TEXT_COLOR]);
    tmp = tmp.replace( "@TEXT_OUTLINE_COLOR@", colors[STYLE_TEXT_OUTLINE_COLOR]);
    return tmp;
}

void StyleItem::setColors( QString colors[], enum style_colours key_color )
{
    if (this->renderer) this->renderer->load( this->makeSvg( colors, key_color ).toAscii() );
}

QString StyleItem::getName()
{
    return this->name;
}

QString StyleItem::getSvg()
{
    return this->svg;
}

QSvgRenderer *StyleItem::getRenderer()
{
    return this->renderer;
}

