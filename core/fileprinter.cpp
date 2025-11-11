/*
    SPDX-FileCopyrightText: 2007, 2010 John Layt <john@layt.net>

    FilePrinterPreview based on KPrintPreview (originally LGPL)
    SPDX-FileCopyrightText: 2007 Alex Merry <huntedhacker@tiscali.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fileprinter.h"

#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPrintEngine>
#include <QShowEvent>
#include <QSize>
#include <QStringList>
#include <QTcpSocket>

#include <KProcess>
#include <KShell>
#include <QDebug>
#include <QStandardPaths>

#include "debug_p.h"

using namespace Okular;

Document::PrintError
FilePrinter::printFile(QPrinter &printer, const QString &file, QPageLayout::Orientation documentOrientation, FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy, const QString &pageRange, ScaleMode scaleMode)
{
    FilePrinter fp;
    return fp.doPrintFiles(printer, QStringList(file), fileDeletePolicy, pageSelectPolicy, pageRange, documentOrientation, scaleMode);
}

static Document::PrintError doKProcessExecute(const QString &exe, const QStringList &argList)
{
    const int ret = KProcess::execute(exe, argList);
    if (ret == -1) {
        return Document::PrintingProcessCrashPrintError;
    }
    if (ret == -2) {
        return Document::PrintingProcessStartPrintError;
    }
    if (ret < 0) {
        return Document::UnknownPrintError;
    }

    return Document::NoPrintError;
}

Document::PrintError
FilePrinter::doPrintFiles(QPrinter &printer, const QStringList &fileList, FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy, const QString &pageRange, QPageLayout::Orientation documentOrientation, ScaleMode scaleMode)
{
    if (fileList.size() < 1) {
        return Document::NoFileToPrintError;
    }

    for (QStringList::ConstIterator it = fileList.constBegin(); it != fileList.constEnd(); ++it) {
        if (!QFile::exists(*it)) {
            return Document::UnableToFindFilePrintError;
        }
    }

    if (printer.printerState() == QPrinter::Aborted || printer.printerState() == QPrinter::Error) {
        return Document::InvalidPrinterStatePrintError;
    }

    QString exe;
    QStringList argList;
    Document::PrintError ret;

    // Print to File if a filename set, assumes there must be only 1 file
    if (!printer.outputFileName().isEmpty()) {
        if (QFile::exists(printer.outputFileName())) {
            QFile::remove(printer.outputFileName());
        }

        QFileInfo inputFileInfo = QFileInfo(fileList[0]);
        QFileInfo outputFileInfo = QFileInfo(printer.outputFileName());

        bool doDeleteFile = (fileDeletePolicy == FilePrinter::SystemDeletesFiles);
        if (inputFileInfo.suffix() == outputFileInfo.suffix()) {
            if (doDeleteFile) {
                bool res = QFile::rename(fileList[0], printer.outputFileName());
                if (res) {
                    doDeleteFile = false;
                    ret = Document::NoPrintError;
                } else {
                    ret = Document::PrintToFilePrintError;
                }
            } else {
                bool res = QFile::copy(fileList[0], printer.outputFileName());
                if (res) {
                    ret = Document::NoPrintError;
                } else {
                    ret = Document::PrintToFilePrintError;
                }
            }
        } else if (inputFileInfo.suffix() == QLatin1String("ps") && printer.outputFormat() == QPrinter::PdfFormat && ps2pdfAvailable()) {
            exe = QStringLiteral("ps2pdf");
            argList << fileList[0] << printer.outputFileName();
            qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;
            ret = doKProcessExecute(exe, argList);
        } else if (inputFileInfo.suffix() == QLatin1String("pdf") && printer.outputFormat() == QPrinter::NativeFormat && pdf2psAvailable()) {
            exe = QStringLiteral("pdf2ps");
            argList << fileList[0] << printer.outputFileName();
            qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;
            ret = doKProcessExecute(exe, argList);
        } else {
            ret = Document::PrintToFilePrintError;
        }

        if (doDeleteFile) {
            QFile::remove(fileList[0]);
        }

    } else { // Print to a printer via lpr command

        // Decide what executable to use to print with, need the CUPS version of lpr if available
        // Some distros name the CUPS version of lpr as lpr-cups or lpr.cups so try those first
        // before default to lpr, or failing that to lp

        if (!QStandardPaths::findExecutable(QStringLiteral("lpr-cups")).isEmpty()) {
            exe = QStringLiteral("lpr-cups");
        } else if (!QStandardPaths::findExecutable(QStringLiteral("lpr.cups")).isEmpty()) {
            exe = QStringLiteral("lpr.cups");
        } else if (!QStandardPaths::findExecutable(QStringLiteral("lpr")).isEmpty()) {
            exe = QStringLiteral("lpr");
        } else if (!QStandardPaths::findExecutable(QStringLiteral("lp")).isEmpty()) {
            exe = QStringLiteral("lp");
        } else {
            return Document::NoBinaryToPrintError;
        }

        bool useCupsOptions = cupsAvailable();
        argList = printArguments(printer, fileDeletePolicy, pageSelectPolicy, useCupsOptions, pageRange, exe, documentOrientation, scaleMode) << fileList;
        qCDebug(OkularCoreDebug) << "Executing" << exe << "with arguments" << argList;

        ret = doKProcessExecute(exe, argList);
    }

    return ret;
}

QList<int> FilePrinter::pageList(QPrinter &printer, int lastPage, int currentPage, const QList<int> &selectedPageList)
{
    if (printer.printRange() == QPrinter::Selection) {
        return selectedPageList;
    }

    int startPage, endPage;
    QList<int> list;

    if (printer.printRange() == QPrinter::PageRange) {
        startPage = printer.fromPage();
        endPage = printer.toPage();
    } else if (printer.printRange() == QPrinter::CurrentPage) {
        startPage = currentPage;
        endPage = currentPage;
    } else { // AllPages
        startPage = 1;
        endPage = lastPage;
    }

    for (int i = startPage; i <= endPage; i++) {
        list << i;
    }

    return list;
}

bool FilePrinter::ps2pdfAvailable()
{
    return (!QStandardPaths::findExecutable(QStringLiteral("ps2pdf")).isEmpty());
}

bool FilePrinter::pdf2psAvailable()
{
    return (!QStandardPaths::findExecutable(QStringLiteral("pdf2ps")).isEmpty());
}

bool FilePrinter::cupsAvailable()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_OSX)
    // Ideally we would have access to the private Qt method
    // QCUPSSupport::cupsAvailable() to do this as it is very complex routine.
    // However, if CUPS is available then QPrinter::supportsMultipleCopies() will always return true
    // whereas if CUPS is not available it will return false.
    // This behaviour is guaranteed never to change, so we can use it as a reliable substitute.
    QPrinter testPrinter;
    return testPrinter.supportsMultipleCopies();
#else
    return false;
#endif
}

QStringList FilePrinter::printArguments(QPrinter &printer,
                                        FileDeletePolicy fileDeletePolicy,
                                        PageSelectPolicy pageSelectPolicy,
                                        bool useCupsOptions,
                                        const QString &pageRange,
                                        const QString &version,
                                        QPageLayout::Orientation documentOrientation,
                                        ScaleMode scaleMode)
{
    QStringList argList;

    if (!destination(printer, version).isEmpty()) {
        argList << destination(printer, version);
    }

    if (!copies(printer, version).isEmpty()) {
        argList << copies(printer, version);
    }

    if (!jobname(printer, version).isEmpty()) {
        argList << jobname(printer, version);
    }

    if (!pages(printer, pageSelectPolicy, pageRange, useCupsOptions, version).isEmpty()) {
        argList << pages(printer, pageSelectPolicy, pageRange, useCupsOptions, version);
    }

    if (useCupsOptions && !cupsOptions(printer, documentOrientation, scaleMode).isEmpty()) {
        argList << cupsOptions(printer, documentOrientation, scaleMode);
    }

    if (!deleteFile(printer, fileDeletePolicy, version).isEmpty()) {
        argList << deleteFile(printer, fileDeletePolicy, version);
    }

    if (version == QLatin1String("lp")) {
        argList << QStringLiteral("--");
    }

    return argList;
}

QStringList FilePrinter::destination(QPrinter &printer, const QString &version)
{
    if (version == QLatin1String("lp")) {
        return QStringList(QStringLiteral("-d")) << printer.printerName();
    }

    if (version.startsWith(QLatin1String("lpr"))) {
        return QStringList(QStringLiteral("-P")) << printer.printerName();
    }

    return QStringList();
}

QStringList FilePrinter::copies(QPrinter &printer, const QString &version)
{
    int cp = printer.copyCount();

    if (version == QLatin1String("lp")) {
        return QStringList(QStringLiteral("-n")) << QStringLiteral("%1").arg(cp);
    }

    if (version.startsWith(QLatin1String("lpr"))) {
        return QStringList() << QStringLiteral("-#%1").arg(cp);
    }

    return QStringList();
}

QStringList FilePrinter::jobname(QPrinter &printer, const QString &version)
{
    if (!printer.docName().isEmpty()) {
        if (version == QLatin1String("lp")) {
            return QStringList(QStringLiteral("-t")) << printer.docName();
        }

        if (version.startsWith(QLatin1String("lpr"))) {
            const QString shortenedDocName = QString::fromUtf8(printer.docName().toUtf8().left(255));
            return QStringList(QStringLiteral("-J")) << shortenedDocName;
        }
    }

    return QStringList();
}

QStringList FilePrinter::deleteFile(QPrinter &, FileDeletePolicy fileDeletePolicy, const QString &version)
{
    if (fileDeletePolicy == FilePrinter::SystemDeletesFiles && version.startsWith(QLatin1String("lpr"))) {
        return QStringList(QStringLiteral("-r"));
    }

    return QStringList();
}

QStringList FilePrinter::pages(QPrinter &printer, PageSelectPolicy pageSelectPolicy, const QString &pageRange, bool useCupsOptions, const QString &version)
{
    if (pageSelectPolicy == FilePrinter::SystemSelectsPages) {
        if (printer.printRange() == QPrinter::Selection && !pageRange.isEmpty()) {
            if (version == QLatin1String("lp")) {
                return QStringList(QStringLiteral("-P")) << pageRange;
            }

            if (version.startsWith(QLatin1String("lpr")) && useCupsOptions) {
                return QStringList(QStringLiteral("-o")) << QStringLiteral("page-ranges=%1").arg(pageRange);
            }
        }

        if (printer.printRange() == QPrinter::PageRange) {
            if (version == QLatin1String("lp")) {
                return QStringList(QStringLiteral("-P")) << QStringLiteral("%1-%2").arg(printer.fromPage()).arg(printer.toPage());
            }

            if (version.startsWith(QLatin1String("lpr")) && useCupsOptions) {
                return QStringList(QStringLiteral("-o")) << QStringLiteral("page-ranges=%1-%2").arg(printer.fromPage()).arg(printer.toPage());
            }
        }
    }

    return QStringList(); // AllPages
}

QStringList FilePrinter::cupsOptions(QPrinter &printer, QPageLayout::Orientation documentOrientation, ScaleMode scaleMode)
{
    QStringList optionList;

    if (!optionMedia(printer).isEmpty()) {
        optionList << optionMedia(printer);
    }

    if (!optionOrientation(printer, documentOrientation).isEmpty()) {
        optionList << optionOrientation(printer, documentOrientation);
    }

    if (!optionDoubleSidedPrinting(printer).isEmpty()) {
        optionList << optionDoubleSidedPrinting(printer);
    }

    if (!optionPageOrder(printer).isEmpty()) {
        optionList << optionPageOrder(printer);
    }

    if (!optionCollateCopies(printer).isEmpty()) {
        optionList << optionCollateCopies(printer);
    }

    if (!optionPageMargins(printer, scaleMode).isEmpty()) {
        optionList << optionPageMargins(printer, scaleMode);
    }

    optionList << optionCupsProperties(printer);

    return optionList;
}

QStringList FilePrinter::optionMedia(QPrinter &printer)
{
    if (!mediaPageSize(printer).isEmpty() && !mediaPaperSource(printer).isEmpty()) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("media=%1,%2").arg(mediaPageSize(printer), mediaPaperSource(printer));
    }

    if (!mediaPageSize(printer).isEmpty()) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("media=%1").arg(mediaPageSize(printer));
    }

    if (!mediaPaperSource(printer).isEmpty()) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("media=%1").arg(mediaPaperSource(printer));
    }

    return QStringList();
}

QString FilePrinter::mediaPageSize(QPrinter &printer)
{
    switch (printer.pageLayout().pageSize().id()) {
    case QPageSize::AnsiC:
        return QStringLiteral("AnsiC");
    case QPageSize::AnsiD:
        return QStringLiteral("AnsiD");
    case QPageSize::AnsiE:
        return QStringLiteral("AnsiE");
    case QPageSize::A0:
        return QStringLiteral("A0");
    case QPageSize::A1:
        return QStringLiteral("A1");
    case QPageSize::A10:
        return QStringLiteral("A10");
    case QPageSize::A2:
        return QStringLiteral("A2");
    case QPageSize::A3:
        return QStringLiteral("A3");
    case QPageSize::A3Extra:
        return QStringLiteral("A3Extra");
    case QPageSize::A4:
        return QStringLiteral("A4");
    case QPageSize::A4Extra:
        return QStringLiteral("A4Extra");
    case QPageSize::A5:
        return QStringLiteral("A5");
    case QPageSize::A5Extra:
        return QStringLiteral("A5Extra");
    case QPageSize::A6:
        return QStringLiteral("A6");
    case QPageSize::A7:
        return QStringLiteral("A7");
    case QPageSize::A8:
        return QStringLiteral("A8");
    case QPageSize::A9:
        return QStringLiteral("A9");
    case QPageSize::ArchA:
        return QStringLiteral("ARCHA");
    case QPageSize::ArchB:
        return QStringLiteral("ARCHB");
    case QPageSize::ArchC:
        return QStringLiteral("ARCHC");
    case QPageSize::ArchD:
        return QStringLiteral("ARCHD");
    case QPageSize::ArchE:
        return QStringLiteral("ARCHE");
    case QPageSize::B0:
        return QStringLiteral("B0");
    case QPageSize::B1:
        return QStringLiteral("ISOB1");
    case QPageSize::B10:
        return QStringLiteral("ISOB10");
    case QPageSize::B2:
        return QStringLiteral("ISOB2");
    case QPageSize::B3:
        return QStringLiteral("ISOB3");
    case QPageSize::B4:
        return QStringLiteral("ISOB4");
    case QPageSize::B5:
        return QStringLiteral("ISOB5");
    case QPageSize::B5Extra:
        return QStringLiteral("ISOB5Extra");
    case QPageSize::B6:
        return QStringLiteral("ISOB6");
    case QPageSize::B7:
        return QStringLiteral("ISOB7");
    case QPageSize::B8:
        return QStringLiteral("ISOB8");
    case QPageSize::B9:
        return QStringLiteral("ISOB9");
    case QPageSize::C5E:
        return QStringLiteral("EnvC5");
    case QPageSize::Comm10E:
        return QStringLiteral("Env10");
    case QPageSize::DLE:
        return QStringLiteral("EnvDL");
    case QPageSize::DoublePostcard:
        return QStringLiteral("DoublePostcard");
    case QPageSize::Envelope9:
        return QStringLiteral("Env9");
    case QPageSize::Envelope11:
        return QStringLiteral("Env11");
    case QPageSize::Envelope12:
        return QStringLiteral("Env12");
    case QPageSize::Envelope14:
        return QStringLiteral("Env14");
    case QPageSize::EnvelopeC0:
        return QStringLiteral("EnvC0");
    case QPageSize::EnvelopeC1:
        return QStringLiteral("EnvC1");
    case QPageSize::EnvelopeC2:
        return QStringLiteral("EnvC2");
    case QPageSize::EnvelopeC3:
        return QStringLiteral("EnvC3");
    case QPageSize::EnvelopeC4:
        return QStringLiteral("EnvC4");
    case QPageSize::EnvelopeC6:
        return QStringLiteral("EnvC6");
    case QPageSize::EnvelopeC65:
        return QStringLiteral("EnvC65");
    case QPageSize::EnvelopeC7:
        return QStringLiteral("EnvC7");
    case QPageSize::EnvelopeChou3:
        return QStringLiteral("EnvChou3");
    case QPageSize::EnvelopeChou4:
        return QStringLiteral("EnvChou4");
    case QPageSize::EnvelopeInvite:
        return QStringLiteral("EnvInvite");
    case QPageSize::EnvelopeItalian:
        return QStringLiteral("EnvItalian");
    case QPageSize::EnvelopeKaku2:
        return QStringLiteral("EnvKaku2");
    case QPageSize::EnvelopeKaku3:
        return QStringLiteral("EnvKaku3");
    case QPageSize::EnvelopeMonarch:
        return QStringLiteral("EnvMonarch");
    case QPageSize::EnvelopePersonal:
        return QStringLiteral("EnvPersonal");
    case QPageSize::EnvelopePrc1:
        return QStringLiteral("EnvPRC1");
    case QPageSize::EnvelopePrc10:
        return QStringLiteral("EnvPRC10");
    case QPageSize::EnvelopePrc2:
        return QStringLiteral("EnvPRC2");
    case QPageSize::EnvelopePrc3:
        return QStringLiteral("EnvPRC3");
    case QPageSize::EnvelopePrc4:
        return QStringLiteral("EnvPRC4");
    case QPageSize::EnvelopePrc5:
        return QStringLiteral("EnvPRC5");
    case QPageSize::EnvelopePrc6:
        return QStringLiteral("EnvPRC6");
    case QPageSize::EnvelopePrc7:
        return QStringLiteral("EnvPRC7");
    case QPageSize::EnvelopePrc8:
        return QStringLiteral("EnvPRC8");
    case QPageSize::EnvelopePrc9:
        return QStringLiteral("EnvPRC9");
    case QPageSize::EnvelopeYou4:
        return QStringLiteral("EnvYou4");
    case QPageSize::Executive:
        return QStringLiteral("Executive");
    case QPageSize::FanFoldGerman:
        return QStringLiteral("FanFoldGerman");
    case QPageSize::FanFoldGermanLegal:
        return QStringLiteral("FanFoldGermanLegal");
    case QPageSize::Folio:
        return QStringLiteral("Folio");
    case QPageSize::Imperial10x11:
        return QStringLiteral("10x11");
    case QPageSize::Imperial10x13:
        return QStringLiteral("10x13");
    case QPageSize::Imperial10x14:
        return QStringLiteral("10x14");
    case QPageSize::Imperial7x9:
        return QStringLiteral("7x9");
    case QPageSize::Imperial8x10:
        return QStringLiteral("8x10");
    case QPageSize::Imperial9x11:
        return QStringLiteral("9x11");
    case QPageSize::Imperial9x12:
        return QStringLiteral("ARCHA");
    case QPageSize::JisB0:
        return QStringLiteral("B0");
    case QPageSize::JisB1:
        return QStringLiteral("B1");
    case QPageSize::JisB10:
        return QStringLiteral("B10");
    case QPageSize::JisB2:
        return QStringLiteral("B2");
    case QPageSize::JisB3:
        return QStringLiteral("B3");
    case QPageSize::JisB4:
        return QStringLiteral("B4");
    case QPageSize::JisB5:
        return QStringLiteral("B5");
    case QPageSize::JisB6:
        return QStringLiteral("B6");
    case QPageSize::JisB7:
        return QStringLiteral("B7");
    case QPageSize::JisB8:
        return QStringLiteral("B8");
    case QPageSize::JisB9:
        return QStringLiteral("B9");
    case QPageSize::Ledger:
        return QStringLiteral("Ledger"); // Tabloid?
    case QPageSize::Legal:
        return QStringLiteral("Legal");
    case QPageSize::LegalExtra:
        return QStringLiteral("LegalExtra");
    case QPageSize::Letter:
        return QStringLiteral("Letter");
    case QPageSize::LetterExtra:
        return QStringLiteral("LetterExtra");
    case QPageSize::LetterPlus:
        return QStringLiteral("LetterPlus");
    case QPageSize::Note:
        return QStringLiteral("Letter");
    case QPageSize::Postcard:
        return QStringLiteral("Postcard");
    case QPageSize::Prc16K:
        return QStringLiteral("PRC16K");
    case QPageSize::Prc32K:
        return QStringLiteral("PRC32K");
    case QPageSize::Prc32KBig:
        return QStringLiteral("PRC32KBig");
    case QPageSize::Quarto:
        return QStringLiteral("Quarto");
    case QPageSize::Statement:
        return QStringLiteral("Statement");
    case QPageSize::SuperA:
        return QStringLiteral("SuperA");
    case QPageSize::SuperB:
        return QStringLiteral("SuperB");
    case QPageSize::Tabloid:
        return QStringLiteral("Tabloid");
    case QPageSize::Custom:
        return QStringLiteral("Custom.%1x%2mm").arg(printer.widthMM()).arg(printer.heightMM());
    default:
        return QString();
    }
}

// What about Upper and MultiPurpose?  And others in PPD???
QString FilePrinter::mediaPaperSource(QPrinter &printer)
{
    switch (printer.paperSource()) {
    case QPrinter::Auto:
        return QString();
    case QPrinter::Cassette:
        return QStringLiteral("Cassette");
    case QPrinter::Envelope:
        return QStringLiteral("Envelope");
    case QPrinter::EnvelopeManual:
        return QStringLiteral("EnvelopeManual");
    case QPrinter::FormSource:
        return QStringLiteral("FormSource");
    case QPrinter::LargeCapacity:
        return QStringLiteral("LargeCapacity");
    case QPrinter::LargeFormat:
        return QStringLiteral("LargeFormat");
    case QPrinter::Lower:
        return QStringLiteral("Lower");
    case QPrinter::MaxPageSource:
        return QStringLiteral("MaxPageSource");
    case QPrinter::Middle:
        return QStringLiteral("Middle");
    case QPrinter::Manual:
        return QStringLiteral("Manual");
    case QPrinter::OnlyOne:
        return QStringLiteral("OnlyOne");
    case QPrinter::Tractor:
        return QStringLiteral("Tractor");
    case QPrinter::SmallFormat:
        return QStringLiteral("SmallFormat");
    default:
        return QString();
    }
}

QStringList FilePrinter::optionOrientation(QPrinter &printer, QPageLayout::Orientation documentOrientation)
{
    // portrait and landscape options rotate the document according to the document orientation
    // If we want to print a landscape document as one would expect it, we have to pass the
    // portrait option so that the document is not rotated additionally
    if (printer.pageLayout().orientation() == documentOrientation) {
        // the user wants the document printed as is
        return QStringList(QStringLiteral("-o")) << QStringLiteral("portrait");
    } else {
        // the user expects the document being rotated by 90 degrees
        return QStringList(QStringLiteral("-o")) << QStringLiteral("landscape");
    }
}

QStringList FilePrinter::optionDoubleSidedPrinting(QPrinter &printer)
{
    switch (printer.duplex()) {
    case QPrinter::DuplexNone:
        return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=one-sided");
    case QPrinter::DuplexAuto:
        if (printer.pageLayout().orientation() == QPageLayout::Landscape) {
            return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-short-edge");
        } else {
            return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-long-edge");
        }
    case QPrinter::DuplexLongSide:
        return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-long-edge");
    case QPrinter::DuplexShortSide:
        return QStringList(QStringLiteral("-o")) << QStringLiteral("sides=two-sided-short-edge");
    default:
        return QStringList(); // Use printer default
    }
}

QStringList FilePrinter::optionPageOrder(QPrinter &printer)
{
    if (printer.pageOrder() == QPrinter::LastPageFirst) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("outputorder=reverse");
    }
    return QStringList(QStringLiteral("-o")) << QStringLiteral("outputorder=normal");
}

QStringList FilePrinter::optionCollateCopies(QPrinter &printer)
{
    if (printer.collateCopies()) {
        return QStringList(QStringLiteral("-o")) << QStringLiteral("Collate=True");
    }
    return QStringList(QStringLiteral("-o")) << QStringLiteral("Collate=False");
}

QStringList FilePrinter::optionPageMargins(QPrinter &printer, ScaleMode scaleMode)
{
    if (printer.printEngine()->property(QPrintEngine::PPK_PageMargins).isNull()) {
        return QStringList();
    } else {
        qreal l(0), t(0), r(0), b(0);
        if (!printer.fullPage()) {
            auto marginsf = printer.pageLayout().margins(QPageLayout::Point);
            l = marginsf.left();
            t = marginsf.top();
            r = marginsf.right();
            b = marginsf.bottom();
        }
        QStringList marginOptions;
        marginOptions << (QStringLiteral("-o")) << QStringLiteral("page-left=%1").arg(l) << QStringLiteral("-o") << QStringLiteral("page-top=%1").arg(t) << QStringLiteral("-o") << QStringLiteral("page-right=%1").arg(r)
                      << QStringLiteral("-o") << QStringLiteral("page-bottom=%1").arg(b);
        if (scaleMode == ScaleMode::FitToPrintArea) {
            marginOptions << QStringLiteral("-o") << QStringLiteral("fit-to-page");
        }

        return marginOptions;
    }
}

QStringList FilePrinter::optionCupsProperties(QPrinter &printer)
{
    QStringList dialogOptions = printer.printEngine()->property(QPrintEngine::PrintEnginePropertyKey(0xfe00)).toStringList();
    QStringList cupsOpts;

    for (int i = 0; i < dialogOptions.count(); i = i + 2) {
        if (dialogOptions[i + 1].isEmpty()) {
            cupsOpts << QStringLiteral("-o") << dialogOptions[i];
        } else {
            cupsOpts << QStringLiteral("-o") << dialogOptions[i] + QLatin1Char('=') + dialogOptions[i + 1];
        }
    }

    return cupsOpts;
}

/* kate: replace-tabs on; indent-width 4; */
