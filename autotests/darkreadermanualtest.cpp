/*
    Throwaway manual validation harness for the Dark Reader exact image mask
    feature. Not wired into CMakeLists.txt; compiled and run manually during
    development. Loads a real PDF, renders it once with normal colors and
    once with Dark Reader enabled, and writes both out as PNGs for visual
    inspection.
*/

#include <QApplication>
#include <QDebug>
#include <QImage>
#include <QPainter>

#include "core/document.h"
#include "core/generator.h"
#include "core/observer.h"
#include "core/page.h"
#include "gui/pagepainter.h"
#include "settings_core.h"

class DummyObserver : public Okular::DocumentObserver
{
public:
    void notifyPageChanged(int, int) override
    {
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (argc < 3) {
        qWarning() << "usage:" << argv[0] << "file.pdf out_prefix";
        return 1;
    }

    const QString filePath = QString::fromLocal8Bit(argv[1]);
    const QString outPrefix = QString::fromLocal8Bit(argv[2]);

    DummyObserver observer;
    Okular::Document doc(nullptr);
    doc.addObserver(&observer);

    const Okular::Document::OpenResult r = doc.openDocument(filePath, QUrl::fromLocalFile(filePath), QMimeType());
    if (r != Okular::Document::OpenSuccess) {
        qWarning() << "failed to open" << filePath << (int)r;
        return 1;
    }

    if (doc.pages() < 1) {
        qWarning() << "no pages";
        return 1;
    }

    Okular::Page *page = doc.page(0);
    const int w = 1000;
    const int h = qRound(1000.0 * page->height() / page->width());

    auto renderAndSave = [&](const QString &suffix) {
        Okular::PixmapRequest *req = new Okular::PixmapRequest(&observer, 0, w, h, 1.0, 1, Okular::PixmapRequest::NoFeature);
        doc.requestPixmaps({req});

        // With EnableThreading disabled, rendering happens synchronously
        // inside requestPixmaps(), so the pixmap is already attached to the
        // page by the time we get here.
        QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
        QPainter p(&img);
        PagePainter::paintPageOnPainter(&p, page, &observer, PagePainter::Accessibility, w, h, QRect(0, 0, w, h));
        p.end();

        const QString outPath = outPrefix + suffix + QStringLiteral(".png");
        img.save(outPath);
        qDebug() << "wrote" << outPath;
    };

    Okular::SettingsCore::setEnableThreading(false);

    Okular::SettingsCore::setChangeColors(false);
    renderAndSave(QStringLiteral("_normal"));

    Okular::SettingsCore::setRenderMode(Okular::SettingsCore::EnumRenderMode::DarkReader);
    Okular::SettingsCore::setChangeColors(true);
    renderAndSave(QStringLiteral("_darkreader"));

    return 0;
}
