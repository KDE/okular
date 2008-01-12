/***************************************************************************
 *   Copyright (C) 2007 by John Layt <john@layt.net>                       *
 *                                                                         *
 *   FilePrinterPreview based on KPrintPreview (originally LGPL)           *
 *   Copyright (c) 2007 Alex Merry <huntedhacker@tiscali.co.uk>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "fileprinter.h"

#include <QtGui/QPrinter>
#include <QPrintEngine>
#include <QStringList>
#include <QFile>
#include <QSize>
#include <QtCore/QFile>
#include <QtGui/QLabel>
#include <QtGui/QShowEvent>

#include <KProcess>
#include <KShell>
#include <klocalsocket.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#include <kdebug.h>


using namespace Okular;

int FilePrinter::printFile( QPrinter &printer, const QString &file, FileDeletePolicy fileDeletePolicy,
                            PageSelectPolicy pageSelectPolicy, const QString &pageRange )
{
    FilePrinter fp;
    return fp.doPrintFiles( printer, QStringList( file ), fileDeletePolicy, pageSelectPolicy, pageRange );
}

int FilePrinter::printFiles( QPrinter &printer, const QStringList &fileList, FileDeletePolicy fileDeletePolicy )
{
    FilePrinter fp;
    return fp.doPrintFiles( printer, fileList, fileDeletePolicy, FilePrinter::ApplicationSelectsPages, QString() );
}

int FilePrinter::doPrintFiles( QPrinter &printer, const QStringList &fileList, FileDeletePolicy fileDeletePolicy,
                               PageSelectPolicy pageSelectPolicy, const QString &pageRange )
{
    QString exe("lpr");

    if ( KStandardDirs::findExe(exe).isEmpty() ) {
        exe = "lp";
    }

    if ( KStandardDirs::findExe(exe).isEmpty() ) {
        return -9;
    }

    if ( fileList.size() < 1 ) {
        return -8;
    }

    for (QStringList::ConstIterator it = fileList.begin(); it != fileList.end(); ++it) {
        if (!QFile::exists(*it)) {
            return -7;
        }
    }

    if ( printer.printerState() == QPrinter::Aborted || printer.printerState() == QPrinter::Error ) {
        return -6;
    }

    // Print to File if a filename set, assumes there must be only 1 file
    if ( !printer.outputFileName().isEmpty() ) {

        if ( QFile::exists( printer.outputFileName() ) ) {
            QFile::remove( printer.outputFileName() );
        }

        int res = QFile::copy( fileList[0], printer.outputFileName() );

        if ( fileDeletePolicy == FilePrinter::SystemDeletesFiles ) {
            QFile::remove( fileList[0] );
        }

        if ( res ) return 0;

    }

    bool useCupsOptions = cupsAvailable();
    QStringList argList = printArguments( printer, fileDeletePolicy, pageSelectPolicy, useCupsOptions, pageRange, exe )
                          << fileList;
    kDebug() << "Executing " << exe << " with arguments" << argList;

    int ret = KProcess::execute( exe, argList );

    // If we used the Cups options and the command failed, try again without them in case Cups lpr/lp not installed.
    // I'm not convinced this will work, I don't think KProcess returns the result of the command, but rather
    // that it was able to be executed?
    if ( useCupsOptions && ret < 0 ) {

        argList = printArguments( printer, fileDeletePolicy, pageSelectPolicy, useCupsOptions, pageRange, exe )
                  << fileList;
        kDebug() << "Retrying " << exe << " without cups arguments" << argList;

        ret = KProcess::execute( exe, argList );
    }

    return ret;
}

QList<int> FilePrinter::pageList( QPrinter &printer, int lastPage, const QList<int> &selectedPageList )
{
    if ( printer.printRange() == QPrinter::Selection) {
        return selectedPageList;
    }

    int startPage, endPage;
    QList<int> list;

    if ( printer.printRange() == QPrinter::PageRange ) {
        startPage = printer.fromPage();
        endPage = printer.toPage();
    } else { //AllPages
        startPage = 1;
        endPage = lastPage;
    }

    for (int i = startPage; i <= endPage; i++ ) {
        list << i;
    }
    return list;
}

QString FilePrinter::pageRange( QPrinter &printer, int lastPage, const QList<int> &selectedPageList )
{
    if ( printer.printRange() == QPrinter::Selection) {
        return pageListToPageRange( selectedPageList );
    }

    if ( printer.printRange() == QPrinter::PageRange ) {
        return QString("%1-%2").arg(printer.fromPage()).arg(printer.toPage());
    }

    return QString("1-%2").arg( lastPage );
}

QString FilePrinter::pageListToPageRange( const QList<int> &pageList )
{
    QString pageRange;
    int count = pageList.count();
    int i = 0;
    int seqStart = i;
    int seqEnd;

    while ( i != count ) {

        if ( i + 1 == count || pageList[i] + 1 != pageList[i+1] ) {

            seqEnd = i;

            if ( !pageRange.isEmpty() ) {
                pageRange.append(",");
            }

            if ( seqStart == seqEnd ) {
                pageRange.append(pageList[i]);
            } else {
                pageRange.append("%1-%2").arg(seqStart).arg(seqEnd);
            }

            seqStart = i + 1;
        }

        i++;
    }

    return pageRange;
}

bool FilePrinter::cupsAvailable()
{
    FilePrinter fp;
    return ( fp.detectCupsConfig() /*&& fp.detectCupsService()*/ );
}

