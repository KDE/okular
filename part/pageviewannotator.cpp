/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pageviewannotator.h"

// qt / kde includes
#include <KLocalizedString>
#include <KMessageBox>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QList>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QPainter>
#include <QSet>
#include <QVariant>

#include <KUser>
#include <QDebug>
#include <QMenu>

// system includes
#include <QKeyEvent>
#include <QStandardPaths>
#include <QTabletEvent>
#include <math.h>
#include <memory>

// local includes
#include "annotationactionhandler.h"
#include "core/annotations.h"
#include "core/area.h"
#include "core/document.h"
#include "core/page.h"
#include "core/signatureutils.h"
#include "debug_ui.h"
#include "editannottooldialog.h"
#include "guiutils.h"
#include "pageview.h"
#include "settings.h"

/** @short PickPointEngine */
class PickPointEngine : public AnnotatorEngine
{
public:
    PickPointEngine(const QDomElement &engineElement)
        : AnnotatorEngine(engineElement)
        , clicked(false)
        , xscale(1.0)
        , yscale(1.0)
    {
        // parse engine specific attributes
        hoverIconName = engineElement.attribute(QStringLiteral("hoverIcon"));
        iconName = m_annotElement.attribute(QStringLiteral("icon"));
        if (m_annotElement.attribute(QStringLiteral("type")) == QLatin1String("Stamp") && !iconName.simplified().isEmpty())
            hoverIconName = iconName;
        center = QVariant(engineElement.attribute(QStringLiteral("center"))).toBool();
        bool ok = true;
        size = engineElement.attribute(QStringLiteral("size"), QStringLiteral("32")).toInt(&ok);
        if (!ok)
            size = 32;
        m_block = QVariant(engineElement.attribute(QStringLiteral("block"))).toBool();

        // create engine objects
        if (!hoverIconName.simplified().isEmpty())
            pixmap = GuiUtils::loadStamp(hoverIconName, size);
    }

    QRect event(EventType type, Button button, Modifiers modifiers, double nX, double nY, double xScale, double yScale, const Okular::Page *page) override
    {
        xscale = xScale;
        yscale = yScale;
        pagewidth = page->width();
        pageheight = page->height();
        // only proceed if pressing left button
        if (button != Left)
            return QRect();

        // start operation on click
        if (type == Press && clicked == false) {
            clicked = true;
            startpoint.x = nX;
            startpoint.y = nY;
        }
        // repaint if moving while pressing
        else if (type == Move && clicked == true) {
        }
        // operation finished on release
        else if (type == Release && clicked == true) {
            m_creationCompleted = true;
        } else
            return QRect();

        // Constrain to 1:1 form factor (e.g. circle or square)
        if (modifiers.constrainRatioAndAngle) {
            double side = qMin(qAbs(nX - startpoint.x) * xScale, qAbs(nY - startpoint.y) * yScale);
            nX = qBound(startpoint.x - side / xScale, nX, startpoint.x + side / xScale);
            nY = qBound(startpoint.y - side / yScale, nY, startpoint.y + side / yScale);
        }
        // update variables and extents (zoom invariant rect)
        point.x = nX;
        point.y = nY;
        if (center) {
            rect.left = nX - (size / (xScale * 2.0));
            rect.top = nY - (size / (yScale * 2.0));
        } else {
            rect.left = nX;
            rect.top = nY;
        }
        rect.right = rect.left + size;
        rect.bottom = rect.top + size;
        QRect boundrect = rect.geometry((int)xScale, (int)yScale).adjusted(0, 0, 1, 1);
        if (m_block) {
            const Okular::NormalizedRect tmprect(qMin(startpoint.x, point.x), qMin(startpoint.y, point.y), qMax(startpoint.x, point.x), qMax(startpoint.y, point.y));
            boundrect |= tmprect.geometry((int)xScale, (int)yScale).adjusted(0, 0, 1, 1);
        }
        return boundrect;
    }

    void paint(QPainter *painter, double xScale, double yScale, const QRect & /*clipRect*/) override
    {
        if (clicked) {
            if (m_block) {
                const QPen origpen = painter->pen();
                QPen pen = painter->pen();
                pen.setStyle(Qt::DashLine);
                painter->setPen(pen);
                const Okular::NormalizedRect tmprect(qMin(startpoint.x, point.x), qMin(startpoint.y, point.y), qMax(startpoint.x, point.x), qMax(startpoint.y, point.y));
                const QRect realrect = tmprect.geometry((int)xScale, (int)yScale);
                painter->drawRect(realrect);
                painter->setPen(origpen);
            }
            if (!pixmap.isNull())
                painter->drawPixmap(QPointF(rect.left * xScale, rect.top * yScale), pixmap);
        }
    }

    void addInPlaceTextAnnotation(Okular::Annotation *&ann, const QString &summary, const QString &content, Okular::TextAnnotation::InplaceIntent inplaceIntent)
    {
        Okular::TextAnnotation *ta = new Okular::TextAnnotation();
        ann = ta;
        ta->setFlags(ta->flags() | Okular::Annotation::FixedRotation);
        ta->setContents(content);
        ta->setTextType(Okular::TextAnnotation::InPlace);
        ta->setInplaceIntent(inplaceIntent);
        // set alignment
        if (m_annotElement.hasAttribute(QStringLiteral("align")))
            ta->setInplaceAlignment(m_annotElement.attribute(QStringLiteral("align")).toInt());
        // set font
        if (m_annotElement.hasAttribute(QStringLiteral("font"))) {
            QFont f;
            f.fromString(m_annotElement.attribute(QStringLiteral("font")));
            ta->setTextFont(f);
        }
        // set font color
        if (m_annotElement.hasAttribute(QStringLiteral("textColor"))) {
            if (inplaceIntent == Okular::TextAnnotation::TypeWriter)
                ta->setTextColor(m_annotElement.attribute(QStringLiteral("textColor")));
            else
                ta->setTextColor(Qt::black);
        }
        // set width
        if (m_annotElement.hasAttribute(QStringLiteral("width"))) {
            ta->style().setWidth(m_annotElement.attribute(QStringLiteral("width")).toDouble());
        }
        // set boundary
        rect.left = qMin(startpoint.x, point.x);
        rect.top = qMin(startpoint.y, point.y);
        rect.right = qMax(startpoint.x, point.x);
        rect.bottom = qMax(startpoint.y, point.y);
        qCDebug(OkularUiDebug).nospace() << "xyScale=" << xscale << "," << yscale;
        static const int padding = 2;
        const QFontMetricsF mf(ta->textFont());
        const QRectF rcf =
            mf.boundingRect(Okular::NormalizedRect(rect.left, rect.top, 1.0, 1.0).geometry((int)pagewidth, (int)pageheight).adjusted(padding, padding, -padding, -padding), Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, ta->contents());
        rect.right = qMax(rect.right, rect.left + (rcf.width() + padding * 2) / pagewidth);
        rect.bottom = qMax(rect.bottom, rect.top + (rcf.height() + padding * 2) / pageheight);
        ta->window().setSummary(summary);
    }

