/*
    SPDX-FileCopyrightText: 2006, 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _DLGPRESENTATION_H
#define _DLGPRESENTATION_H

#include <QComboBox>
#include <QWidget>

class DlgPresentation : public QWidget
{
    Q_OBJECT

public:
    explicit DlgPresentation(QWidget *parent = nullptr);
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

protected Q_SLOTS:
    /**
     * Populates the combobox list with items for the special screens,
     * and for every connected screen.
     * If @c m_disconnectedScreenNumber is set, adds an item for this disconnected screen.
     */
    void repopulateList();
};

/** “Current” and “Default” */
const int k_specialScreenCount = 2;
/** Default value of m_disconnectedScreenNumber when no disconnected screen is remembered */
const int k_noDisconnectedScreenNumber = -3;

#endif
