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

#include <QDebug>
#include <QBitmap>
#include <QGraphicsSceneHoverEvent>
#include "key.h"
#include "florence.h"

Key::Key( QDomElement el, Settings *settings )
    : QGraphicsSvgItem()
{
    bool ok;
    QDomElement code = el.firstChildElement("code");
    this->code = code.text().toInt(&ok);
    QDomElement xpos = el.firstChildElement("xpos");
    qreal x = xpos.text().toDouble(&ok);
    QDomElement ypos = el.firstChildElement("ypos");
    qreal y = ypos.text().toDouble(&ok);
    QDomElement width = el.firstChildElement("width");
    if (width.isNull()) this->width = 2.0;
    else this->width = width.text().toDouble(&ok);
    QDomElement height = el.firstChildElement("height");
    if (height.isNull()) this->height = 2.0;
    else this->height = height.text().toDouble(&ok);
    this->setPos( x, y );
    bounds = QRectF(-this->width/2.0, -(this->height/2.0), this->width, this->height);

    QDomElement shape = el.firstChildElement("shape");
    if (shape.isNull()) this->keyShape = "default";
    else this->keyShape = shape.text();
    QDomElement bounds = el.firstChildElement("bounds");
    if (bounds.isNull()) {
        this->keyBounds.moveTo( -this->width/2.0, -this->height/2.0 );
        this->keyBounds.lineTo( this->width/2.0, -this->height/2.0 );
        this->keyBounds.lineTo( this->width/2.0, this->height/2.0 );
        this->keyBounds.lineTo( -this->width/2.0, this->height/2.0 );
    } else {
        QDomElement a = bounds.firstChildElement();
        while ( !a.isNull() ) {
            qreal x = a.attribute("x").toDouble();
            qreal y = a.attribute("y").toDouble();
            if ( a.tagName() == "m" )
                this->keyBounds.moveTo( x - (this->width/2.0), y - (this->height/2.0) );
            else if ( a.tagName() == "l" )
                this->keyBounds.lineTo( x - (this->width/2.0), y - (this->height/2.0) );
            a = a.nextSiblingElement();
        }
    }

    this->setAcceptHoverEvents( true );
    this->hovered = false;

    this->activeRenderer = NULL;
    this->pressedRenderer = NULL;
    this->latchedRenderer = NULL;

    this->status = KEY_RELEASED;
    this->connect( &(this->releaseTimer), SIGNAL(timeout()), this, SLOT(release()) );

    this->settings = settings;
    this->connect( settings, SIGNAL(styleChanged(Style*)), SLOT(setStyle(Style*)) );
    this->connect( settings, SIGNAL(keymapChanged(Keymap*)), SLOT(redraw()) );
    this->connect( settings, SIGNAL(colorChanged(enum StyleItem::style_colours,QString)), SLOT(redraw()) );
    this->connect( settings, SIGNAL(opacityChanged(qreal)), SLOT(redraw()) );
    this->connect( settings, SIGNAL(fontChanged(QString)), SLOT(redraw()) );
}

void Key::setStyle( Style *style )
{
    if ( style->getShape( this->keyShape ) ) {
        this->setSharedRenderer( style->getShape( this->keyShape )->getRenderer() );
        this->activeRenderer = style->getShape( this->keyShape )->getActiveRenderer();
        this->pressedRenderer = style->getShape( this->keyShape )->getPressedRenderer();
        this->latchedRenderer = style->getShape( this->keyShape )->getLatchedRenderer();
    }
    this->update();
}

void Key::redraw()
{
    this->update();
}

QRectF Key::boundingRect() const
{
    return bounds;
}

QPainterPath Key::shape() const
{
    return this->keyBounds;
}

