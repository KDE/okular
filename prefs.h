/***********************************************************************
 *
 *  prefs.h
 *
 **********************************************************************/

#ifndef KDVIPREFS_H
#define KDVIPREFS_H

class QButtonGroup;
class QPushButton;
class QRadioButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QString;
class QDate;
class QTabDialog;
class QListBox;
class QBoxLayout;
class QGridLayout;

#include <qtabdlg.h>
#include <kapp.h>

class kdviprefs: public QTabDialog
{
  Q_OBJECT
 
public:
	kdviprefs( QWidget *parent=0, const char *name=0 );
	~kdviprefs() {}

	static bool paperSizes( const char *p, float &w, float &h );
  
signals:
	void preferencesChanged();

private slots:
	void applyChanges();

private:
  // Inserts all pages in the dialog.
	void insertPages();
  
  // Store pointers to dialog pages
	QWidget *pages[4]; 
  
  // First page of tab preferences dialog
	QGridLayout *l10;
	QBoxLayout *l11;
 	QCheckBox *scrollb;
 	QCheckBox *menu;
 	QCheckBox *button;
 	QCheckBox *status;
 	QCheckBox *pagelist;
 	QCheckBox *textb;
	QLineEdit *recent;

  // Second page of tab preferences dialog
	QGridLayout *l20;
	QBoxLayout *l21;
	QLineEdit *dpi;
	QLineEdit *mode;
 	QCheckBox *pk;
	QLineEdit *tetexdir;
	QLineEdit *fontdir;
	QLineEdit *small;
	QLineEdit *large;

  // Third page of tab preferences dialog
	QGridLayout *l30;
	QBoxLayout *l31;

 	QCheckBox *ps;
 	QCheckBox *psaa;
	QLineEdit *gamma;

  // Fourth page of tab preferences dialog
	QBoxLayout *l40;
	QBoxLayout *l41;
	QComboBox *paper;
};

#endif
