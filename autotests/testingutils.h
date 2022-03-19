/*
    SPDX-FileCopyrightText: 2013 Jon Mease <jon.mease@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_TESTINGUTILS_H
#define OKULAR_TESTINGUTILS_H

template<class T> class QList;
class QString;

namespace Okular
{
class NormalizedPoint;
class Annotation;
}

namespace TestingUtils
{
/**
 * Return the XML string associated with an annotation's properties
 */
QString getAnnotationXml(const Okular::Annotation *annotation);

/**
 * Returns true if the pairwise comparison coordinates of points in @p points1 and @p points2 are almost
 * equal (according to qFuzzyCompare)
 */
bool pointListsAlmostEqual(const QList<Okular::NormalizedPoint> &points1, const QList<Okular::NormalizedPoint> &points2);

/*
 * The AnnotationDisposeWatcher class provides a static disposeAnnotation function
 * that may be assigned to an annotation with Annotation::setDisposeDataFunction in order to
 * determine when an annotation has been disposed of.
 */
class AnnotationDisposeWatcher
{
private:
    static QString m_disposedAnnotationName;

public:
    static QString disposedAnnotationName();
    static void resetDisposedAnnotationName();
    static void disposeAnnotation(const Okular::Annotation *ann);
};

}

#endif