void Key::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    if ( !this->hovered ) {
        this->bounds = QRectF( -(this->width/2.0), -(this->height/2.0), this->width, this->height );
    }

    painter->save();
    QRectF viewbox = this->renderer()->viewBoxF();

    painter->translate( this->bounds.x(), this->bounds.y());
    painter->scale( this->bounds.width()/viewbox.width(), this->bounds.height()/viewbox.height() );

    switch ( this->status ) {
        case KEY_RELEASED:
            if ( hovered ) {
                if ( !this->activeRenderer ) return;
                this->activeRenderer->render( painter, viewbox );
            } else {
                if ( !this->renderer() ) return;
                QGraphicsSvgItem::paint( painter, option, widget );
            }
            break;
        case KEY_PRESSED:
        case KEY_LOCKED:
            if ( !this->pressedRenderer ) return;
            this->pressedRenderer->render( painter, viewbox );
            break;
        case KEY_LATCHED:
            if ( !this->latchedRenderer ) return;
            this->latchedRenderer->render( painter, viewbox );
            break;
    }

    painter->setCompositionMode( QPainter::CompositionMode_DestinationOut );
    painter->fillRect( -3, -3, viewbox.width() + 6, viewbox.height() + 6, QColor( 0, 0, 0, (1.0-this->settings->getOpacity()) * 255.0 ) );

    painter->restore();
    Symbol *s;
    if ( ( s = this->settings->getKeymap()->getSymbol(this->code) ) )
        s->paint( painter, this->bounds, hovered );
}

void Key::hoverEnterEvent ()
{
    this->hovered = true;
    this->setZValue(1);
    this->bounds = QRectF(-(this->width*1.2/2.0), -(this->height*1.2/2.0), this->width*1.2, this->height*1.2);
    if (this->x() + this->bounds.x()<0) this->bounds.moveLeft(-this->width/2.0);
    if (this->y() + this->bounds.y()<0) this->bounds.moveTop(-this->height/2.0);
    if (this->x() + this->bounds.right()>this->scene()->width()) this->bounds.moveLeft(-this->width*1.4/2.0);
    if (this->y() + this->bounds.bottom()>this->scene()->height()) this->bounds.moveTop(-this->height*1.4/2.0);
    this->update();
}

void Key::hoverLeaveEvent ()
{
    this->hovered = false;
    this->setZValue(0);
    this->update();
}

void Key::mouseReleaseEvent ()
{
    quint8 mod = this->settings->getKeymap()->getKeyModifier( this->code );

    if ( mod > 0 ) {
        bool update = false;
        switch ( this->status ) {
            case KEY_RELEASED:
                this->settings->getKeymap()->addModifier( mod );
                if ( this->settings->getKeymap()->isLocker( this ->code ) ) {
                    this->status = KEY_LOCKED;
                } else {
                    this->status = KEY_LATCHED;
                    emit latchKey( this );
                }
                update = true;
                break;
            case KEY_LATCHED:
                this->status = KEY_LOCKED;
                emit unlatchKey( this );
                this->update();
                break;
            case KEY_LOCKED:
	    case KEY_PRESSED: // Should not happen
                this->settings->getKeymap()->removeModifier( mod );
                this->status = KEY_RELEASED;
                update = true;
                break;
        }

        if ( update ) {
            QList<QGraphicsItem *> list = this->scene()->items();
            QList<QGraphicsItem *>::iterator it;
            for (it = list.begin(); it != list.end(); ++it) {
                (*it)->update();
            }
        }

        this->hovered = false;
        this->setZValue(0);
    } else {
        this->press();
    }
}

bool Key::unlatch()
{
    bool ret = false;
    if ( this->status == KEY_LATCHED ) {
        this->settings->getKeymap()->removeModifier( this->settings->getKeymap()->getKeyModifier( this->code ) );
        this->status = KEY_RELEASED;
        ret = true;
    }
    return ret;
}

void Key::press()
{
    if ( this->settings->getKeymap()->getKeyModifier( this->code ) == 0 ) {
        Symbol *s = this->settings->getKeymap()->getSymbol( this->code );
        emit keyPressed( s->getRole(), s->getName() );
        this->hovered = true;
        this->setZValue(1);
        this->status = KEY_PRESSED;
        this->update();
        emit unlatchAll();
        releaseTimer.start(200);
    }
}

void Key::release()
{
    if ( releaseTimer.isActive() ) releaseTimer.stop();
    this->hovered = false;
    this->setZValue(0);
    this->status = KEY_RELEASED;
    this->update();
}
