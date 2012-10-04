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

#include "style.h"
#include "settings.h"
#include <QDomElement>
#include <QVectorIterator>

Style::Style()
{
}

Style::~Style()
{
    this->unload();
}

bool Style::load( QString file, Settings *settings )
{
    QDomDocument style;
    QFile f(file);

    if ( !f.open( QIODevice::ReadOnly ) ) return false;
    if ( !style.setContent(&f) ) {
        f.close();
        return false;
    }
    f.close();

    this->unload();

    QDomElement root = style.documentElement();
    QDomElement shapes = root.firstChildElement( "shapes" );
    if (!shapes.isNull()) {
        QDomElement defs = shapes.firstChildElement("defs");
        QDomElement shape = shapes.firstChildElement( "shape" );
        while ( !shape.isNull() ) {
            StyleShape *s = new StyleShape( shape, settings->getColors(), defs );
            this->shapes.append( s );
            shape = shape.nextSiblingElement( "shape" );
        }
    }

    QDomElement symbols = root.firstChildElement( "symbols" );
    if (!symbols.isNull()) {
        QDomElement defs = shapes.firstChildElement("defs");
        QDomElement symbol = symbols.firstChildElement( "symbol" );
        while ( !symbol.isNull() ) {
            StyleSymbol *s = new StyleSymbol( symbol, settings->getColors(), defs );
            this->symbols.append( s );
            symbol = symbol.nextSiblingElement( "symbol" );
        }
    }

    return true;
}

void Style::unload()
{
    if ( this->shapes.count() > 0 ) {
        QVectorIterator<StyleShape *> it(this->shapes);
        do {
            StyleShape *s = it.next();
            delete s;
        } while (it.hasNext());
        this->shapes.clear();
    }
    if ( this->symbols.count() > 0 ) {
        QVectorIterator<StyleSymbol *> it(this->symbols);
        do {
            StyleSymbol *s = it.next();
            delete s;
        } while (it.hasNext());
        this->symbols.clear();
    }
}

void Style::setColor( enum StyleItem::style_colours color, Settings *settings )
{
    switch ( color ) {
        case StyleItem::STYLE_OUTLINE_COLOR:
            foreach ( StyleShape *s, this->shapes ) {
                s->setColors( settings->getColors(), StyleItem::STYLE_KEY_COLOR );
                s->setColors( settings->getColors(), StyleItem::STYLE_ACTIVATED_COLOR );
                s->setColors( settings->getColors(), StyleItem::STYLE_LATCHED_COLOR );
            }
            break;
        case StyleItem::STYLE_KEY_COLOR:
        case StyleItem::STYLE_ACTIVATED_COLOR:
        case StyleItem::STYLE_LATCHED_COLOR:
        case StyleItem::STYLE_MOUSE_OVER_COLOR:
            foreach ( StyleShape *s, this->shapes ) {
                s->setColors( settings->getColors(), color );
            }
            break;
        case StyleItem::STYLE_TEXT_COLOR:
        case StyleItem::STYLE_TEXT_OUTLINE_COLOR:
            foreach ( StyleSymbol *s, this->symbols ) {
                s->setColors( settings->getColors(), StyleItem::STYLE_KEY_COLOR );
            }
            break;
    	default:
	    break;
    }
}

StyleShape *Style::getShape( QString name )
{
    QVectorIterator<StyleShape *> it(this->shapes);
    StyleShape *s = it.next();
    while ( s && it.hasNext() && ( s->getName() != name ) ) {
        s = it.next();
    }
    return s ? ( s->getName() == name ? s : NULL ) : NULL;
}

StyleSymbol *Style::getSymbol( QString name )
{
    if ( name.length() == 0 ) return NULL;
    QVectorIterator<StyleSymbol *> it(this->symbols);
    StyleSymbol *s, *ret = NULL;
    do {
        s = it.next();
        if ( s ) {
            QRegExp re( s->getName() );
            if ( re.exactMatch( name ) ) {
                ret = s;
                break;
            }
        }
    } while ( s && it.hasNext() );
    return ret;
}
