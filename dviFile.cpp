/*
 * Copyright (c) 1994 Paul Vojta.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * NOTE:
 *	xdvi is based on prior work as noted in the modification history, below.
 */

/*
 * DVI previewer for X.
 *
 * Eric Cooper, CMU, September 1985.
 *
 * Code derived from dvi-imagen.c.
 *
 * Modification history:
 * 1/1986	Modified for X.10	--Bob Scheifler, MIT LCS.
 * 7/1988	Modified for X.11	--Mark Eichin, MIT
 * 12/1988	Added 'R' option, toolkit, magnifying glass
 *					--Paul Vojta, UC Berkeley.
 * 2/1989	Added tpic support	--Jeffrey Lee, U of Toronto
 * 4/1989	Modified for System V	--Donald Richardson, Clarkson Univ.
 * 3/1990	Added VMS support	--Scott Allendorf, U of Iowa
 * 7/1990	Added reflection mode	--Michael Pak, Hebrew U of Jerusalem
 * 1/1992	Added greyscale code	--Till Brychcy, Techn. Univ. Muenchen
 *					  and Lee Hetherington, MIT
 * 4/1994	Added DPS support, bounding box
 *					--Ricardo Telichevesky
 *					  and Luis Miguel Silveira, MIT RLE.
 */

#include <config.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktempfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <stdlib.h>
#include <kprocio.h>

extern "C" {
#include "dvi.h"
}

#include "../kviewshell/pageSize.h"
#include "dviFile.h"
#include "fontpool.h"
#include "xdvi.h"


dvifile::dvifile(const dvifile *old, fontPool *fp) 
{
  errorMsg     = QString::null;
  errorCounter = 0;
  page_offset  = 0;
  suggestedPageSize = 0;
  numberOfExternalPSFiles = 0;
  numberOfExternalNONPSFiles = 0;
  sourceSpecialMarker = old->sourceSpecialMarker;

  dviData = old->dviData.copy();

  filename = old->filename;
  size_of_file = old->size_of_file;
  end_pointer = dvi_Data()+size_of_file; 
  if (dvi_Data() == 0) {
    kdError(4300) << "Not enough memory to copy the DVI-file." << endl;
    return;
  }

  font_pool = fp;
  filename = old->filename;
  generatorString = old->generatorString;
  total_pages = old->total_pages;


  tn_table.clear();
  process_preamble();
  find_postamble();
  read_postamble();
  prepare_pages();
}


void dvifile::process_preamble()
{
  command_pointer = dvi_Data();
  
  Q_UINT8 magic_number = readUINT8();
  if (magic_number != PRE) {
    errorMsg = i18n("The DVI file does not start with the preamble.");
    return;
  }
  magic_number =  readUINT8();
  if (magic_number != 2) {
    errorMsg = i18n("The DVI file contains the wrong version of DVI output for this program. "
		    "Hint: If you use the typesetting system Omega, you have to use a special "
		    "program, such as oxdvi.");
    return;
  }
  
  /** numerator, denominator and the magnification value that describe
      how many centimeters there are in one TeX unit, as explained in
      section A.3 of the DVI driver standard, Level 0, published by
      the TUG DVI driver standards committee. */
  Q_UINT32 numerator     = readUINT32();
  Q_UINT32 denominator   = readUINT32();
  _magnification = readUINT32();
  
  cmPerDVIunit =  (double(numerator) / double(denominator)) * (double(_magnification) / 1000.0) * (1.0 / 1e5);
  
  
  // Read the generatorString (such as "TeX output ..." from the
  // DVI-File). The variable "magic_number" holds the length of the
  // string.
  char	job_id[300];
  magic_number = readUINT8();
  strncpy(job_id, (char *)command_pointer, magic_number);
  job_id[magic_number] = '\0';
  generatorString = job_id;
}


/** find_postamble locates the beginning of the postamble and leaves
    the file ready to start reading at that location. */