bool FilePrinter::detectCupsService()
{
    // copied from KdeprintChecker::checkService()
    // Copyright (c) 2001 Michael Goffioul <kdeprint@swing.be>
    // original license LGPL
    KLocalSocket sock;
    sock.connectToPath("/ipp");
    kDebug() << "socket wait = " << sock.waitForConnected();
    kDebug() << "socket error = " << sock.error();
    kDebug() << "socket isOpen() = " << sock.isOpen();
    return sock.isOpen();
}

bool FilePrinter::detectCupsConfig()
{
    if ( QFile::exists("/etc/cups/cupsd.conf") ) return true;
    if ( QFile::exists("/usr/etc/cups/cupsd.conf") ) return true;
    if ( QFile::exists("/usr/local/etc/cups/cupsd.conf") ) return true;
    if ( QFile::exists("/opt/etc/cups/cupsd.conf") ) return true;
    if ( QFile::exists("/opt/local/etc/cups/cupsd.conf") ) return true;
    return false;
}

QSize FilePrinter::psPaperSize( QPrinter &printer )
{
    QSize size;

    switch ( printer.pageSize() ) {
    case QPrinter::A0:        size = QSize( 2384, 3370 ); break;
    case QPrinter::A1:        size = QSize( 1684, 2384 ); break;
    case QPrinter::A2:        size = QSize( 1191, 1684 ); break;
    case QPrinter::A3:        size = QSize(  842, 1191 ); break;
    case QPrinter::A4:        size = QSize(  595,  842 ); break;
    case QPrinter::A5:        size = QSize(  420,  595 ); break;
    case QPrinter::A6:        size = QSize(  298,  420 ); break;
    case QPrinter::A7:        size = QSize(  210,  298 ); break;
    case QPrinter::A8:        size = QSize(  147,  210 ); break;
    case QPrinter::A9:        size = QSize(  105,  147 ); break;
    case QPrinter::B0:        size = QSize( 2835, 4008 ); break;
    case QPrinter::B1:        size = QSize( 2004, 2835 ); break;
    case QPrinter::B2:        size = QSize( 1417, 2004 ); break;
    case QPrinter::B3:        size = QSize( 1001, 1417 ); break;
    case QPrinter::B4:        size = QSize(  709, 1001 ); break;
    case QPrinter::B5:        size = QSize(  499,  709 ); break;
    case QPrinter::B6:        size = QSize(  354,  499 ); break;
    case QPrinter::B7:        size = QSize(  249,  354 ); break;
    case QPrinter::B8:        size = QSize(  176,  249 ); break;
    case QPrinter::B9:        size = QSize(  125,  176 ); break;
    case QPrinter::B10:       size = QSize(   88,  125 ); break;
    case QPrinter::C5E:       size = QSize(  459,  649 ); break;
    case QPrinter::Comm10E:   size = QSize(  297,  684 ); break;
    case QPrinter::DLE:       size = QSize(  312,  624 ); break;
    case QPrinter::Executive: size = QSize(  522,  756 ); break;
    case QPrinter::Folio:     size = QSize(  595,  935 ); break;
    case QPrinter::Ledger:    size = QSize( 1224,  792 ); break;
    case QPrinter::Legal:     size = QSize(  612, 1008 ); break;
    case QPrinter::Letter:    size = QSize(  612,  792 ); break;
    case QPrinter::Tabloid:   size = QSize(  792, 1224 ); break;
    case QPrinter::Custom:    return QSize( (int) printer.widthMM() * ( 25.4 / 72 ),
                                            (int) printer.heightMM() * ( 25.4 / 72 ) );
    default:                  return QSize();
    }

    if ( printer.orientation() == QPrinter::Landscape ) {
        size.transpose();
    }

    return size;
}