    QList<Okular::Annotation *> end() override
    {
        // find out annotation's description node
        if (m_annotElement.isNull()) {
            m_creationCompleted = false;
            clicked = false;
            return QList<Okular::Annotation *>();
        }

        // find out annotation's type
        Okular::Annotation *ann = nullptr;
        const QString typeString = m_annotElement.attribute(QStringLiteral("type"));
        // create InPlace TextAnnotation from path
        if (typeString == QLatin1String("FreeText")) {
            bool resok;
            const QString content = QInputDialog::getMultiLineText(nullptr, i18n("New Text Note"), i18n("Text of the new note:"), QString(), &resok);
            if (resok)
                addInPlaceTextAnnotation(ann, i18n("Inline Note"), content, Okular::TextAnnotation::Unknown);
        } else if (typeString == QLatin1String("Typewriter")) {
            bool resok;
            const QString content = QInputDialog::getMultiLineText(nullptr, i18n("New Text Note"), i18n("Text of the new note:"), QString(), &resok);
            if (resok)
                addInPlaceTextAnnotation(ann, i18n("Typewriter"), content, Okular::TextAnnotation::TypeWriter);
        } else if (typeString == QLatin1String("Text")) {
            Okular::TextAnnotation *ta = new Okular::TextAnnotation();
            ann = ta;
            ta->setTextType(Okular::TextAnnotation::Linked);
            ta->setTextIcon(iconName);
            // ta->window.flags &= ~(Okular::Annotation::Hidden);
            const double iconhei = 0.03;
            rect.left = point.x;
            rect.top = point.y;
            rect.right = rect.left + iconhei;
            rect.bottom = rect.top + iconhei * xscale / yscale;
            ta->window().setSummary(i18n("Pop-up Note"));
        }
        // create StampAnnotation from path
        else if (typeString == QLatin1String("Stamp")) {
            Okular::StampAnnotation *sa = new Okular::StampAnnotation();
            ann = sa;
            sa->setStampIconName(iconName);
            // set boundary
            rect.left = qMin(startpoint.x, point.x);
            rect.top = qMin(startpoint.y, point.y);
            rect.right = qMax(startpoint.x, point.x);
            rect.bottom = qMax(startpoint.y, point.y);
            const QRectF rcf = rect.geometry((int)xscale, (int)yscale);
            const int ml = (rcf.bottomRight() - rcf.topLeft()).toPoint().manhattanLength();
            if (ml <= QApplication::startDragDistance()) {
                const double stampxscale = pixmap.width() / xscale;
                const double stampyscale = pixmap.height() / yscale;
                if (center) {
                    rect.left = point.x - stampxscale / 2;
                    rect.top = point.y - stampyscale / 2;
                } else {
                    rect.left = point.x;
                    rect.top = point.y;
                }
                rect.right = rect.left + stampxscale;
                rect.bottom = rect.top + stampyscale;
            }
        }
        // create GeomAnnotation
        else if (typeString == QLatin1String("GeomSquare") || typeString == QLatin1String("GeomCircle")) {
            Okular::GeomAnnotation *ga = new Okular::GeomAnnotation();
            ann = ga;
            // set the type
            if (typeString == QLatin1String("GeomSquare"))
                ga->setGeometricalType(Okular::GeomAnnotation::InscribedSquare);
            else
                ga->setGeometricalType(Okular::GeomAnnotation::InscribedCircle);
            if (m_annotElement.hasAttribute(QStringLiteral("width")))
                ann->style().setWidth(m_annotElement.attribute(QStringLiteral("width")).toDouble());
            if (m_annotElement.hasAttribute(QStringLiteral("innerColor")))
                ga->setGeometricalInnerColor(QColor(m_annotElement.attribute(QStringLiteral("innerColor"))));
            // set boundary
            rect.left = qMin(startpoint.x, point.x);
            rect.top = qMin(startpoint.y, point.y);
            rect.right = qMax(startpoint.x, point.x);
            rect.bottom = qMax(startpoint.y, point.y);
        }

        m_creationCompleted = false;
        clicked = false;

        // safety check
        if (!ann)
            return QList<Okular::Annotation *>();

        // set common attributes
        ann->style().setColor(m_annotElement.hasAttribute(QStringLiteral("color")) ? m_annotElement.attribute(QStringLiteral("color")) : m_engineColor);
        if (m_annotElement.hasAttribute(QStringLiteral("opacity")))
            ann->style().setOpacity(m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble());

        // set the bounding rectangle, and make sure that the newly created
        // annotation lies within the page by translating it if necessary
        if (rect.right > 1) {
            rect.left -= rect.right - 1;
            rect.right = 1;
        }
        if (rect.bottom > 1) {
            rect.top -= rect.bottom - 1;
            rect.bottom = 1;
        }
        ann->setBoundingRectangle(rect);

        // return annotation
        return QList<Okular::Annotation *>() << ann;
    }

protected:
    bool clicked;
    bool m_block;
    double xscale, yscale;
    Okular::NormalizedRect rect;
    Okular::NormalizedPoint startpoint;
    Okular::NormalizedPoint point;

private:
    QPixmap pixmap;
    QString hoverIconName, iconName;
    int size;
    double pagewidth, pageheight;
    bool center;
};

class PickPointEngineSignature : public PickPointEngine
{
public:
    PickPointEngineSignature(Okular::Document *document, PageView *pageView)
        : PickPointEngine({})
        , m_document(document)
        , m_page(nullptr)
        , m_pageView(pageView)
        , m_startOver(false)
    {
        m_block = true;
    }

    QRect event(EventType type, Button button, Modifiers modifiers, double nX, double nY, double xScale, double yScale, const Okular::Page *page) override
    {
        m_page = page;
        return PickPointEngine::event(type, button, modifiers, nX, nY, xScale, yScale, page);
    }

    QList<Okular::Annotation *> end() override
    {
        m_startOver = false;
        rect.left = qMin(startpoint.x, point.x);
        rect.top = qMin(startpoint.y, point.y);
        rect.right = qMax(startpoint.x, point.x);
        rect.bottom = qMax(startpoint.y, point.y);

        // FIXME this is a bit arbitrary, try to figure out a better rule, potentially based in cm and not pixels?
        if (rect.width() * m_page->width() < 100 || rect.height() * m_page->height() < 100) {
            const KMessageBox::ButtonCode answer = KMessageBox::questionYesNo(
                m_pageView,
                xi18nc("@info", "A signature of this size may be too small to read. If you would like to create a potentially more readable signature, press <interface>Start over</interface> and draw a bigger rectangle."),
                QString(),
                KGuiItem(i18n("Start over")),
                KGuiItem(i18n("Sign")),
                QStringLiteral("TooSmallDigitalSignatureQuestion"));
            if (answer == KMessageBox::Yes) {
                m_startOver = true;
                return {};
            }
        }

        const Okular::CertificateStore *certStore = m_document->certificateStore();
        bool userCancelled;
        const QList<Okular::CertificateInfo *> &certs = certStore->signingCertificates(&userCancelled);

        if (userCancelled) {
            return {};
        }

        QStringList items;
        QHash<QString, Okular::CertificateInfo *> nickToCert;
        const QDateTime now = QDateTime::currentDateTime();
        for (auto cert : certs) {
            if (cert->validityStart() <= now && now <= cert->validityEnd()) {
                items.append(cert->nickName());
                nickToCert[cert->nickName()] = cert;
            }
        }

        if (items.isEmpty()) {
            m_creationCompleted = false;
            clicked = false;
            if (certs.isEmpty()) {
                KMessageBox::information(m_pageView,
                                         i18n("There are no available signing certificates.<br/>For more information, please see the section about <a href=\"%1\">Adding Digital Signatures</a> in the manual.",
                                              QStringLiteral("help:/okular/signatures.html#adding_digital_signatures")),
                                         QString(),
                                         QString(),
                                         KMessageBox::Notify | KMessageBox::AllowLink);
            } else {
                KMessageBox::information(m_pageView, i18n("All your signing certificates are either not valid yet or are past their validity date."));
            }
            return {};
        }

        bool resok = false;
        certNicknameToUse = QInputDialog::getItem(m_pageView, i18n("Select certificate to sign with"), i18n("Certificates:"), items, 0, false, &resok);

        if (resok) {
            // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
            // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
            // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
            Okular::CertificateInfo *cert = nickToCert.value(certNicknameToUse);
            bool passok = cert->checkPassword(QString());
            while (!passok) {
                const QString title = i18n("Enter password (if any) to unlock certificate: %1", certNicknameToUse);
                bool ok;
                passToUse = QInputDialog::getText(m_pageView, i18n("Enter certificate password"), title, QLineEdit::Password, QString(), &ok);
                if (ok) {
                    passok = cert->checkPassword(passToUse);
                } else {
                    break;
                }
            }

            if (passok) {
                certCommonName = cert->subjectInfo(Okular::CertificateInfo::CommonName);
            } else {
                certNicknameToUse.clear();
            }
        } else {
            certNicknameToUse.clear();
        }

        m_creationCompleted = false;
        clicked = false;

        qDeleteAll(certs);

        return {};
    }

