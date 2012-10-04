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

#ifndef KEYMAP_H
#define KEYMAP_H

#include <QString>
#include <QVector>
#include "symbol.h"

/*! \class KeymapKey
  * \brief This class associates a key code with zero, one or several symbols.
  *
  * A key code can have several symbols assiciated with different modifiers.
  * The KeymapKey is loaded from a &lt;key&gt; element in the Keymap file.
  * The keys are represented in the Keymap file as follow:
  * \code
  * <key code="code" mod="modifier" locker="yes">
  *     list of symbols
  * </key>
  * \endcode
  * where <i>code</i> is the key code and <i>modifier</i> is an optional attribute that represents the modifier value of the key.
  * The key is considered a locker key if the <i>locker</i> attribute is present.
  * If the modifier value is not present, the key is considered a non modifier key and its modifier value is 0.
  * \see Keymap
  */
class KeymapKey : public QObject
{
    Q_OBJECT

public:
    /*! \fn KeymapKey( QDomElement el, Settings *settings )
      * \brief Constructor
      *
      * Instantiates a KeymapKey element.
      * \param el DOM element representing the key in the Keymap file.
      * \param settings The settings object used to instantiate the symbols.
      */
    KeymapKey( QDomElement el, Settings *settings );
    /*! \fn ~KeymapKey()
      * \brief Destructor
      *
      * Destroys a KeymapKey object
      */
    ~KeymapKey();

    /*! \fn getSymbol( quint8 mod )
      * \brief Returns the Symbol associated with this key and the provided modifier.
      * \param mod Modifier applied to this key.
      * \returns A pointer to the ModifiedSymbol associated with this key code and the provided modifier.
      * \see ModifiedSymbol
      */
    ModifiedSymbol *getSymbol( quint8 mod );
    /*! \fn getModifier()
      * \brief Returns the modifier value of this key.
      * \returns 0 for non modifier keys, or the value of the modifier for modifier keys.
      */
    quint8 getModifier();
    /*! \fn isLocker()
      * \brief Returns true if the key is a locker key.
      * \returns true if the key is a locker key.
      */
    bool isLocker();

private:
    QVector<ModifiedSymbol *> symbols;
    quint8 modifier;
    bool locker;
};

/*! \class Keymap
  * \brief Represents a language dependant map to translate key codes to symbols.
  *
  * The keymap class stores the map used to translate key codes to key symbols.
  * It is loaded from an XML file structured as follow:
  * \code
  * <keymap>
  *     list of keys
  * </keymap>
  * \endcode
  * where <i>list of keys</i> is a list of &lt;key&gt; elements.
  * Each key is recorded in a KeymapKey object.
  *
  * The Keymap maintains a global modifier that is calculated from all the latched and locked modifiers.
  *
  * Each key associates a code and a modifier with a symbol.
  * The Keymap can be queried with getSymbol().
  * \see KeymapKey
  */
class Keymap : public QObject
{
    Q_OBJECT

public:
    /*! \fn Keymap()
      * \brief Constructor.
      *
      * Instantiates a Keymap
      */
    Keymap();
    /*! \fn ~Keymap()
      * \brief Destructor.
      *
      * Destroys a Keymap
      */
    ~Keymap();

    /*! \fn load( QString file, Settings *settings )
      * \brief Loads the Keymap from a XML description file.
      * \param file The file name of the XML keymap description.
      * \param settings The settings object used to instantiate the symbols.
      * \returns true for success, false for failure.
      */
    bool load( QString file, Settings *settings );

    /*! \fn getSymbol( quint8 code )
      * \brief Returns the symbol associated with the key code.
      * \param code Key code to query
      * \returns The Symbol associated with the key code
      */
    Symbol *getSymbol( quint8 code );
    /*! \fn getKeyModifier( quint8 code )
      * \brief Returns the modifier value of the key code.
      *
      * This returns the modifier value for modifier keys.
      * For non modifier keys, it returns 0.
      * \param code The key code to query
      * \returns The modifier value assiciated with the key code
      * \see Keymap::addModifier
      * \see Keymap::removeModifier
      */
    quint8 getKeyModifier( quint8 code );
    /*! \fn isLocker( quint8 code )
      * \brief Returns true if the code is associated with a locker key.
      * \param code The key code to query
      * \returns true if the key is a locker key, false otherwise.
      */
    bool isLocker( quint8 code );

    /*! \fn addModifier( quint8 mod )
      * \brief Adds a modifier to the global modifier state.
      *
      * The global modifier state is used to retreive the symbol associated with the key.
      * Use this function when a modifier key is pressed to make sure all future symbol queries will take this modifier into account.
      * \param mod The modifier code as returned by the getKeyModifier() function.
      * \see Keymap::getKeyModifier
      * \see Keymap::removeModifier
      */
    void addModifier( quint8 mod );
    /*! \fn removeModifier( quint8 mod )
      * \brief Removes a modifier from the global modifier state.
      *
      * Use this function when a modifier key has been released.
      * \param mod The modifier code as returned by the getKeyModifier() function.
      * \see Keymap::addModifier
      * \see Keymap::getKeyModifier
      */
    void removeModifier( quint8 mod );

private:
    KeymapKey *keys[256];
    quint8 modifier;
};

#endif // KEYMAP_H