QStringList FilePrinter::printArguments( QPrinter &printer, FileDeletePolicy fileDeletePolicy,
                                         PageSelectPolicy pageSelectPolicy, bool useCupsOptions,
                                         const QString &pageRange, const QString &version )
{
    QStringList argList;

    if ( ! destination( printer, version ).isEmpty() ) {
        argList << destination( printer, version );
    }

    if ( ! copies( printer, version ).isEmpty() ) {
        argList << copies( printer, version );
    }

    if ( ! jobname( printer, version ).isEmpty() ) {
        argList << jobname( printer, version );
    }

    if ( ! pages( printer, pageSelectPolicy, pageRange, useCupsOptions, version ).isEmpty() ) {
        argList << pages( printer, pageSelectPolicy, pageRange, useCupsOptions, version );
    }

    if ( useCupsOptions && ! cupsOptions( printer ).isEmpty() ) {
        argList << cupsOptions( printer );
    }

    if ( ! deleteFile( printer, fileDeletePolicy, version ).isEmpty() ) {
        argList << deleteFile( printer, fileDeletePolicy, version );
    }

    if ( version == "lp" ) {
        argList << "--";
    }

    return argList;
}

QStringList FilePrinter::destination( QPrinter &printer, const QString &version )
{
    if ( version == "lp" ) {
        return QStringList("-d") << printer.printerName();
    }

    if ( version == "lpr" ) {
        return QStringList("-P") << printer.printerName();
    }

    return QStringList();
}

QStringList FilePrinter::copies( QPrinter &printer, const QString &version )
{
    // Can't use QPrinter::numCopies(), as if CUPS will always return 1, not sure if this behaves same way as well?
    int cp = printer.printEngine()->property( QPrintEngine::PPK_NumberOfCopies ).toInt();

    if ( version == "lp" ) {
        return QStringList("-n") << QString("%1").arg( cp );
    }

    if ( version == "lpr" ) {
        return QStringList() << QString("-#%1").arg( cp );
    }

    return QStringList();
}

QStringList FilePrinter::jobname( QPrinter &printer, const QString &version )
{
    if ( ! printer.docName().isEmpty() ) {

        if ( version == "lp" ) {
            return QStringList("-t") << printer.docName();
        }

        if ( version == "lpr" ) {
            return QStringList("-J") << printer.docName();
        }
    }

    return QStringList();
}

QStringList FilePrinter::deleteFile( QPrinter &printer, FileDeletePolicy fileDeletePolicy, const QString &version )
{
    if ( fileDeletePolicy == FilePrinter::SystemDeletesFiles && version == "lpr" ) {
        return QStringList("-r");
    }

    return QStringList();
}

QStringList FilePrinter::pages( QPrinter &printer, PageSelectPolicy pageSelectPolicy, const QString &pageRange,
                                    bool useCupsOptions, const QString &version )
{
    if ( pageSelectPolicy == FilePrinter::SystemSelectsPages ) {

        if ( printer.printRange() == QPrinter::Selection && ! pageRange.isEmpty() ) {

            if ( version == "lp" ) {
                return QStringList("-P") << pageRange ;
            }

            if ( version == "lpr" && useCupsOptions ) {
                return QStringList("-o") << QString("page-ranges=%1").arg( pageRange );
            }

        }

        if ( printer.printRange() == QPrinter::PageRange ) {

            if ( version == "lp" ) {
                return QStringList("-P") << QString("%1-%2").arg( printer.fromPage() )
                                                            .arg( printer.toPage() );
            }

            if ( version == "lpr" && useCupsOptions ) {
                return QStringList("-o") << QString("page-ranges=%1-%2").arg( printer.fromPage() )
                                                                        .arg( printer.toPage() );
            }

        }

    }

    return QStringList(); // AllPages
}

QStringList FilePrinter::cupsOptions( QPrinter &printer )
{
    QStringList optionList;

    if ( ! optionMedia( printer ).isEmpty() ) {
        optionList << optionMedia( printer );
    }

    if ( ! optionOrientation( printer ).isEmpty() ) {
        optionList << optionOrientation( printer );
    }

    if ( ! optionDoubleSidedPrinting( printer ).isEmpty() ) {
        optionList << optionDoubleSidedPrinting( printer );
    }

    if ( ! optionPageOrder( printer ).isEmpty() ) {
        optionList << optionPageOrder( printer );
    }

    if ( ! optionCollateCopies( printer ).isEmpty() ) {
        optionList << optionCollateCopies( printer );
    }

    return optionList;
}