void dvifile::find_postamble()
{
  // Move backwards through the TRAILER bytes
  command_pointer = dvi_Data() + size_of_file - 1;
  while((*command_pointer == TRAILER) && (command_pointer > dvi_Data()))
    command_pointer--;
  if (command_pointer == dvi_Data()) {
    errorMsg = i18n("The DVI file is badly corrupted. KDVI was not able to find the postamble.");
    return;
  }

  // And this is finally the pointer to the beginning of the postamble
  command_pointer -= 4;
  beginning_of_postamble = readUINT32();
  command_pointer  = dvi_Data() + beginning_of_postamble;
}


void dvifile::read_postamble()
{
  Q_UINT8 magic_byte = readUINT8();
  if (magic_byte != POST) {
    errorMsg = i18n("The postamble does not begin with the POST command.");
    return;
  }
  last_page_offset = readUINT32();

  // Skip the numerator, denominator and magnification, the largest
  // box height and width and the maximal depth of the stack. These
  // are not used at the moment.
  command_pointer += 4 + 4 + 4 + 4 + 4 + 2;

  // The number of pages is more interesting for us.
  total_pages  = readUINT16();

  // As a next step, read the font definitions.
  Q_UINT8 cmnd = readUINT8();
  while (cmnd >= FNTDEF1 && cmnd <= FNTDEF4) {
    Q_UINT32 TeXnumber = readUINT(cmnd-FNTDEF1+1);
    Q_UINT32 checksum  = readUINT32(); // Checksum of the font, as found by TeX in the TFM file

    // Read scale and design factor, and the name of the font. All
    // these are explained in section A.4 of the DVI driver standard,
    // Level 0, published by the TUG DVI driver standards committee
    Q_UINT32 scale     = readUINT32(); 
    Q_UINT32 design    = readUINT32(); 
    Q_UINT16 len       = readUINT8() + readUINT8(); // Length of the font name, including the directory name
    char *fontname  = new char[len + 1];
    strncpy(fontname, (char *)command_pointer, len );
    fontname[len] = '\0';
    command_pointer += len;
    
#ifdef DEBUG_FONTS
    kdDebug(4300) << "Postamble: define font \"" << fontname << "\" scale=" << scale << " design=" << design << endl;
#endif
    
    // According to section A.4 of the DVI driver standard, this font
    // shall be enlarged by the following factor before it is used.
    double enlargement_factor = (double(scale) * double(_magnification))/(double(design) * 1000.0);
    
    if (font_pool != 0) {
      TeXFontDefinition *fontp = font_pool->appendx(fontname, checksum, scale, enlargement_factor);
      
      // Insert font in dictionary and make sure the dictionary is big
      // enough.
      if (tn_table.size()-2 <= tn_table.count())
	// Not quite optimal. The size of the dictionary should be a
	// prime for optimal performance. I don't care.
	tn_table.resize(tn_table.size()*2); 
      tn_table.insert(TeXnumber, fontp);
    }
    
    // Read the next command
    cmnd = readUINT8();
  }
  
  if (cmnd != POSTPOST) {
    errorMsg = i18n("The postamble contained a command other than FNTDEF.");
    return;
  }

  // Now we remove all those fonts from the memory which are no longer
  // in use.
  if (font_pool != 0)
    font_pool->release_fonts();
}


void dvifile::prepare_pages()
{
#ifdef DEBUG_DVIFILE
  kdDebug(4300) << "prepare_pages" << endl;
#endif

  if (page_offset.resize(total_pages+1) == false) {
    kdError(4300) << "No memory for page list!" << endl;
    return;
  }
  for(int i=0; i<=total_pages; i++)
    page_offset[i] = 0;
  

  page_offset[total_pages] = beginning_of_postamble;
  Q_UINT16 i               = total_pages-1;
  page_offset[i]           = last_page_offset;

  // Follow back pointers through pages in the DVI file, storing the
  // offsets in the page_offset table.
   while (i > 0) {
    command_pointer  = dvi_Data() + page_offset[i--];
    if (readUINT8() != BOP) {
      errorMsg = i18n("The page %1 does not start with the BOP command.").arg(i+1);
      return;
    }
    command_pointer += 10 * 4;
    page_offset[i] = readUINT32();
    if ((dvi_Data()+page_offset[i] < dvi_Data())||(dvi_Data()+page_offset[i] > dvi_Data()+size_of_file))
      break;
  }
}


