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

#include "settings.h"

Settings::Settings()
{
    this->colors[StyleItem::STYLE_KEY_COLOR] = "#000000";
    this->colors[StyleItem::STYLE_MOUSE_OVER_COLOR] = "#0000FF";
    this->colors[StyleItem::STYLE_ACTIVATED_COLOR] = "#FF0000";
    this->colors[StyleItem::STYLE_LATCHED_COLOR] = "#00FF00";
    this->colors[StyleItem::STYLE_OUTLINE_COLOR] = "#808080";
    this->colors[StyleItem::STYLE_TEXT_COLOR] = "#FFFFFF";
    this->colors[StyleItem::STYLE_TEXT_OUTLINE_COLOR] = "#000000";

    this->font = "Sans";

    this->opacity = 1.0;

    this->style = new Style();
    this->keymap = new Keymap();
    this->setStyle( ":default.style" );
    this->setKeymap( ":/us.xml" );
}

Settings::~Settings()
{
    delete this->style;
    delete this->keymap;
}

qreal Settings::getOpacity()
{
    return this->opacity;
}

QString Settings::getFont()
{
    return this->font;
}

QString Settings::getColor( enum StyleItem::style_colours color )
{
    return this->colors[color];
}

QString *Settings::getColors()
{
    return this->colors;
}

Style *Settings::getStyle()
{
    return this->style;
}

Keymap *Settings::getKeymap()
{
    return this->keymap;
}

void Settings::setOpacity( qreal opacity )
{
    if ( opacity < 0.05 ) this->opacity = 0.05;
    else if ( opacity > 1.0 ) this->opacity = 1.0;
    else this->opacity = opacity;
    emit opacityChanged( this->opacity );
}

void Settings::setFont( QString font )
{
    this->font = font;
    emit fontChanged( this->font );
}

void Settings::setColor( enum StyleItem::style_colours color, QString value )
{
    this->colors[color] = value;
    this->style->setColor( color, this );
    emit colorChanged( color, value );
}

bool Settings::setStyle( QString file )
{
    if ( this->style->load( file, this ) ) {
        emit styleChanged( this->style );
        return true;
    } else return false;
}

bool Settings::setKeymap( QString file )
{
    if ( this->keymap->load( file, this ) ) {
        emit keymapChanged( this->keymap );
        return true;
    } else return false;
}
