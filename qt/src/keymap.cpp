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

#include <QDomElement>
#include <QFile>
#include "keymap.h"
#include "symbol.h"
#include "settings.h"

Keymap::Keymap()
{
    for ( int i = 0 ; i < 256 ; i++ ) {
        this->keys[i] = NULL;
    }
    this->modifier = 0;
}

Keymap::~Keymap()
{
    for ( int i = 0 ; i < 256 ; i++ ) {
        if ( this->keys[i] ) delete this->keys[i];
    }
}

bool Keymap::load( QString file, Settings *settings )
{
    QDomDocument keymap;
    QFile f(file);

    if ( !f.open( QIODevice::ReadOnly ) ) return false;
    if ( !keymap.setContent(&f) ) {
        f.close();
        return false;
    }
    f.close();

    for ( int i = 0 ; i < 256 ; i++ ) {
        if ( this->keys[i] ) {
            delete this->keys[i];
            this->keys[i] = NULL;
        }
    }

    QDomElement root = keymap.documentElement();
    if (root.isNull()) return false;

    QDomElement key = root.firstChildElement("key");
    while ( !key.isNull() ) {
        quint8 code = key.attribute("code").toInt();
        this->keys[code] = new KeymapKey( key, settings );
        key = key.nextSiblingElement("key");
    }

    this->modifier = 0;
    return true;
}

Symbol *Keymap::getSymbol( quint8 code )
{
    if ( this->keys[code] ) {
        return this->keys[code]->getSymbol( this->modifier );
    } else return NULL;
}

quint8 Keymap::getKeyModifier( quint8 code )
{
    return this->keys[code]->getModifier();
}

bool Keymap::isLocker( quint8 code )
{
    return this->keys[code]->isLocker();
}

void Keymap::addModifier( quint8 mod )
{
    this->modifier |= mod;
}

void Keymap::removeModifier( quint8 mod )
{
    this->modifier -= mod;
}

KeymapKey::KeymapKey( QDomElement el, Settings *settings )
{
    if ( el.attribute("mod").isNull() ) {
        this->modifier = 0;
    } else {
        this->modifier = el.attribute("mod").toInt();
    }
    this->locker = !el.attribute("locker").isNull();
    QDomElement symbol = el.firstChildElement("symbol");
    while ( !symbol.isNull() ) {
        ModifiedSymbol *s = new ModifiedSymbol( symbol, settings );
        s->setStyle( settings->getStyle() );
        this->symbols.append( s );
        symbol = symbol.nextSiblingElement("symbol");
    }
}

KeymapKey::~KeymapKey()
{
    foreach( ModifiedSymbol *s, this->symbols ) {
        delete s;
    }
}

ModifiedSymbol *KeymapKey::getSymbol( quint8 mod )
{
    int score = -1;
    ModifiedSymbol *ret = NULL;

    foreach( ModifiedSymbol *s, this->symbols ) {
        if ( ( mod & s->getModifier() ) > score ) {
            ret = s;
            score = ( mod & ret->getModifier() );
        }
    }

    return ret;
}

quint8 KeymapKey::getModifier()
{
    return this->modifier;
}

bool KeymapKey::isLocker()
{
    return this->locker;
}