QStringList FilePrinter::optionMedia( QPrinter &printer )
{
    if ( ! mediaPageSize( printer ).isEmpty() && 
         ! mediaPaperSource( printer ).isEmpty() ) {
        return QStringList("-o") <<
                QString("media=%1,%2").arg( mediaPageSize( printer ) )
                                      .arg( mediaPaperSource( printer ) );
    }

    if ( ! mediaPageSize( printer ).isEmpty() ) {
        return QStringList("-o") <<
                QString("media=%1").arg( mediaPageSize( printer ) );
    }

    if ( ! mediaPaperSource( printer ).isEmpty() ) {
        return QStringList("-o") <<
                QString("media=%1").arg( mediaPaperSource( printer ) );
    }

    return QStringList();
}

QString FilePrinter::mediaPageSize( QPrinter &printer )
{
    switch ( printer.pageSize() ) {
    case QPrinter::A0:         return "A0";
    case QPrinter::A1:         return "A1";
    case QPrinter::A2:         return "A2";
    case QPrinter::A3:         return "A3";
    case QPrinter::A4:         return "A4";
    case QPrinter::A5:         return "A5";
    case QPrinter::A6:         return "A6";
    case QPrinter::A7:         return "A7";
    case QPrinter::A8:         return "A8";
    case QPrinter::A9:         return "A9";
    case QPrinter::B0:         return "B0";
    case QPrinter::B1:         return "B1";
    case QPrinter::B10:        return "B10";
    case QPrinter::B2:         return "B2";
    case QPrinter::B3:         return "B3";
    case QPrinter::B4:         return "B4";
    case QPrinter::B5:         return "B5";
    case QPrinter::B6:         return "B6";
    case QPrinter::B7:         return "B7";
    case QPrinter::B8:         return "B8";
    case QPrinter::B9:         return "B9";
    case QPrinter::C5E:        return "C5";     //Correct Translation?
    case QPrinter::Comm10E:    return "Comm10"; //Correct Translation?
    case QPrinter::DLE:        return "DL";     //Correct Translation?
    case QPrinter::Executive:  return "Executive";
    case QPrinter::Folio:      return "Folio";
    case QPrinter::Ledger:     return "Ledger";
    case QPrinter::Legal:      return "Legal";
    case QPrinter::Letter:     return "Letter";
    case QPrinter::Tabloid:    return "Tabloid";
    case QPrinter::Custom:     return QString("Custom.%1x%2mm")
                                            .arg( printer.heightMM() )
                                            .arg( printer.widthMM() );
    default:                   return QString();
    }
}

// What about Upper and MultiPurpose?  And others in PPD???
QString FilePrinter::mediaPaperSource( QPrinter &printer )
{
    switch ( printer.paperSource() ) {
    case QPrinter::Auto:            return QString();
    case QPrinter::Cassette:        return "Cassette";
    case QPrinter::Envelope:        return "Envelope";
    case QPrinter::EnvelopeManual:  return "EnvelopeManual";
    case QPrinter::FormSource:      return "FormSource";
    case QPrinter::LargeCapacity:   return "LargeCapacity";
    case QPrinter::LargeFormat:     return "LargeFormat";
    case QPrinter::Lower:           return "Lower";
    case QPrinter::MaxPageSource:   return "MaxPageSource";
    case QPrinter::Middle:          return "Middle";
    case QPrinter::Manual:          return "Manual";
    case QPrinter::OnlyOne:         return "OnlyOne";
    case QPrinter::Tractor:         return "Tractor";
    case QPrinter::SmallFormat:     return "SmallFormat";
    default:                        return QString();
    }
}

QStringList FilePrinter::optionOrientation( QPrinter &printer )
{
    switch ( printer.orientation() ) {
    case QPrinter::Portrait:   return QStringList("-o") << "portrait";
    case QPrinter::Landscape:  return QStringList("-o") << "landscape";
    default:                   return QStringList();
    }
}

QStringList FilePrinter::optionDoubleSidedPrinting( QPrinter &printer )
{
    if ( printer.doubleSidedPrinting() ) {
        if ( printer.orientation() == QPrinter::Landscape ) {
            return QStringList("-o") << "sides=two-sided-short-edge";
        } else {
            return QStringList("-o") << "sides=two-sided-long-edge";
        }
    }
    return QStringList("-o") << "sides=one-sided";
}

QStringList FilePrinter::optionPageOrder( QPrinter &printer )
{
    if ( printer.pageOrder() == QPrinter::LastPageFirst ) {
        return QStringList("-o") << "outputorder=reverse";
    }
    return QStringList("-o") << "outputorder=normal";
}

QStringList FilePrinter::optionCollateCopies( QPrinter &printer )
{
    if ( printer.collateCopies() ) {
        return QStringList("-o") << "Collate=True";
    }
    return QStringList("-o") << "Collate=False";
}

#include "fileprinter.moc"
