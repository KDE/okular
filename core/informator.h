/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

/**
 * @short The source of information about the Generator
 * 
 * The goal of this class is to provide per Generator settings,
 * about data and other stuff not related with generating okular 
 * content, but still needed for a full-featured plugin system.
 *
 */

class Informator
{
    virtual QString iconName() = 0;
    virtual void addConfigurationPages ( KConfigDialog* cfg ) = 0;
    virtual KAboutData about();
    virtual QStringList mimetypes() = 0;
}
