// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
/**
 * \file dvisourceeditor.h
 * Distributed under the GNU GPL version 2 or (at your option)
 * any later version. See accompanying file COPYING or copy at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * \author Angus Leeming
 * \author Stefan Kebekus
 */

#ifndef DVISOURCEEDITOR_H
#define DVISOURCEEDITOR_H

#include <ksharedptr.h>

#include <qobject.h>

class DocumentWidget;
class dviRenderer;
class KProcess;
class QPoint;
class QWidget;


class DVISourceEditor: public QObject, public KShared
{
  Q_OBJECT
public:
  /** @param dvi_file the name of the DVI file to edit.
   *  @param link the source hyperlink.
   *  @param point the position of the mouse pointer in the parent widget
   *  that led to the creation of this variable.
   *  @param win the parent widget.
   *
   *  @c event and @c win are used to control the position of the "flashing
   *  rectangle" that indicates that the external editor is being launched.
   */
  DVISourceEditor(dviRenderer& parent,
                  QWidget* parentWidget,
                  const QString& link,
                  const QPoint& point,
                  DocumentWidget* win);

  ~DVISourceEditor();

  /** @c started() Flags whether or not the external process was
   *  spawned successfully.
   *  Can be used to decide whether to discard the DVIExport variable,
   *  or to store it and await notification that the external process
   *  has finished.
   */
  bool started() const { return started_; }

private slots:
  /** Called when the external process @c process finishes.
   *  @param process handle to the external process. Should be identical
   *  to that stored internally.
   */
  void finished(KProcess* process);

  /** This slot receives all output from the child process's stdin
   *  and stdout streams.
   */
  void output_receiver(KProcess*, char* buffer, int buflen);

private:
  QString error_message_;
  bool started_;
  KProcess* process_;
  dviRenderer* parent_;
  QWidget* parent_widget_;
};

#endif
