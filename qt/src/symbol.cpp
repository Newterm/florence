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

#include "symbol.h"
#include "styleshape.h"
#include "style.h"
#include "settings.h"

Symbol::Symbol( QDomElement el, Settings *settings )
{
    QDomElement name = el.firstChildElement("name");
    if ( name.isNull() ) {
        this->name = el.text();
        this->role = SYMBOL_TEXT;
    } else {
        this->name = name.text();
        if ( this->name == "BackSpace" ) {
            this->role = SYMBOL_BACKSPACE;
        } else if ( this->name == "Return" ) {
            this->role = SYMBOL_RETURN;
        } else if ( this->name == "Tab" ) {
            this->role = SYMBOL_TAB;
        } else if ( this->name == "ISO_Left_Tab" ) {
            this->role = SYMBOL_LEFTTAB;
        } else {
            this->role = SYMBOL_DEAD;
        }
    }
    this->renderer = NULL;
    this->settings = settings;
    this->connect( this->settings, SIGNAL(styleChanged(Style*)), SLOT(setStyle(Style*)) );
}

void Symbol::setStyle( Style *style )
{
    this->renderer = style->getSymbol( this->name );
}

QString Symbol::getName()
{
    return this->name;
}

enum Symbol::symbol_role Symbol::getRole()
{
    return this->role;
}

void Symbol::paint( QPainter *painter, QRectF &bounds, bool hovered )
{
    if ( this->renderer && this->renderer->getRenderer() ) {
        this->renderer->paint( painter, bounds );
    } else {
        QString text;
        if ( this->renderer ) text = this->renderer->getLabel();
        else if ( this->role == SYMBOL_TEXT ) text = this->getName();
        else return;

        qreal z = 1.0;
        if ( hovered ) z = 1.2;

        QPainterPath textPath;
        QFont font( this->settings->getFont(), 10 );
        textPath.addText(0, 0, font, text);
        painter->translate( bounds.x() + ( ( bounds.width() - ( textPath.boundingRect().width()*0.05*z ) ) / 2.0  ),
                            bounds.y() + ( bounds.height() * 0.6 ));
        painter->scale( z*0.05, z*0.05 );
        painter->setRenderHint( QPainter::Antialiasing );
        QPen p( QColor( this->settings->getColor( StyleItem::STYLE_TEXT_OUTLINE_COLOR ) ) );
        p.setWidth( 2 );
        painter->strokePath( textPath, p );
        painter->fillPath( textPath, QBrush( QColor( this->settings->getColor( StyleItem::STYLE_TEXT_COLOR ) ) ) );
    }
}

ModifiedSymbol::ModifiedSymbol( QDomElement el, Settings *settings )
    : Symbol( el, settings )
{
    this->modifier = el.attribute("mod").toInt();
}

quint8 ModifiedSymbol::getModifier()
{
    return this->modifier;
}
