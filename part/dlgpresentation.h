/***************************************************************************
 *   Copyright (C) 2006,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DLGPRESENTATION_H
#define _DLGPRESENTATION_H

#include <QComboBox>
#include <QWidget>

class Ui_DlgPresentationBase;

class DlgPresentation : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPresentation(QWidget *parent = nullptr);
    ~DlgPresentation() override;

protected:
    Ui_DlgPresentationBase *m_dlg;
};

/**
 * We need this because there are some special screens,
 * which are not represented by the typical currentIndex(),
 * which would be used by KConfigWidgets.
 *
 * Additionally this class allows to remember a disconnected screen.
 */
class PreferredScreenSelector : public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(int preferredScreen READ preferredScreen WRITE setPreferredScreen NOTIFY preferredScreenChanged)

public:
    explicit PreferredScreenSelector(QWidget *parent);
    int preferredScreen() const;

Q_SIGNALS:
    void preferredScreenChanged(int screen);

public Q_SLOTS:
    void setPreferredScreen(int newScreen);

protected:
    // These two variables protect the screen setting from changing
    // when the configured screen is currently disconnected.

    /**
     * When setScreen() is called with a disconnected screen,
     * a “disconnected” entry is created at this index:
     */
    int m_disconnectedScreenIndex;

    /**
     * Which screen is referred by @c m_disconnectedScreenIndex.
     * Until @c m_disconnectedScreenIndex entry is created, this is @c k_noDisconnectedScreenNumber.
     */
    int m_disconnectedScreenNumber;
};

/** “Current” and “Default” */
const int k_specialScreenCount = 2;
/** Default value of m_disconnectedScreenNumber when no disconnected screen is remembered */
const int k_noDisconnectedScreenNumber = -3;

#endif
