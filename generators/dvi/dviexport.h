// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
/**
 * \file dviexport.h
 * Distributed under the GNU GPL version 2 or (at your option)
 * any later version. See accompanying file COPYING or copy at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * \author Angus Leeming
 * \author Stefan Kebekus
 *
 * Classes DVIExportToPDF and DVIExportToPS control the export
 * of a DVI file to PDF or PostScript format, respectively.
 * Common functionality is factored out into a common base class,
 * DVIExport which itself derives from KShared allowing easy,
 * polymorphic storage of multiple KSharedPtr<DVIExport> variables
 * in a container of all exported processes.
 */

#ifndef DVIEXPORT_H
#define DVIEXPORT_H

#include <ksharedptr.h>

#include <QObject>
#include <QtGui/QPrinter>


class dviRenderer;
class fontProgressDialog;
class KProcess;
class QStringList;


class DVIExport: public QObject, public KShared
{
  Q_OBJECT
public:
  virtual ~DVIExport();

  /** @c started() Flags whether or not the external process was
   *  spawned successfully.
   *  Can be used to decide whether to discard the DVIExport variable,
   *  or to store it and await notification that the external process
   *  has finished.
   */
  bool started() const { return started_; }

Q_SIGNALS:
  void error( const QString &message, int duration );

protected:
  /** @param parent is stored internally in order to inform the parent
   *  that the external process has finished and that this variable
   *  can be removed from any stores.
   *  @param parent_widget is parent's (otherwise private) parentWidget.
   */
  DVIExport(dviRenderer& parent, QWidget* parent_widget);

  /** A wrapper around fontProgressDialog, also connecting some signals/slots.
   */
  void initialise_progress_dialog(int total_steps,
                                  const QString& label_text,
                                  const QString& whats_this_text,
                                  const QString& tooltip_text);

  /** Spawns the external process having connected slots to the child
   *  process's stdin and stdout streams.
   */
  void start(const QString& command,
             const QStringList& args,
             const QString& working_directory,
             const QString& error_message);

  /** The real implementation of the abort_process() slot that is
   *  called when the fontProcessDialog is closed by the user,
   *  indicating that the export should be halted.
   */
  virtual void abort_process_impl();

  /** The real implementation of the finished() slot that is called
   *  when the external process finishes.
   *  @param exit_code the exit code retuned by the external process.
   */
  virtual void finished_impl(int exit_code);

private slots:
  /// Calls an impl() inline so that derived classes don't need slots.
  void abort_process() { abort_process_impl(); }
  void finished(int exit_code) { finished_impl(exit_code); }

  /** This slot receives all output from the child process's stdin
   *  and stdout streams.
   */
  void output_receiver();

private:
  QString error_message_;
  bool started_;
  KProcess* process_;
  fontProgressDialog* progress_;
  dviRenderer* parent_;
  QWidget* parent_widget_;
};


class DVIExportToPDF : public DVIExport
{
public:
  /** @param parent is stored internally in order to inform the parent
   *  that the external process has finished.
   *  @param parent_widget is passed to all Qt widgets.
   */
  DVIExportToPDF(dviRenderer& parent, QWidget* parent_widget);
};


class DVIExportToPS : public DVIExport
{
public:
  /** @param parent is stored internally in order to inform the parent
   *  that the external process has finished.
   *  @param parent_widget is passed to all Qt widgets.
   *  @param output_name is the name of the PostScript file that is
   *  to contain the exported data. If this variable is empty, the
   *  user will be asked to supply a file name.
   *  @param options extra command line arguments that are to be
   *  passed to the external process's argv command line.
   *  @param printer having generated the PostScript file, it is passed
   *  to @c printer (if not null).
   *  @param orientation the original orientation of the document
   */
  DVIExportToPS(dviRenderer& parent,
                QWidget* parent_widget,
                const QString& output_name,
                const QStringList& options,
                QPrinter* printer,
                bool useFontHinting,
                QPrinter::Orientation orientation = QPrinter::Portrait);

private:
  virtual void abort_process_impl();
  virtual void finished_impl(int exit_code);

  QPrinter* printer_;
  QString output_name_;
  QString tmpfile_name_;
  QPrinter::Orientation orientation_;
};

#endif
