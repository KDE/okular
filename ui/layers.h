#ifndef _OKULAR_LAYERS_H_
#define _OKULAR_LAYERS_H_

#include <qwidget.h>
#include "core/observer.h"

#include "okular_part_export.h"

class QModelIndex;
class QTreeView;
class KTreeViewSearchLine;

namespace Okular {
class Document;
class PartTest;
}

class OKULAR_PART_EXPORT Layers : public QWidget, public Okular::DocumentObserver
{
Q_OBJECT
    friend class Okular::PartTest;

    public:
        Layers(QWidget *parent, Okular::Document *document);
        ~Layers();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );

    signals:
	void hasLayers(bool has);

    private slots:
        void saveSearchOptions();

    private:

        Okular::Document *m_document;
        QTreeView *m_treeView;
        KTreeViewSearchLine *m_searchLine;
};

#endif
