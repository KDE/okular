/**********************************************************************

	--- Dlgedit generated file ---

	File: print.h
	Last generated: Wed Oct 1 21:53:48 1997

 *********************************************************************/

#ifndef print_included
#define print_included

#include "printData.h"

class DVIFile;

class print : public printData
{
    Q_OBJECT

public:

    print
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~print();

    void setFile( QString file );
    void setCurrentPage( int page, int totalpages );
    void setMarkList( QStrList *marklist );

protected slots:

    void printDestinationChanged(int);
    void rangeToggled(bool);
    void setupPressed();
    void okPressed();
    void cancelPressed();
    void nupPressed(int);
    void readConfig();

private:
    QString ifile,ofile;
    int	curpage, totalpages, nup, printdest;
    QStrList *marklist;
    QString nupProgram, printMethod, spooler;

};
#endif // print_included