    bool isAccepted() const
    {
        return !certNicknameToUse.isEmpty();
    }

    bool userWantsToStartOver() const
    {
        return m_startOver;
    }

    bool sign(const QString &newFilePath)
    {
        Okular::NewSignatureData data;
        data.setCertNickname(certNicknameToUse);
        data.setCertSubjectCommonName(certCommonName);
        data.setPassword(passToUse);
        data.setPage(m_page->number());
        data.setBoundingRectangle(rect);
        passToUse.clear();
        return m_document->sign(data, newFilePath);
    }

private:
    QString certNicknameToUse;
    QString certCommonName;
    QString passToUse;

    Okular::Document *m_document;
    const Okular::Page *m_page;
    PageView *m_pageView;

    bool m_startOver;
};

/** @short PolyLineEngine */
class PolyLineEngine : public AnnotatorEngine
{
public:
    PolyLineEngine(const QDomElement &engineElement)
        : AnnotatorEngine(engineElement)
        , last(false)
    {
        // parse engine specific attributes
        m_block = engineElement.attribute(QStringLiteral("block")) == QLatin1String("true");
        bool ok = true;
        // numofpoints represents the max number of points for the current
        // polygon/polyline, with a pair of exceptions:
        // -1 means: the polyline must close on the first point (polygon)
        // 0 means: construct as many points as you want, right-click
        //   to construct the last point
        numofpoints = engineElement.attribute(QStringLiteral("points")).toInt(&ok);
        if (!ok)
            numofpoints = -1;
    }

    static Okular::NormalizedPoint constrainAngle(const Okular::NormalizedPoint &p1, double x, double y, double xScale, double yScale, double angleIncrement)
    {
        // given the normalized point (x, y), return the closest point such that the line segment from p1 forms an angle
        // with the horizontal axis which is an integer multiple of angleIncrement on a reference area of size xScale x yScale
        double dist = sqrt(p1.distanceSqr(x, y, xScale, yScale));
        double angle = atan2((y - p1.y) * yScale, (x - p1.x) * xScale);
        double constrainedAngle = round(angle / angleIncrement) * angleIncrement;
        double offset = dist * sin(angle - constrainedAngle);
        x += offset * sin(constrainedAngle) / xScale;
        y -= offset * cos(constrainedAngle) / yScale;
        return Okular::NormalizedPoint(x, y);
    }

    QRect event(EventType type, Button button, Modifiers modifiers, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/) override
    {
        // only proceed if pressing left button
        //            if ( button != Left )
        //                return rect;

        // Constrain to 15° steps, except first point of course.
        if (modifiers.constrainRatioAndAngle && !points.isEmpty()) {
            const Okular::NormalizedPoint constrainedPoint = constrainAngle(points.constLast(), nX, nY, xScale, yScale, M_PI / 12.);
            nX = constrainedPoint.x;
            nY = constrainedPoint.y;
        }
        // process button press
        if (type == Press) {
            newPoint.x = nX;
            newPoint.y = nY;
            if (button == Right)
                last = true;
        }
        // move the second point
        else if (type == Move) {
            movingpoint.x = nX;
            movingpoint.y = nY;
            const QRect oldmovingrect = movingrect;
            movingrect = rect | QRect((int)(movingpoint.x * xScale), (int)(movingpoint.y * yScale), 1, 1);
            return oldmovingrect | movingrect;
        } else if (type == Release) {
            const Okular::NormalizedPoint tmppoint(nX, nY);
            if (fabs(tmppoint.x - newPoint.x) + fabs(tmppoint.y - newPoint.y) > 1e-2)
                return rect;

            if (numofpoints == -1 && points.count() > 1 && (fabs(points[0].x - newPoint.x) + fabs(points[0].y - newPoint.y) < 1e-2)) {
                last = true;
            } else {
                points.append(newPoint);
                rect |= QRect((int)(newPoint.x * xScale), (int)(newPoint.y * yScale), 1, 1);
            }
            // end creation if we have constructed the last point of enough points
            if (last || points.count() == numofpoints) {
                m_creationCompleted = true;
                last = false;
                normRect = Okular::NormalizedRect(rect, xScale, yScale);
            }
        }

        return rect;
    }

    void paint(QPainter *painter, double xScale, double yScale, const QRect & /*clipRect*/) override
    {
        if (points.count() < 1)
            return;

        if (m_block && points.count() == 2) {
            const Okular::NormalizedPoint first = points[0];
            const Okular::NormalizedPoint second = points[1];
            // draw a semitransparent block around the 2 points
            painter->setPen(m_engineColor);
            painter->setBrush(QBrush(m_engineColor.lighter(), Qt::Dense4Pattern));
            painter->drawRect((int)(first.x * (double)xScale), (int)(first.y * (double)yScale), (int)((second.x - first.x) * (double)xScale), (int)((second.y - first.y) * (double)yScale));
        } else {
            // draw a polyline that connects the constructed points
            painter->setPen(QPen(m_engineColor, 2));
            for (int i = 1; i < points.count(); ++i)
                painter->drawLine((int)(points[i - 1].x * (double)xScale), (int)(points[i - 1].y * (double)yScale), (int)(points[i].x * (double)xScale), (int)(points[i].y * (double)yScale));
            painter->drawLine((int)(points.last().x * (double)xScale), (int)(points.last().y * (double)yScale), (int)(movingpoint.x * (double)xScale), (int)(movingpoint.y * (double)yScale));
        }
    }

    QList<Okular::Annotation *> end() override
    {
        m_creationCompleted = false;

        // find out annotation's description node
        if (m_annotElement.isNull())
            return QList<Okular::Annotation *>();

        // find out annotation's type
        Okular::Annotation *ann = nullptr;
        const QString typeString = m_annotElement.attribute(QStringLiteral("type"));

        // create LineAnnotation from path
        if (typeString == QLatin1String("Line") || typeString == QLatin1String("Polyline") || typeString == QLatin1String("Polygon")) {
            if (points.count() < 2)
                return QList<Okular::Annotation *>();

            // add note
            Okular::LineAnnotation *la = new Okular::LineAnnotation();
            ann = la;
            QLinkedList<Okular::NormalizedPoint> list;
            for (int i = 0; i < points.count(); ++i)
                list.append(points[i]);

            la->setLinePoints(list);

            if (numofpoints == -1) {
                la->setLineClosed(true);
                if (m_annotElement.hasAttribute(QStringLiteral("innerColor")))
                    la->setLineInnerColor(QColor(m_annotElement.attribute(QStringLiteral("innerColor"))));
            } else if (numofpoints == 2) {
                if (m_annotElement.hasAttribute(QStringLiteral("leadFwd")))
                    la->setLineLeadingForwardPoint(m_annotElement.attribute(QStringLiteral("leadFwd")).toDouble());
                if (m_annotElement.hasAttribute(QStringLiteral("leadBack")))
                    la->setLineLeadingBackwardPoint(m_annotElement.attribute(QStringLiteral("leadBack")).toDouble());
            }
            if (m_annotElement.hasAttribute(QStringLiteral("startStyle")))
                la->setLineStartStyle((Okular::LineAnnotation::TermStyle)m_annotElement.attribute(QStringLiteral("startStyle")).toInt());
            if (m_annotElement.hasAttribute(QStringLiteral("endStyle")))
                la->setLineEndStyle((Okular::LineAnnotation::TermStyle)m_annotElement.attribute(QStringLiteral("endStyle")).toInt());

            la->setBoundingRectangle(normRect);
        }

        // safety check
        if (!ann)
            return QList<Okular::Annotation *>();

        if (m_annotElement.hasAttribute(QStringLiteral("width")))
            ann->style().setWidth(m_annotElement.attribute(QStringLiteral("width")).toDouble());

        // set common attributes
        ann->style().setColor(m_annotElement.hasAttribute(QStringLiteral("color")) ? m_annotElement.attribute(QStringLiteral("color")) : m_engineColor);
        if (m_annotElement.hasAttribute(QStringLiteral("opacity")))
            ann->style().setOpacity(m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble());
        // return annotation

        return QList<Okular::Annotation *>() << ann;
    }

private:
    QList<Okular::NormalizedPoint> points;
    Okular::NormalizedPoint newPoint;
    Okular::NormalizedPoint movingpoint;
    QRect rect;
    QRect movingrect;
    Okular::NormalizedRect normRect;
    bool m_block;
    bool last;
    int numofpoints;
};

