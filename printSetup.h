/**********************************************************************

	--- Dlgedit generated file ---

	File: printSetup.h
	Last generated: Fri Oct 3 01:51:37 1997

 *********************************************************************/

#ifndef printSetup_included
#define printSetup_included

#include "printSetupData.h"

class printSetup : public printSetupData
{
    Q_OBJECT

public:

    printSetup
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~printSetup();

protected slots:

    virtual void removePrinter();
    virtual void okPressed();
    virtual void addPrinter();
    void readConfig();

};
#endif // printSetup_included