dvifile::dvifile(const QString& fname, fontPool* pool)
{
#ifdef DEBUG_DVIFILE
  kdDebug(4300) << "init_dvi_file: " << fname << endl;
#endif

  errorMsg     = QString::null;
  errorCounter = 0;
  page_offset  = 0;
  suggestedPageSize = 0;
  numberOfExternalPSFiles = 0;
  numberOfExternalNONPSFiles = 0;
  font_pool    = pool;
  sourceSpecialMarker = true;
  
  QFile file(fname);
  filename = file.name();
  file.open( IO_ReadOnly );
  size_of_file = file.size();
  dviData.resize(size_of_file);
  // Sets the end pointer for the bigEndianByteReader so that the
  // whole memory buffer is readable
  end_pointer = dvi_Data()+size_of_file; 
  if (dvi_Data() == 0) {
    kdError() << i18n("Not enough memory to load the DVI-file.");
    return;
  }
  file.readBlock((char *)dvi_Data(), size_of_file);
  file.close();
  if (file.status() != IO_Ok) {
    kdError() << i18n("Could not load the DVI-file.");
    return;
  }

  tn_table.clear();
  
  process_preamble();
  find_postamble();
  read_postamble();
  prepare_pages();

  return;
}


dvifile::~dvifile()
{
#ifdef DEBUG_DVIFILE
  kdDebug(4300) << "destroy dvi-file" << endl;
#endif

  // Delete converted PDF files
  QMap<QString, QString>::Iterator it;
  for ( it = convertedFiles.begin(); it != convertedFiles.end(); ++it ) 
    QFile::remove(it.data());

  if (suggestedPageSize != 0)
    delete suggestedPageSize;
  if (font_pool != 0)
    font_pool->mark_fonts_as_unused();
}


void dvifile::renumber()
{
  dviData.detach();

  // Write the page number to the file, taking good care of byte
  // orderings.
  int wordSize;
  bool bigEndian;
  qSysInfo (&wordSize, &bigEndian);

  for(Q_UINT32 i=1; i<=total_pages; i++) {
    Q_UINT8 *ptr = dviData.data() + page_offset[i-1]+1;
    Q_UINT8 *num = (Q_UINT8 *)&i;
    for(Q_UINT8 j=0; j<4; j++)
      if (bigEndian) {
	*(ptr++) = num[0];
	*(ptr++) = num[1];
	*(ptr++) = num[2];
	*(ptr++) = num[3];
      } else {
	*(ptr++) = num[3];
	*(ptr++) = num[2];
	*(ptr++) = num[1];
	*(ptr++) = num[0];
      }
  }
}


QString dvifile::convertPDFtoPS(const QString &PDFFilename)
{
  // Check if the PDFFile is known
  QMap<QString, QString>::Iterator it =  convertedFiles.find(PDFFilename);
  if (it != convertedFiles.end()) {
    // PDF-File is known. Good.
    return it.data();
  }

  // Get the name of a temporary file
  KTempFile tmpfile(QString::null, ".ps");
  QString convertedFileName = tmpfile.name();
  tmpfile.close();
  tmpfile.unlink();

  // Use pdf2ps to do the conversion
  KProcIO proc;
  proc << "pdf2ps" << PDFFilename << convertedFileName;
  if (proc.start(KProcess::Block) == false) 
    convertedFileName = QString::null; // Indicates that conversion failed, won't try again.
  if (!QFile::exists(convertedFileName))
    convertedFileName = QString::null; // Indicates that conversion failed, won't try again.

  // Save name of converted file to buffer, so PDF file won't be
  // converted again, and files can be deleted when *this is
  // deconstructed.
  convertedFiles[PDFFilename] = convertedFileName;

  return convertedFileName;
}


bool dvifile::saveAs(const QString &filename)
{
  if (dvi_Data() == 0)
    return false;

  QFile out(filename);
  if (out.open( IO_Raw|IO_WriteOnly ) == false)
    return false;
  if (out.writeBlock ( (char *)(dvi_Data()), size_of_file ) == -1)
    return false;
  out.close();
  return true;
}
