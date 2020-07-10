/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TEXTDOCUMENTSETTINGS_P_H_
#define _OKULAR_TEXTDOCUMENTSETTINGS_P_H_

class KFontRequester;
class Ui_TextDocumentSettings;

namespace Okular
{
class TextDocumentSettingsWidgetPrivate
{
public:
    /**
     * @note the private class won't take ownership of the ui, so you
     *       must delete it yourself
     */
    explicit TextDocumentSettingsWidgetPrivate(Ui_TextDocumentSettings *ui)
        : mUi(ui)
    {
    }

    KFontRequester *mFont;
    Ui_TextDocumentSettings *mUi;
};

class TextDocumentSettingsPrivate : public QObject
{
    Q_OBJECT

public:
    explicit TextDocumentSettingsPrivate(QObject *parent)
        : QObject(parent)
    {
    }

    QFont mFont;
};

}

#endif
