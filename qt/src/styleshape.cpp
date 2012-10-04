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

#include "styleshape.h"

StyleShape::StyleShape( QDomElement el, QString colors[], QDomElement defs )
        : StyleItem ( el, colors, defs )
{
    this->activeRenderer = new QSvgRenderer( this->makeSvg( colors, STYLE_MOUSE_OVER_COLOR ).toAscii() );
    this->pressedRenderer = new QSvgRenderer( this->makeSvg( colors, STYLE_ACTIVATED_COLOR ).toAscii() );
    this->latchedRenderer = new QSvgRenderer( this->makeSvg( colors, STYLE_LATCHED_COLOR ).toAscii() );
}

StyleShape::~StyleShape()
{
    delete this->activeRenderer;
    delete this->pressedRenderer;
    delete this->latchedRenderer;
}

QSvgRenderer *StyleShape::getActiveRenderer()
{
    return this->activeRenderer;
}

QSvgRenderer *StyleShape::getPressedRenderer()
{
    return this->pressedRenderer;
}

QSvgRenderer *StyleShape::getLatchedRenderer()
{
    return this->latchedRenderer;
}

void StyleShape::setColors( QString colors[], enum style_colours key_color )
{
    switch( key_color ) {
        case STYLE_MOUSE_OVER_COLOR:
            this->activeRenderer->load( this->makeSvg( colors, STYLE_MOUSE_OVER_COLOR ).toAscii() );
            break;
        case STYLE_ACTIVATED_COLOR:
            this->pressedRenderer->load( this->makeSvg( colors, STYLE_ACTIVATED_COLOR ).toAscii() );
            break;
        case STYLE_LATCHED_COLOR:
            this->latchedRenderer->load( this->makeSvg( colors, STYLE_LATCHED_COLOR ).toAscii() );
            break;
        case STYLE_KEY_COLOR:
            StyleItem::setColors( colors, STYLE_KEY_COLOR );
            break;
	default:
	    break;
    }
}
