#ifndef _KPDF_SHELL_H_
#define _KPDF_SHELL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <kapplication.h>
#include <kparts/mainwindow.h>
 
namespace KPDF
{

  /**
   * This is the application "Shell".  It has a menubar, toolbar, and
   * statusbar but relies on the "Part" to do all the real work.
   *
   * @short Application Shell
   * @author Wilco Greven <greven@kde.org>
   * @version 0.1
   */
  class Shell : public KParts::MainWindow
  {
    Q_OBJECT

  public:
    /**
     * Default Constructor
     */
    Shell();

    /**
     * Default Destructor
     */
    virtual ~Shell();

    /**
     * Use this method to load whatever file/URL you have
     */
    void load(const KURL& url);

  protected:
    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig*);

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(KConfig*);

  private slots:
    void fileOpen();
    void fileSaveAs();

    void optionsConfigureKeys();
    void optionsConfigureToolbars();

    void applyNewToolbarConfig();

  private:
    void setupAccel();
    void setupActions();

  private:
    KParts::ReadOnlyPart* m_part;

  };

}

#endif

// vim:ts=2:sw=2:tw=78:et
