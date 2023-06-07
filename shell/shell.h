/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Benjamin Meyer <benjamin@csh.rit.edu>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003 Luboš Luňák <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2004 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2004 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_SHELL_H_
#define _OKULAR_SHELL_H_

#include <QAction>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <kparts/mainwindow.h>
#include <kparts/readwritepart.h>

#include <QDBusAbstractAdaptor> // for Q_NOREPLY
#include <QStackedWidget>

#include "welcomescreen.h"

class Sidebar;
class KRecentFilesAction;
class KToggleAction;
class QTabWidget;
class KPluginFactory;

#ifndef Q_OS_WIN
namespace KActivities
{
class ResourceInstance;
}
#endif

/**
 * This is the application "Shell".  It has a menubar and a toolbar
 * but relies on the "Part" to do all the real work.
 *
 * @short Application Shell
 * @author Wilco Greven <greven@kde.org>
 * @version 0.1
 */
class Shell : public KParts::MainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.okular")

    friend class MainShellTest;
    friend class AnnotationToolBarTest;

public:
    /**
     * Constructor
     */
    explicit Shell(const QString &serializedOptions = QString());

    /**
     * Default Destructor
     */
    ~Shell() override;

    QSize sizeHint() const override;

    /**
     * Returns false if Okular component wasn't found
     **/
    bool isValid() const;

    bool openDocument(const QUrl &url, const QString &serializedOptions);

public Q_SLOTS:
    Q_SCRIPTABLE Q_NOREPLY void tryRaise(const QString &startupId);
    Q_SCRIPTABLE bool openDocument(const QString &urlString, const QString &serializedOptions = QString());
    Q_SCRIPTABLE bool canOpenDocs(int numDocs, int desktop);

protected:
    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfigGroup &) override;

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(const KConfigGroup &) override;

    /**
     * Expose internal functions for session restore testing
     */
    void savePropertiesInternal(KConfig *config, int num)
    {
        KMainWindow::savePropertiesInternal(config, num);
    }
    void readPropertiesInternal(KConfig *config, int num)
    {
        KMainWindow::readPropertiesInternal(config, num);
    }

    void readSettings();
    void writeSettings();
    void setFullScreen(bool);

    using KParts::MainWindow::setCaption;
    void setCaption(const QString &caption) override;

    bool queryClose() override;

    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;

private Q_SLOTS:
    void fileOpen();

    void slotUpdateFullScreen();
    void slotShowMenubar();

    void openUrl(const QUrl &url, const QString &serializedOptions = QString());
    void showOpenRecentMenu();
    void closeUrl();
    void print();
    void setPrintEnabled(bool enabled);
    void setCloseEnabled(bool enabled);
    void setTabIcon(const QMimeType &mimeType);
    void handleDroppedUrls(const QList<QUrl> &urls);

    // Tab event handlers
    void setActiveTab(int tab);
    void closeTab(int tab);
    void activateNextTab();
    void activatePrevTab();
    void undoCloseTab();
    void moveTabData(int from, int to);

    void slotFitWindowToPage(const QSize pageViewSize, const QSize pageSize);

    void hideWelcomeScreen();
    void showWelcomeScreen();
    void refreshRecentsOnWelcomeScreen();

    void forgetRecentItem(QUrl const &url);

Q_SIGNALS:
    void moveSplitter(int sideWidgetSize);

private:
    void saveRecents();
    void setupAccel();
    void setupActions();
    void openNewTab(const QUrl &url, const QString &serializedOptions);
    void applyOptionsToPart(QObject *part, const QString &serializedOptions);
    void connectPart(const KParts::ReadWritePart *part);
    int findTabIndex(QObject *sender) const;
    int findTabIndex(const QUrl &url) const;

private:
    void reloadAllXML();
    bool eventFilter(QObject *obj, QEvent *event) override;

    KPluginFactory *m_partFactory;
    KRecentFilesAction *m_recent;
    QStringList m_fileformats;
    bool m_fileformatsscanned;
    QAction *m_printAction;
    QAction *m_closeAction;
    KToggleAction *m_fullScreenAction;
    KToggleAction *m_showMenuBarAction;
    bool m_menuBarWasShown, m_toolBarWasShown;
    bool m_unique;
    QTabWidget *m_tabWidget;
    KToggleAction *m_openInTab;
    WelcomeScreen *m_welcomeScreen;
    QStackedWidget *m_centralStackedWidget;
    Sidebar *m_sidebar = nullptr;

    struct TabState {
        explicit TabState(KParts::ReadWritePart *p)
            : part(p)
            , printEnabled(false)
            , closeEnabled(false)
        {
        }
        KParts::ReadWritePart *part;
        bool printEnabled;
        bool closeEnabled;
    };
    QList<TabState> m_tabs;
    QList<QUrl> m_closedTabUrls;
    QAction *m_nextTabAction;
    QAction *m_prevTabAction;
    QAction *m_undoCloseTab;
    QAction *m_showSidebarAction = nullptr;
    QAction *m_lockSidebarAction = nullptr;

#ifndef Q_OS_WIN
    KActivities::ResourceInstance *m_activityResource;
#endif
    bool m_isValid;
};

#endif

// vim:ts=2:sw=2:tw=78:et