/** @short TextSelectorEngine */
class TextSelectorEngine : public AnnotatorEngine
{
public:
    TextSelectorEngine(const QDomElement &engineElement, PageView *pageView)
        : AnnotatorEngine(engineElement)
        , m_pageView(pageView)
    {
        // parse engine specific attributes
    }

    QRect event(EventType type, Button button, Modifiers /*modifiers*/, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/) override
    {
        // only proceed if pressing left button
        if (button != Left)
            return QRect();

        if (type == Press) {
            lastPoint.x = nX;
            lastPoint.y = nY;
            const QRect oldrect = rect;
            rect = QRect();
            return oldrect;
        } else if (type == Move) {
            if (item()) {
                const QPoint start((int)(lastPoint.x * item()->uncroppedWidth()), (int)(lastPoint.y * item()->uncroppedHeight()));
                const QPoint end((int)(nX * item()->uncroppedWidth()), (int)(nY * item()->uncroppedHeight()));
                selection.reset();
                std::unique_ptr<Okular::RegularAreaRect> newselection(m_pageView->textSelectionForItem(item(), start, end));
                if (newselection && !newselection->isEmpty()) {
                    const QList<QRect> geom = newselection->geometry((int)xScale, (int)yScale);
                    QRect newrect;
                    for (const QRect &r : geom) {
                        if (newrect.isNull())
                            newrect = r;
                        else
                            newrect |= r;
                    }
                    rect |= newrect;
                    selection = std::move(newselection);
                }
            }
        } else if (type == Release) {
            m_creationCompleted = true;
        }
        return rect;
    }

    void paint(QPainter *painter, double xScale, double yScale, const QRect & /*clipRect*/) override
    {
        if (selection) {
            painter->setPen(Qt::NoPen);
            QColor col = m_engineColor;
            col.setAlphaF(0.5);
            painter->setBrush(col);
            for (const Okular::NormalizedRect &r : qAsConst(*selection)) {
                painter->drawRect(r.geometry((int)xScale, (int)yScale));
            }
        }
    }

    QList<Okular::Annotation *> end() override
    {
        m_creationCompleted = false;

        // safety checks
        if (m_annotElement.isNull() || !selection)
            return QList<Okular::Annotation *>();

        // find out annotation's type
        Okular::Annotation *ann = nullptr;
        const QString typeString = m_annotElement.attribute(QStringLiteral("type"));

        Okular::HighlightAnnotation::HighlightType type = Okular::HighlightAnnotation::Highlight;
        bool typevalid = false;
        // create HighlightAnnotation's from the selected area
        if (typeString == QLatin1String("Highlight")) {
            type = Okular::HighlightAnnotation::Highlight;
            typevalid = true;
        } else if (typeString == QLatin1String("Squiggly")) {
            type = Okular::HighlightAnnotation::Squiggly;
            typevalid = true;
        } else if (typeString == QLatin1String("Underline")) {
            type = Okular::HighlightAnnotation::Underline;
            typevalid = true;
        } else if (typeString == QLatin1String("StrikeOut")) {
            type = Okular::HighlightAnnotation::StrikeOut;
            typevalid = true;
        }
        if (typevalid) {
            Okular::HighlightAnnotation *ha = new Okular::HighlightAnnotation();
            ha->setHighlightType(type);
            ha->setBoundingRectangle(Okular::NormalizedRect(rect, item()->uncroppedWidth(), item()->uncroppedHeight()));
            for (const Okular::NormalizedRect &r : qAsConst(*selection)) {
                Okular::HighlightAnnotation::Quad q;
                q.setCapStart(false);
                q.setCapEnd(false);
                q.setFeather(1.0);
                q.setPoint(Okular::NormalizedPoint(r.left, r.bottom), 0);
                q.setPoint(Okular::NormalizedPoint(r.right, r.bottom), 1);
                q.setPoint(Okular::NormalizedPoint(r.right, r.top), 2);
                q.setPoint(Okular::NormalizedPoint(r.left, r.top), 3);
                ha->highlightQuads().append(q);
            }
            ann = ha;
        }

        selection.reset();

        // safety check
        if (!ann)
            return QList<Okular::Annotation *>();

        // set common attributes
        ann->style().setColor(m_annotElement.hasAttribute(QStringLiteral("color")) ? m_annotElement.attribute(QStringLiteral("color")) : m_engineColor);
        if (m_annotElement.hasAttribute(QStringLiteral("opacity")))
            ann->style().setOpacity(m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble());

        // return annotations
        return QList<Okular::Annotation *>() << ann;
    }

    QCursor cursor() const override
    {
        return Qt::IBeamCursor;
    }

private:
    // data
    PageView *m_pageView;
    // TODO: support more pages
    std::unique_ptr<Okular::RegularAreaRect> selection;
    Okular::NormalizedPoint lastPoint;
    QRect rect;
};

/** @short AnnotationTools*/
class AnnotationTools
{
public:
    AnnotationTools()
        : m_toolsCount(0)
    {
    }

    void setTools(const QStringList &tools)
    {
        // Populate m_toolsDefinition
        m_toolsCount = 0;
        m_toolsDefinition.clear();
        QDomElement root = m_toolsDefinition.createElement(QStringLiteral("root"));
        m_toolsDefinition.appendChild(root);
        for (const QString &toolXml : tools) {
            QDomDocument entryParser;
            if (entryParser.setContent(toolXml)) {
                root.appendChild(m_toolsDefinition.importNode(entryParser.documentElement(), true));
                m_toolsCount++;
            } else {
                qCWarning(OkularUiDebug) << "Skipping malformed tool XML in AnnotationTools setting";
            }
        }
    }

    QStringList toStringList()
    {
        QStringList tools;
        QDomElement toolElement = m_toolsDefinition.documentElement().firstChildElement();
        QString str;
        QTextStream stream(&str);
        while (!toolElement.isNull()) {
            str.clear();
            toolElement.save(stream, -1 /* indent disabled */);
            tools << str;
            toolElement = toolElement.nextSiblingElement();
        }
        return tools;
    }

    QDomElement tool(int toolId)
    {
        QDomElement toolElement = m_toolsDefinition.documentElement().firstChildElement();
        while (!toolElement.isNull() && toolElement.attribute(QStringLiteral("id")).toInt() != toolId) {
            toolElement = toolElement.nextSiblingElement();
        }
        return toolElement; // can return a null element
    }

    void appendTool(QDomElement toolElement)
    {
        toolElement = toolElement.cloneNode().toElement();
        toolElement.setAttribute(QStringLiteral("id"), ++m_toolsCount);
        m_toolsDefinition.documentElement().appendChild(toolElement);
    }

    bool updateTool(QDomElement newToolElement, int toolId)
    {
        QDomElement toolElement = tool(toolId);
        if (toolElement.isNull())
            return false;
        newToolElement = newToolElement.cloneNode().toElement();
        newToolElement.setAttribute(QStringLiteral("id"), toolId);
        QDomNode oldTool = m_toolsDefinition.documentElement().replaceChild(newToolElement, toolElement);
        return !oldTool.isNull();
    }

