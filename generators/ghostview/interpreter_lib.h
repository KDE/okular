/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <gs/gdevdsp.h>
#include <stdio.h>
#include <qimage.h>
#include <qstringlist.h>
#include <qmutex.h>


class GVInterpreter
{
    public:
        GVInterpreter(QStringList * list );
        ~GVInterpreter();
        bool handleExit(int code);
        bool run (FILE * tmp, long begin , unsigned int lenght);
        void setGhostscriptArguments( QStringList* list)
            { interpreterLock.lock(); gsArgs=list; interpreterLock.unlock(); };
        bool locked () { return interpreterLock.locked() ;} ;
        QImage* m_img;
    private:
        char** argsToChar();
        void * ghostScriptInstance;
        QString gsPath;
        QStringList *gsArgs;
        QMutex interpreterLock;
};

