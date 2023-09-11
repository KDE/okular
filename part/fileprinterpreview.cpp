/*
    SPDX-FileCopyrightText: 2007 John Layt <john@layt.net>

    FilePrinterPreview based on KPrintPreview (originally LGPL)
    SPDX-FileCopyrightText: 2007 Alex Merry <huntedhacker@tiscali.co.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fileprinterpreview.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QSize>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QLoggingCategory>
#include <kparts/readonlypart.h>

#include "gui/debug_ui.h"

using namespace Okular;

// This code copied from KPrintPreview by Alex Merry, adapted to do PS files instead of PDF

class Okular::FilePrinterPreviewPrivate
{
public:
    FilePrinterPreviewPrivate(FilePrinterPreview *host, const QString &_filename)
        : q(host)
        , mainWidget(new QWidget(host))
        , previewPart(nullptr)
        , failMessage(nullptr)
        , config(KSharedConfig::openConfig(QStringLiteral("okularrc")))

    {
        mainlayout = new QVBoxLayout(q);
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, q);
        mainlayout->addWidget(buttonBox);
        filename = _filename;
    }

    void getPart();
    bool doPreview();
    void fail();

    FilePrinterPreview *q;

    QWidget *mainWidget;

    QDialogButtonBox *buttonBox;

    QVBoxLayout *mainlayout;

    QString filename;

    KParts::ReadOnlyPart *previewPart;
    QWidget *failMessage;

    KSharedConfig::Ptr config;
    Q_DISABLE_COPY(FilePrinterPreviewPrivate)
};

void FilePrinterPreviewPrivate::getPart()
{
    if (previewPart) {
        qCDebug(OkularUiDebug) << "already got a part";
        return;
    }

    auto result = KPluginFactory::instantiatePlugin<KParts::ReadOnlyPart>(KPluginMetaData(QStringLiteral("okularpart")), q, QVariantList() << QStringLiteral("Print/Preview"));

    if (!result) {
        qCWarning(OkularUiDebug) << "Part creation failed" << result.errorText;
    } else {
        previewPart = result.plugin;
    }
}

bool FilePrinterPreviewPrivate::doPreview()
{
    if (!QFile::exists(filename)) {
        qCWarning(OkularUiDebug) << "Nothing was produced to be previewed";
        return false;
    }

    getPart();
    if (!previewPart) {
        // TODO: error dialog
        qCWarning(OkularUiDebug) << "Could not find a PS viewer for the preview dialog";
        fail();
        return false;
    } else {
        mainlayout->insertWidget(0, previewPart->widget());
        return previewPart->openUrl(QUrl::fromLocalFile(filename));
    }
}

void FilePrinterPreviewPrivate::fail()
{
    if (!failMessage) {
        failMessage = new QLabel(i18n("Could not load print preview part"), q);
    }
    mainlayout->insertWidget(0, failMessage);
}

FilePrinterPreview::FilePrinterPreview(const QString &filename, QWidget *parent)
    : QDialog(parent)
    , d(new FilePrinterPreviewPrivate(this, filename))
{
    qCDebug(OkularUiDebug) << "kdeprint: creating preview dialog";

    // Set up the dialog
    setWindowTitle(i18n("Print Preview"));

    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    KWindowConfig::restoreWindowSize(windowHandle(), d->config->group("Print Preview"));
}

FilePrinterPreview::~FilePrinterPreview()
{
    KConfigGroup group(d->config->group("Print Preview"));
    KWindowConfig::saveWindowSize(windowHandle(), group);

    delete d;
}

QSize FilePrinterPreview::sizeHint() const
{
    // return a more or less useful window size, if not saved already
    return QSize(600, 500);
}

void FilePrinterPreview::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        // being shown for the first time
        if (!d->doPreview()) {
            event->accept();
            return;
        }
    }
    QDialog::showEvent(event);
}

#include "moc_fileprinterpreview.cpp"