    int findToolId(const QString &type)
    {
        int toolId = -1;
        if (type.isEmpty()) {
            return -1;
        }
        // FIXME: search from left. currently searching from right side as a workaround to avoid matching
        // straight line tools to the arrow tool, which is also of type straight-line
        QDomElement toolElement = m_toolsDefinition.documentElement().lastChildElement();
        while (!toolElement.isNull() && toolElement.attribute(QStringLiteral("type")) != type) {
            toolElement = toolElement.previousSiblingElement();
        }
        if (!toolElement.isNull() && toolElement.hasAttribute(QStringLiteral("id"))) {
            bool ok;
            toolId = toolElement.attribute(QStringLiteral("id")).toInt(&ok);
            if (!ok) {
                return -1;
            }
        }
        return toolId;
    }

private:
    QDomDocument m_toolsDefinition;
    int m_toolsCount;
};

/** PageViewAnnotator **/
const int PageViewAnnotator::STAMP_TOOL_ID = 14;

PageViewAnnotator::PageViewAnnotator(PageView *parent, Okular::Document *storage)
    : QObject(parent)
    , m_document(storage)
    , m_pageView(parent)
    , m_actionHandler(nullptr)
    , m_engine(nullptr)
    , m_builtinToolsDefinition(nullptr)
    , m_quickToolsDefinition(nullptr)
    , m_continuousMode(true)
    , m_constrainRatioAndAngle(false)
    , m_signatureMode(false)
    , m_lastToolsDefinition(nullptr)
    , m_lastToolId(-1)
    , m_lockedItem(nullptr)
{
    reparseConfig();
    reparseBuiltinToolsConfig();
    reparseQuickToolsConfig();
    connect(Okular::Settings::self(), &Okular::Settings::builtinAnnotationToolsChanged, this, &PageViewAnnotator::reparseBuiltinToolsConfig);
    connect(Okular::Settings::self(), &Okular::Settings::quickAnnotationToolsChanged, this, &PageViewAnnotator::reparseQuickToolsConfig, Qt::QueuedConnection);
}

void PageViewAnnotator::reparseConfig()
{
    m_continuousMode = Okular::Settings::annotationContinuousMode();

    if (Okular::Settings::identityAuthor().isEmpty())
        detachAnnotation();
}

void PageViewAnnotator::reparseBuiltinToolsConfig()
{
    // Read tool list from configuration. It's a list of XML <tool></tool> elements
    if (!m_builtinToolsDefinition)
        m_builtinToolsDefinition = new AnnotationTools();
    m_builtinToolsDefinition->setTools(Okular::Settings::builtinAnnotationTools());

    if (m_actionHandler)
        m_actionHandler->reparseBuiltinToolsConfig();
}

void PageViewAnnotator::reparseQuickToolsConfig()
{
    // Read tool list from configuration. It's a list of XML <tool></tool> elements
    if (!m_quickToolsDefinition)
        m_quickToolsDefinition = new AnnotationTools();
    m_quickToolsDefinition->setTools(Okular::Settings::quickAnnotationTools());

    if (m_actionHandler)
        m_actionHandler->reparseQuickToolsConfig();
}

PageViewAnnotator::~PageViewAnnotator()
{
    delete m_engine;
    delete m_builtinToolsDefinition;
    delete m_quickToolsDefinition;
}

void PageViewAnnotator::setSignatureMode(bool enabled)
{
    m_signatureMode = enabled;
}

bool PageViewAnnotator::signatureMode() const
{
    return m_signatureMode;
}

bool PageViewAnnotator::active() const
{
    return (m_engine != nullptr) || m_signatureMode;
}

bool PageViewAnnotator::annotating() const
{
    return active() && m_lockedItem;
}

QCursor PageViewAnnotator::cursor() const
{
    return m_engine ? m_engine->cursor() : Qt::CrossCursor;
}

QRect PageViewAnnotator::performRouteMouseOrTabletEvent(const AnnotatorEngine::EventType eventType, const AnnotatorEngine::Button button, const AnnotatorEngine::Modifiers modifiers, const QPointF pos, PageViewItem *item)
{
    // creationCompleted is intended to be set by event(), handled subsequently by end(), and cleared within end().
    // If it's set here, we recursed for some reason (e.g., stacked event loop).
    // Just bail out, all we want to do is already on stack.
    if (m_engine && m_engine->creationCompleted()) {
        return QRect();
    }

    // if the right mouse button was pressed, we simply do nothing. In this way, we are still editing the annotation
    // and so this function will receive and process the right mouse button release event too. If we detach now the annotation tool,
    // the release event will be processed by the PageView class which would create the annotation property widget, and we do not want this.
    if (button == AnnotatorEngine::Right && eventType == AnnotatorEngine::Press)
        return QRect();
    else if (button == AnnotatorEngine::Right && eventType == AnnotatorEngine::Release) {
        detachAnnotation();
        return QRect();
    }

    if (signatureMode() && eventType == AnnotatorEngine::Press) {
        m_engine = new PickPointEngineSignature(m_document, m_pageView);
    }

    // 1. lock engine to current item
    if (!m_lockedItem && eventType == AnnotatorEngine::Press) {
        m_lockedItem = item;
        m_engine->setItem(m_lockedItem);
    }
    if (!m_lockedItem) {
        return QRect();
    }

    // find out normalized mouse coords inside current item
    const QRect &itemRect = m_lockedItem->uncroppedGeometry();
    const QPointF eventPos = m_pageView->contentAreaPoint(pos);
    const double nX = qBound(0.0, m_lockedItem->absToPageX(eventPos.x()), 1.0);
    const double nY = qBound(0.0, m_lockedItem->absToPageY(eventPos.y()), 1.0);

    QRect modifiedRect;

    // 2. use engine to perform operations
    const QRect paintRect = m_engine->event(eventType, button, modifiers, nX, nY, itemRect.width(), itemRect.height(), m_lockedItem->page());

    // 3. update absolute extents rect and send paint event(s)
    if (paintRect.isValid()) {
        // 3.1. unite old and new painting regions
        QRegion compoundRegion(m_lastDrawnRect);
        m_lastDrawnRect = paintRect;
        m_lastDrawnRect.translate(itemRect.left(), itemRect.top());
        // 3.2. decompose paint region in rects and send paint events
        const QRegion rgn = compoundRegion.united(m_lastDrawnRect);
        const QPoint areaPos = m_pageView->contentAreaPosition();
        for (const QRect &r : rgn)
            m_pageView->viewport()->update(r.translated(-areaPos));
        modifiedRect = compoundRegion.boundingRect() | m_lastDrawnRect;
    }

    // 4. if engine has finished, apply Annotation to the page
    if (m_engine->creationCompleted()) {
        // apply engine data to the Annotation's and reset engine
        const QList<Okular::Annotation *> annotations = m_engine->end();
        // attach the newly filled annotations to the page
        for (Okular::Annotation *annotation : annotations) {
            if (!annotation)
                continue;

            annotation->setCreationDate(QDateTime::currentDateTime());
            annotation->setModificationDate(QDateTime::currentDateTime());
            annotation->setAuthor(Okular::Settings::identityAuthor());
            m_document->addPageAnnotation(m_lockedItem->pageNumber(), annotation);

            if (annotation->openDialogAfterCreation())
                m_pageView->openAnnotationWindow(annotation, m_lockedItem->pageNumber());
        }

        if (signatureMode()) {
            auto signEngine = static_cast<PickPointEngineSignature *>(m_engine);
            if (signEngine->isAccepted()) {
                QMimeDatabase db;
                const QString typeName = m_document->documentInfo().get(Okular::DocumentInfo::MimeType);
                const QMimeType mimeType = db.mimeTypeForName(typeName);
                const QString mimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", mimeType.comment(), mimeType.globPatterns().join(QLatin1Char(' ')));

                const QUrl currentFileUrl = m_document->currentDocument();
                const QFileInfo currentFileInfo(currentFileUrl.fileName());
                const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QString();
                const QString newFileName =
                    localFilePathIfAny + i18nc("Used when suggesting a new name for a digitally signed file. %1 is the old file name and %2 it's extension", "%1_signed.%2", currentFileInfo.baseName(), currentFileInfo.completeSuffix());

                const QString newFilePath = QFileDialog::getSaveFileName(m_pageView, i18n("Save Signed File As"), newFileName, mimeTypeFilter);

                if (!newFilePath.isEmpty()) {
                    const bool success = static_cast<PickPointEngineSignature *>(m_engine)->sign(newFilePath);
                    if (success) {
                        emit requestOpenFile(newFilePath, m_lockedItem->pageNumber() + 1);
                    } else {
                        KMessageBox::error(m_pageView, i18nc("%1 is a file path", "Could not sign. Invalid certificate password or could not write to '%1'", newFilePath));
                    }
                }
            } else if (signEngine->userWantsToStartOver()) {
                delete m_engine;
                m_engine = new PickPointEngineSignature(m_document, m_pageView);
                return {};
            }
            m_continuousMode = false;
        }

        if (m_continuousMode)
            selectLastTool();
        else
            detachAnnotation();
    }

    return modifiedRect;
}

