//    This file is part of Florence Virtual Keyboard for Qt.
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

#include <QtPlugin>
#include <QFile>
#include <QDomElement>
#include <QPainter>
#include <QMouseEvent>
#include "florence.h"

Florence::Florence( QWidget *parent )
    : QGraphicsView( parent )
{
    this->focusKey = NULL;

    this->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    this->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    this->setStyleSheet( "background: transparent; border-style: none;" );
    this->setContentsMargins(0, 0, 0, 0);
    this->graphicsScene = new QGraphicsScene( this );
    this->setScene( this->graphicsScene );

    this->setMouseTracking( true );
    this->autoRepeatTimer = new QTimer( this );
    connect(this->autoRepeatTimer, SIGNAL(timeout()), this, SLOT(repeat()));

    this->settings = new Settings();
    this->setLayout( ":/florence.xml" );
}

Florence::~Florence()
{
    delete this->settings;
    foreach ( QGraphicsItem *it, this->scene()->items() ) {
        this->scene()->removeItem( it );
        Key *k = static_cast<Key *>(it);
        delete k;
    }
    delete this->graphicsScene;
}

bool Florence::setLayout( QString file )
{
    bool ok;
    QDomDocument layout;
    QFile f(file);

    if ( !f.open( QIODevice::ReadOnly ) ) return false;
    if ( !layout.setContent(&f) ) {
        f.close();
        return false;
    }
    f.close();

    QDomElement root = layout.documentElement();
    QDomElement keyboard = root.firstChildElement("keyboard");
    if (keyboard.isNull()) return false;
    QDomElement width = keyboard.firstChildElement("width");
    QDomElement height = keyboard.firstChildElement("height");
    if ( width.isNull() || height.isNull() ) return false;
    this->sceneWidth = width.text().toDouble(&ok);
    if (!ok) return false;
    this->sceneHeight = height.text().toDouble(&ok);
    if (!ok) return false;

    foreach ( QGraphicsItem *it, this->scene()->items() ) {
        this->scene()->removeItem( it );
        Key *k = static_cast<Key *>(it);
        delete k;
    }

    QDomElement key = keyboard.firstChildElement("key");
    while ( !key.isNull() ) {
        Key *k = new Key( key, this->settings );
        k->setStyle( this->settings->getStyle() );
        this->connect( k, SIGNAL(keyPressed(Symbol::symbol_role,QString)),
                       SLOT(keyPressed(Symbol::symbol_role, QString)) );
        this->connect( k, SIGNAL(latchKey(Key*)), SLOT(latchKey(Key*)) );
        this->connect( k, SIGNAL(unlatchKey(Key*)), SLOT(unlatchKey(Key*)) );
        this->connect( k, SIGNAL(unlatchAll()), SLOT(unlatchAll()) );
        this->scene()->addItem(k);
        key = key.nextSiblingElement("key");
    }

    return true;
}

bool Florence::setStyle( QString file )
{
    return this->settings->setStyle( file );
}

void Florence::setColor( enum StyleItem::style_colours color, QString value )
{
    this->settings->setColor( color, value );
}

void Florence::setOpacity( qreal opacity )
{
    this->settings->setOpacity( opacity );
}

void Florence::setFont( QString font )
{
    this->settings->setFont( font );
}

bool Florence::setKeymap( QString file )
{
    return this->settings->setKeymap( file );
}

void Florence::resizeEvent( QResizeEvent *event )
{
    QTransform matrix;
    matrix.scale(this->width()/this->scene()->width(), this->height()/this->scene()->height());
    this->setTransform( matrix, false );
    QGraphicsView::resizeEvent( event );
}

void Florence::mousePressEvent( QMouseEvent *event )
{
    this->mouseMoveEvent( event );
}

void Florence::mouseDoubleClickEvent( QMouseEvent *event )
{
    this->mouseMoveEvent( event );
}

void Florence::mouseMoveEvent( QMouseEvent *event )
{
    if ( event->buttons() > Qt::NoButton )  {
        Key *k = static_cast<Key *>( this->scene()->itemAt( this->mapToScene( event->pos() ) ) );
        if ( k != this->focusKey ) {
            this->autoRepeatTimer->stop();
            if ( this->focusKey ) this->focusKey->hoverLeaveEvent();
            this->focusKey = k;
            if ( k ) {
                k->hoverEnterEvent();
                this->autoRepeatTimer->start( 1000 );
            }
        }
    }
    QGraphicsView::mouseMoveEvent( event );
}

void Florence::repeat()
{
    if ( this->focusKey ) this->focusKey->press();
    this->autoRepeatTimer->stop();
    this->autoRepeatTimer->start(100);
}

void Florence::leaveEvent( QEvent *event )
{
    this->autoRepeatTimer->stop();
    if ( this->focusKey ) this->focusKey->hoverLeaveEvent();
    this->focusKey = NULL;
    QGraphicsView::leaveEvent( event );
}

void Florence::mouseReleaseEvent( QMouseEvent *event )
{
    //Key *k = static_cast<Key *>( this->scene()->itemAt( this->mapToScene( event->pos() ) ) );
    if ( this->focusKey ) this->focusKey->mouseReleaseEvent();
    if ( this->autoRepeatTimer->isActive() ) this->autoRepeatTimer->stop();
    this->focusKey = NULL;
    QGraphicsView::mouseReleaseEvent( event );
}

void Florence::latchKey( Key *key )
{
    this->latchedKeys.append( key );
}

void Florence::unlatchKey( Key *key )
{
    this->latchedKeys.remove( this->latchedKeys.indexOf( key ) );
}

void Florence::unlatchAll()
{
    bool unlatched = false;
    foreach ( Key *k, this->latchedKeys ) {
        unlatched = k->unlatch() || unlatched;
    }

    if ( unlatched ) foreach ( QGraphicsItem *it, this->scene()->items() ) {
        Key *k = static_cast<Key *>(it);
        k->update();
    }

    this->latchedKeys.clear();
}

void Florence::keyPressed( enum Symbol::symbol_role role, QString text )
{
    switch ( role ) {
        case Symbol::SYMBOL_TEXT:
            emit inputText( text );
            break;
        case Symbol::SYMBOL_BACKSPACE:
            emit backspace();
            break;
        case Symbol::SYMBOL_RETURN:
            emit enter();
            break;
        case Symbol::SYMBOL_TAB:
            emit tab();
            break;
        case Symbol::SYMBOL_LEFTTAB:
            emit leftTab();
            break;
        default:
            break;
    }
}
