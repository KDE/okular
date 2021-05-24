/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "testingutils.h"
#include "core/annotations.h"

namespace TestingUtils
{
QString getAnnotationXml(const Okular::Annotation *annotation)
{
    QString annotXmlString;
    QTextStream stream(&annotXmlString, QIODevice::Append);
    annotation->getAnnotationPropertiesDomNode().save(stream, 0);
    return annotXmlString;
}

bool pointListsAlmostEqual(const QLinkedList<Okular::NormalizedPoint> &points1, const QLinkedList<Okular::NormalizedPoint> &points2)
{
    QLinkedListIterator<Okular::NormalizedPoint> it1(points1);
    QLinkedListIterator<Okular::NormalizedPoint> it2(points2);
    while (it1.hasNext() && it2.hasNext()) {
        const Okular::NormalizedPoint &p1 = it1.next();
        const Okular::NormalizedPoint &p2 = it2.next();
        if (!qFuzzyCompare(p1.x, p2.x) || !qFuzzyCompare(p1.y, p2.y)) {
            return false;
        }
    }
    return !it1.hasNext() && !it2.hasNext();
}

QString AnnotationDisposeWatcher::m_disposedAnnotationName = QString(); // krazy:exclude=nullstrassign

QString AnnotationDisposeWatcher::disposedAnnotationName()
{
    return m_disposedAnnotationName;
}

void AnnotationDisposeWatcher::resetDisposedAnnotationName()
{
    m_disposedAnnotationName = QString();
}

void AnnotationDisposeWatcher::disposeAnnotation(const Okular::Annotation *ann)
{
    m_disposedAnnotationName = ann->uniqueName();
}

}