QRect PageViewAnnotator::routeMouseEvent(QMouseEvent *e, PageViewItem *item)
{
    AnnotatorEngine::EventType eventType;
    AnnotatorEngine::Button button;
    AnnotatorEngine::Modifiers modifiers;

    // figure out the event type and button
    AnnotatorEngine::decodeEvent(e, &eventType, &button);

    // Constrain angle if action checked XOR shift button pressed.
    modifiers.constrainRatioAndAngle = (bool(constrainRatioAndAngleActive()) != bool(e->modifiers() & Qt::ShiftModifier));

    return performRouteMouseOrTabletEvent(eventType, button, modifiers, e->localPos(), item);
}

QRect PageViewAnnotator::routeTabletEvent(QTabletEvent *e, PageViewItem *item, const QPoint localOriginInGlobal)
{
    // Unlike routeMouseEvent, routeTabletEvent must explicitly ignore events it doesn't care about so that
    // the corresponding mouse event will later be delivered.
    if (!item) {
        e->ignore();
        return QRect();
    }

    AnnotatorEngine::EventType eventType;
    AnnotatorEngine::Button button;
    AnnotatorEngine::Modifiers modifiers;

    // figure out the event type and button
    AnnotatorEngine::decodeEvent(e, &eventType, &button);

    // Constrain angle if action checked XOR shift button pressed.
    modifiers.constrainRatioAndAngle = (bool(constrainRatioAndAngleActive()) != bool(e->modifiers() & Qt::ShiftModifier));

    const QPointF globalPosF = e->globalPosF();
    const QPointF localPosF = globalPosF - localOriginInGlobal;
    return performRouteMouseOrTabletEvent(eventType, button, modifiers, localPosF, item);
}

bool PageViewAnnotator::routeKeyEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        detachAnnotation();
        return true;
    }
    return false;
}

bool PageViewAnnotator::routePaints(const QRect wantedRect) const
{
    return m_engine && wantedRect.intersects(m_lastDrawnRect) && m_lockedItem;
}

void PageViewAnnotator::routePaint(QPainter *painter, const QRect paintRect)
{
    // if there's no locked item, then there's no decided place to draw on
    if (!m_lockedItem)
        return;

#ifndef NDEBUG
    // [DEBUG] draw the paint region if enabled
    if (Okular::Settings::debugDrawAnnotationRect())
        painter->drawRect(paintRect);
#endif
    // move painter to current itemGeometry rect
    const QRect &itemRect = m_lockedItem->uncroppedGeometry();
    painter->save();
    painter->translate(itemRect.topLeft());
    // TODO: Clip annotation painting to cropped page.

    // transform cliprect from absolute to item relative coords
    QRect annotRect = paintRect.intersected(m_lastDrawnRect);
    annotRect.translate(-itemRect.topLeft());

    // use current engine for painting (in virtual page coordinates)
    m_engine->paint(painter, m_lockedItem->uncroppedWidth(), m_lockedItem->uncroppedHeight(), annotRect);
    painter->restore();
}

void PageViewAnnotator::selectBuiltinTool(int toolId, ShowTip showTip)
{
    selectTool(m_builtinToolsDefinition, toolId, showTip);
}

void PageViewAnnotator::selectQuickTool(int toolId)
{
    selectTool(m_quickToolsDefinition, toolId, ShowTip::Yes);
}

void PageViewAnnotator::selectTool(AnnotationTools *toolsDefinition, int toolId, ShowTip showTip)
{
    // ask for Author's name if not already set
    if (toolId > 0 && Okular::Settings::identityAuthor().isEmpty()) {
        // get default username from the kdelibs/kdecore/KUser
        KUser currentUser;
        QString userName = currentUser.property(KUser::FullName).toString();
        // ask the user for confirmation/change
        if (userName.isEmpty()) {
            bool ok = false;
            userName = QInputDialog::getText(nullptr, i18n("Bookmark annotation"), i18n("Insert a custom name for the annotation:"), QLineEdit::Normal, QString(), &ok);

            if (!ok) {
                detachAnnotation();
                return;
            }
        }
        // save the name
        Okular::Settings::setIdentityAuthor(userName);
        Okular::Settings::self()->save();
    }

    // terminate any previous operation
    if (m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }
    m_lockedItem = nullptr;
    if (m_lastDrawnRect.isValid()) {
        m_pageView->viewport()->update(m_lastDrawnRect.translated(-m_pageView->contentAreaPosition()));
        m_lastDrawnRect = QRect();
    }

    // store current tool for later usage
    m_lastToolId = toolId;
    m_lastToolsDefinition = toolsDefinition;

    // handle tool deselection
    if (toolId == -1) {
        m_pageView->displayMessage(QString());
        m_pageView->updateCursor();
        emit toolActive(false);
        return;
    }

    // for the selected tool create the Engine
    QDomElement toolElement = toolsDefinition->tool(toolId);
    if (!toolElement.isNull()) {
        // parse tool properties
        QDomElement engineElement = toolElement.firstChildElement(QStringLiteral("engine"));
        if (!engineElement.isNull()) {
            // create the AnnotatorEngine
            QString type = engineElement.attribute(QStringLiteral("type"));
            if (type == QLatin1String("SmoothLine"))
                m_engine = new SmoothPathEngine(engineElement);
            else if (type == QLatin1String("PickPoint"))
                m_engine = new PickPointEngine(engineElement);
            else if (type == QLatin1String("PolyLine"))
                m_engine = new PolyLineEngine(engineElement);
            else if (type == QLatin1String("TextSelector"))
                m_engine = new TextSelectorEngine(engineElement, m_pageView);
            else
                qCWarning(OkularUiDebug).nospace() << "tools.xml: engine type:'" << type << "' is not defined!";

            if (showTip == ShowTip::Yes) {
                // display the tooltip
                const QString annotType = toolElement.attribute(QStringLiteral("type"));
                QString tip;

                if (annotType == QLatin1String("ellipse"))
                    tip = i18nc("Annotation tool", "Draw an ellipse (drag to select a zone)");
                else if (annotType == QLatin1String("highlight"))
                    tip = i18nc("Annotation tool", "Highlight text");
                else if (annotType == QLatin1String("ink"))
                    tip = i18nc("Annotation tool", "Draw a freehand line");
                else if (annotType == QLatin1String("note-inline"))
                    tip = i18nc("Annotation tool", "Inline Text Annotation (drag to select a zone)");
                else if (annotType == QLatin1String("note-linked"))
                    tip = i18nc("Annotation tool", "Put a pop-up note");
                else if (annotType == QLatin1String("polygon"))
                    tip = i18nc("Annotation tool", "Draw a polygon (click on the first point to close it)");
                else if (annotType == QLatin1String("rectangle"))
                    tip = i18nc("Annotation tool", "Draw a rectangle");
                else if (annotType == QLatin1String("squiggly"))
                    tip = i18nc("Annotation tool", "Squiggle text");
                else if (annotType == QLatin1String("stamp"))
                    tip = i18nc("Annotation tool", "Put a stamp symbol");
                else if (annotType == QLatin1String("straight-line"))
                    tip = i18nc("Annotation tool", "Draw a straight line");
                else if (annotType == QLatin1String("strikeout"))
                    tip = i18nc("Annotation tool", "Strike out text");
                else if (annotType == QLatin1String("underline"))
                    tip = i18nc("Annotation tool", "Underline text");
                else if (annotType == QLatin1String("typewriter"))
                    tip = i18nc("Annotation tool", "Typewriter Annotation (drag to select a zone)");

                if (!tip.isEmpty())
                    m_pageView->displayMessage(tip, QString(), PageViewMessage::Annotation);
            }
        }

        // consistency warning
        if (!m_engine) {
            qCWarning(OkularUiDebug) << "tools.xml: couldn't find good engine description. check xml.";
        }

        m_pageView->updateCursor();
    }

    emit toolActive(true);
}

