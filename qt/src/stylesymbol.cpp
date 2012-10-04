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
#include "stylesymbol.h"

StyleSymbol::StyleSymbol( QDomElement el, QString colors[], QDomElement defs )
    : StyleItem ( el, colors, defs )
{
    if ( !this->getRenderer() ) {
        QDomElement label = el.firstChildElement("label");
        if ( !label.isNull() ) {
            this->label = label.text();
        }
    }
}

QString StyleSymbol::getLabel()
{
    return this->label;
}

void StyleSymbol::paint( QPainter *painter, QRectF &bounds )
{
    if ( this->getRenderer() ) {
        QRectF viewbox = this->getRenderer()->viewBoxF();
        qreal xoffset, yoffset, xscale, yscale;
        yscale = bounds.height() / viewbox.height();
        xscale = yscale * viewbox.width() / viewbox.height();
        yoffset = bounds.y();
        xoffset = bounds.x() + ( ( bounds.width() - ( viewbox.width() * xscale ) ) / 2.0 );
        painter->translate( xoffset, yoffset );
        painter->scale( xscale, yscale );
        this->getRenderer()->render( painter, viewbox );
    }
}
