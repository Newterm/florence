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

#ifndef SYMBOL_H
#define SYMBOL_H

#include <QString>
#include <QDomElement>
#include "stylesymbol.h"

class Settings;
class Style;

/*! \class Symbol
  * \brief This class represents a key symbol.
  *
  * Key symbols have a name and a role.
  * The role determines the effect of keys.
  * Key symbols are loaded from the Keymap file.
  * A key symbol in the Keymap file can have 2 different structures:
  * \code
  * <symbol mod="modifier">name of the symbol</symbol>
  * \endcode
  * or
  * \code
  * <symbol mod="modifier"><name>name of the symbol</name></symbol>
  * \endcode
  * the <i>modifier</i> attribute is not handled in this class but in the ModifiedSymbol subclass.
  * In the first case, the symbol is a "text" symbol, which means its effect is to input text.
  * In the second case, the symbol is either BackSpace, Return, Tab, LeftTab or a dead symbol.
  * Dead symbols have no effect.
  * \see ModifiedSymbol
  */
class Symbol : public QObject
{
    Q_OBJECT

public:
    /*! \enum symbol_role
      * These constants represent the possible symbol effects.
      */
    enum symbol_role {
        SYMBOL_DEAD, /*!< The symbol has no effect and is ignored. */
        SYMBOL_TEXT, /*!< The symbol's effect is to input text. */
        SYMBOL_BACKSPACE, /*!< The symbol is the backspace symbol. */
        SYMBOL_RETURN, /*!< The symbol is the return symbol. */
        SYMBOL_TAB, /*!< The symbol is the tab symbol. */
        SYMBOL_LEFTTAB /*!< The symbol is the leftTab symbol. */
    };

    /*! \fn Symbol( QDomElement el, Settings *settings )
      * \brief Constuctor.
      *
      * Instantiates a Symbol object.
      * \param el Pointer to the parsed XML document containing the symbol description.
      * \param settings Pointer to the settings object of the Keyboard.
      */
    Symbol( QDomElement el, Settings *settings );

    /*! \fn getName()
      * \returns The name of the symbol.
      */
    QString getName();
    /*! \fn getRole()
      * \returns The role of the symbol.
      */
    enum Symbol::symbol_role getRole();

    /*! \fn paint( QPainter *painter, QRectF &bounds, bool hovered )
      * \brief Renders the Symbol on a QPaintDevice.
      * \param painter The painter object used to render the symbol.
      * \param bounds A rectangle inside which the symbol must fit.
      * \param hovered True is the symbol is hovered with the pointer. This means that the symbol must be magnified.
      */
    void paint( QPainter *painter, QRectF &bounds, bool hovered );

public slots:
    void setStyle( Style *style );

private:
    enum symbol_role role;
    QString name;
    Settings *settings;
    StyleSymbol *renderer;
};

/*! \class ModifiedSymbol
  * \brief A ModifiedSymbol is a Symbol with a modifier attribute.
  *
  * A ModifiedSymbol is a Symbol that can be obtained from a Keymap with a modifier mask.
  * \see Symbol
  */
class ModifiedSymbol : public Symbol
{
public:
    /*! \fn ModifiedSymbol( QDomElement el, Settings *settings )
      * \brief Constuctor.
      *
      * Instantiates a ModifiedSymbol object.
      * \param el Pointer to the parsed XML element containing the symbol description.
      * \param settings Pointer to the settings object of the Keyboard.
      */
    ModifiedSymbol( QDomElement el, Settings *settings );
    /*! \fn getModifier()
      * \returns the modifier mask to obtain the symbol.
      */
    quint8 getModifier();
    
private:
    quint8 modifier;
};

#endif // SYMBOL_H