void PageViewAnnotator::selectLastTool()
{
    selectTool(m_lastToolsDefinition, m_lastToolId, ShowTip::No);
}

void PageViewAnnotator::selectStampTool(const QString &stampSymbol)
{
    QDomElement toolElement = builtinTool(STAMP_TOOL_ID);
    QDomElement engineElement = toolElement.firstChildElement(QStringLiteral("engine"));
    QDomElement annotationElement = engineElement.firstChildElement(QStringLiteral("annotation"));
    engineElement.setAttribute(QStringLiteral("hoverIcon"), stampSymbol);
    annotationElement.setAttribute(QStringLiteral("icon"), stampSymbol);
    saveBuiltinAnnotationTools();
    selectBuiltinTool(STAMP_TOOL_ID, ShowTip::Yes);
}

void PageViewAnnotator::detachAnnotation()
{
    if (m_lastToolId == -1) {
        return;
    }
    selectBuiltinTool(-1, ShowTip::No);
    if (!signatureMode()) {
        if (m_actionHandler)
            m_actionHandler->deselectAllAnnotationActions();
    } else {
        m_pageView->displayMessage(QString());
        setSignatureMode(false);
    }
}

QString PageViewAnnotator::defaultToolName(const QDomElement &toolElement)
{
    const QString annotType = toolElement.attribute(QStringLiteral("type"));

    if (annotType == QLatin1String("ellipse"))
        return i18n("Ellipse");
    else if (annotType == QLatin1String("highlight"))
        return i18n("Highlighter");
    else if (annotType == QLatin1String("ink"))
        return i18n("Freehand Line");
    else if (annotType == QLatin1String("note-inline"))
        return i18n("Inline Note");
    else if (annotType == QLatin1String("note-linked"))
        return i18n("Pop-up Note");
    else if (annotType == QLatin1String("polygon"))
        return i18n("Polygon");
    else if (annotType == QLatin1String("rectangle"))
        return i18n("Rectangle");
    else if (annotType == QLatin1String("squiggly"))
        return i18n("Squiggle");
    else if (annotType == QLatin1String("stamp"))
        return i18n("Stamp");
    else if (annotType == QLatin1String("straight-line"))
        return i18n("Straight Line");
    else if (annotType == QLatin1String("strikeout"))
        return i18n("Strike out");
    else if (annotType == QLatin1String("underline"))
        return i18n("Underline");
    else if (annotType == QLatin1String("typewriter"))
        return i18n("Typewriter");
    else
        return QString();
}

QPixmap PageViewAnnotator::makeToolPixmap(const QDomElement &toolElement)
{
    QPixmap pixmap(32 * qApp->devicePixelRatio(), 32 * qApp->devicePixelRatio());
    pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    const QString annotType = toolElement.attribute(QStringLiteral("type"));

    // Load HiDPI variant on HiDPI screen
    QString imageVariant;
    if (qApp->devicePixelRatio() > 1.05) {
        imageVariant = QStringLiteral("@2x");
    }

    // Load base pixmap. We'll draw on top of it
    pixmap.load(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-base-okular" + imageVariant + ".png")));

    /* Parse color, innerColor and icon (if present) */
    QColor engineColor, innerColor, textColor, annotColor;
    QString icon;
    QDomNodeList engineNodeList = toolElement.elementsByTagName(QStringLiteral("engine"));
    if (engineNodeList.size() > 0) {
        QDomElement engineEl = engineNodeList.item(0).toElement();
        if (!engineEl.isNull() && engineEl.hasAttribute(QStringLiteral("color")))
            engineColor = QColor(engineEl.attribute(QStringLiteral("color")));
    }
    QDomNodeList annotationNodeList = toolElement.elementsByTagName(QStringLiteral("annotation"));
    if (annotationNodeList.size() > 0) {
        QDomElement annotationEl = annotationNodeList.item(0).toElement();
        if (!annotationEl.isNull()) {
            if (annotationEl.hasAttribute(QStringLiteral("color")))
                annotColor = annotationEl.attribute(QStringLiteral("color"));
            if (annotationEl.hasAttribute(QStringLiteral("innerColor")))
                innerColor = QColor(annotationEl.attribute(QStringLiteral("innerColor")));
            if (annotationEl.hasAttribute(QStringLiteral("textColor")))
                textColor = QColor(annotationEl.attribute(QStringLiteral("textColor")));
            if (annotationEl.hasAttribute(QStringLiteral("icon")))
                icon = annotationEl.attribute(QStringLiteral("icon"));
        }
    }

    QPainter p(&pixmap);

    if (annotType == QLatin1String("ellipse")) {
        p.setRenderHint(QPainter::Antialiasing);
        if (innerColor.isValid())
            p.setBrush(innerColor);
        p.setPen(QPen(engineColor, 2));
        p.drawEllipse(2, 7, 21, 14);
    } else if (annotType == QLatin1String("highlight")) {
        QImage overlay(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-highlighter-okular-colorizable" + imageVariant + ".png")));
        QImage colorizedOverlay = overlay;
        GuiUtils::colorizeImage(colorizedOverlay, engineColor);

        p.drawImage(QPoint(0, 0), colorizedOverlay);   // Trail
        p.drawImage(QPoint(0, -32), overlay);          // Text + Shadow (uncolorized)
        p.drawImage(QPoint(0, -64), colorizedOverlay); // Pen
    } else if (annotType == QLatin1String("ink")) {
        QImage overlay(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-ink-okular-colorizable" + imageVariant + ".png")));
        QImage colorizedOverlay = overlay;
        GuiUtils::colorizeImage(colorizedOverlay, engineColor);

        p.drawImage(QPoint(0, 0), colorizedOverlay);   // Trail
        p.drawImage(QPoint(0, -32), overlay);          // Shadow (uncolorized)
        p.drawImage(QPoint(0, -64), colorizedOverlay); // Pen
    } else if (annotType == QLatin1String("note-inline")) {
        QImage overlay(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-note-inline-okular-colorizable" + imageVariant + ".png")));
        GuiUtils::colorizeImage(overlay, engineColor);
        p.drawImage(QPoint(0, 0), overlay);
    } else if (annotType == QLatin1String("note-linked")) {
        QImage overlay(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-note-okular-colorizable" + imageVariant + ".png")));
        GuiUtils::colorizeImage(overlay, engineColor);
        p.drawImage(QPoint(0, 0), overlay);
    } else if (annotType == QLatin1String("polygon")) {
        QPainterPath path;
        path.moveTo(0, 7);
        path.lineTo(19, 7);
        path.lineTo(19, 14);
        path.lineTo(23, 14);
        path.lineTo(23, 20);
        path.lineTo(0, 20);
        if (innerColor.isValid())
            p.setBrush(innerColor);
        p.setPen(QPen(engineColor, 1));
        p.drawPath(path);
    } else if (annotType == QLatin1String("rectangle")) {
        p.setRenderHint(QPainter::Antialiasing);
        if (innerColor.isValid())
            p.setBrush(innerColor);
        p.setPen(QPen(engineColor, 2));
        p.drawRect(2, 7, 21, 14);
    } else if (annotType == QLatin1String("squiggly")) {
        QPen pen(engineColor, 1);
        pen.setDashPattern(QVector<qreal>() << 1 << 1);
        p.setPen(pen);
        p.drawLine(1, 13, 16, 13);
        p.drawLine(2, 14, 15, 14);
        p.drawLine(0, 20, 19, 20);
        p.drawLine(1, 21, 18, 21);
    } else if (annotType == QLatin1String("stamp")) {
        QPixmap stamp = GuiUtils::loadStamp(icon, 16, false /* keepAspectRatio */);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPixmap(16, 14, stamp);
    } else if (annotType == QLatin1String("straight-line")) {
        QPainterPath path;
        path.moveTo(1, 8);
        path.lineTo(20, 8);
        path.lineTo(1, 27);
        path.lineTo(20, 27);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(engineColor, 1));
        p.drawPath(path); // TODO To be discussed: This is not a straight line!
    } else if (annotType == QLatin1String("strikeout")) {
        p.setPen(QPen(engineColor, 1));
        p.drawLine(1, 10, 16, 10);
        p.drawLine(0, 17, 19, 17);
    } else if (annotType == QLatin1String("underline")) {
        p.setPen(QPen(engineColor, 1));
        p.drawLine(1, 13, 16, 13);
        p.drawLine(0, 20, 19, 20);
    } else if (annotType == QLatin1String("typewriter")) {
        QImage overlay(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("okular/pics/tool-typewriter-okular-colorizable" + imageVariant + ".png")));
        GuiUtils::colorizeImage(overlay, textColor);
        p.drawImage(QPoint(-2, 2), overlay);
    } else {
        /* Unrecognized annotation type -- It shouldn't happen */
        p.setPen(QPen(engineColor));
        p.drawText(QPoint(20, 31), QStringLiteral("?"));
    }

    return pixmap;
}

void PageViewAnnotator::setupActions(KActionCollection *ac)
{
    if (!m_actionHandler) {
        m_actionHandler = new AnnotationActionHandler(this, ac);
    }
}

void PageViewAnnotator::setupActionsPostGUIActivated()
{
    m_actionHandler->setupAnnotationToolBarVisibilityAction();
}

bool PageViewAnnotator::continuousMode()
{
    return m_continuousMode;
}

void PageViewAnnotator::setContinuousMode(bool enabled)
{
    m_continuousMode = enabled;
    Okular::Settings::setAnnotationContinuousMode(enabled);
    Okular::Settings::self()->save();
}

bool PageViewAnnotator::constrainRatioAndAngleActive()
{
    return m_constrainRatioAndAngle;
}

void PageViewAnnotator::setConstrainRatioAndAngle(bool enabled)
{
    m_constrainRatioAndAngle = enabled;
}

void PageViewAnnotator::setToolsEnabled(bool enabled)
{
    if (m_actionHandler)
        m_actionHandler->setToolsEnabled(enabled);
}

void PageViewAnnotator::setTextToolsEnabled(bool enabled)
{
    if (m_actionHandler)
        m_actionHandler->setTextToolsEnabled(enabled);
}

void PageViewAnnotator::saveBuiltinAnnotationTools()
{
    Okular::Settings::setBuiltinAnnotationTools(m_builtinToolsDefinition->toStringList());
    Okular::Settings::self()->save();
}

QDomElement PageViewAnnotator::builtinTool(int toolId)
{
    return m_builtinToolsDefinition->tool(toolId);
}

QDomElement PageViewAnnotator::quickTool(int toolId)
{
    return m_quickToolsDefinition->tool(toolId);
}

QDomElement PageViewAnnotator::currentEngineElement()
{
    return m_builtinToolsDefinition->tool(m_lastToolId).firstChildElement(QStringLiteral("engine"));
}

QDomElement PageViewAnnotator::currentAnnotationElement()
{
    return currentEngineElement().firstChildElement(QStringLiteral("annotation"));
}

void PageViewAnnotator::setAnnotationWidth(double width)
{
    currentAnnotationElement().setAttribute(QStringLiteral("width"), QString::number(width));
    saveBuiltinAnnotationTools();
    selectLastTool();
}

void PageViewAnnotator::setAnnotationColor(const QColor &color)
{
    currentEngineElement().setAttribute(QStringLiteral("color"), color.name(QColor::HexRgb));
    QDomElement annotationElement = currentAnnotationElement();
    QString annotType = annotationElement.attribute(QStringLiteral("type"));
    if (annotType == QLatin1String("Typewriter")) {
        annotationElement.setAttribute(QStringLiteral("textColor"), color.name(QColor::HexRgb));
    } else {
        annotationElement.setAttribute(QStringLiteral("color"), color.name(QColor::HexRgb));
    }
    saveBuiltinAnnotationTools();
    selectLastTool();
}

void PageViewAnnotator::setAnnotationInnerColor(const QColor &color)
{
    QDomElement annotationElement = currentAnnotationElement();
    if (color == Qt::transparent) {
        annotationElement.removeAttribute(QStringLiteral("innerColor"));
    } else {
        annotationElement.setAttribute(QStringLiteral("innerColor"), color.name(QColor::HexRgb));
    }
    saveBuiltinAnnotationTools();
    selectLastTool();
}

void PageViewAnnotator::setAnnotationOpacity(double opacity)
{
    currentAnnotationElement().setAttribute(QStringLiteral("opacity"), QString::number(opacity));
    saveBuiltinAnnotationTools();
    selectLastTool();
}

void PageViewAnnotator::setAnnotationFont(const QFont &font)
{
    currentAnnotationElement().setAttribute(QStringLiteral("font"), font.toString());
    saveBuiltinAnnotationTools();
    selectLastTool();
}

void PageViewAnnotator::addToQuickAnnotations()
{
    QDomElement sourceToolElement = m_builtinToolsDefinition->tool(m_lastToolId);
    if (sourceToolElement.isNull())
        return;

    // set custom name for quick annotation
    bool ok = false;
    QString itemText = QInputDialog::getText(nullptr, i18n("Add favorite annotation"), i18n("Custom annotation name:"), QLineEdit::Normal, defaultToolName(sourceToolElement), &ok);
    if (!ok)
        return;

    QDomElement toolElement = sourceToolElement.cloneNode().toElement();
    // store name attribute only if the user specified a customized name
    if (!itemText.isEmpty())
        toolElement.setAttribute(QStringLiteral("name"), itemText);
    m_quickToolsDefinition->appendTool(toolElement);
    Okular::Settings::setQuickAnnotationTools(m_quickToolsDefinition->toStringList());
    Okular::Settings::self()->save();
}

void PageViewAnnotator::slotAdvancedSettings()
{
    QDomElement toolElement = m_builtinToolsDefinition->tool(m_lastToolId);

    EditAnnotToolDialog t(nullptr, toolElement, true);
    if (t.exec() != QDialog::Accepted)
        return;

    QDomElement toolElementUpdated = t.toolXml().documentElement();
    int toolId = toolElement.attribute(QStringLiteral("id")).toInt();
    m_builtinToolsDefinition->updateTool(toolElementUpdated, toolId);
    saveBuiltinAnnotationTools();
    selectLastTool();
}

/* kate: replace-tabs on; indent-width 4; */
