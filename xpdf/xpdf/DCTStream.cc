//========================================================================
//
// DCTStream.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include "DCTStream.h"

static void str_init_source(j_decompress_ptr /*cinfo*/)
{
}

static boolean str_fill_input_buffer(j_decompress_ptr cinfo)
{
  struct str_src_mgr * src = (struct str_src_mgr *)cinfo->src;
  src->buffer = src->str->getChar();
  src->pub.next_input_byte = &src->buffer;
  src->pub.bytes_in_buffer = 1;
  return TRUE;
}

static void str_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  struct str_src_mgr * src = (struct str_src_mgr *)cinfo->src;
  if (num_bytes > 0) {
    while (num_bytes > (long) src->pub.bytes_in_buffer) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      str_fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void str_term_source(j_decompress_ptr /*cinfo*/)
{
}

DCTStream::DCTStream(Stream *strA):
  FilterStream(strA) {
  
  jpeg_create_decompress(&cinfo);
  src.pub.init_source = str_init_source;
  src.pub.fill_input_buffer = str_fill_input_buffer;
  src.pub.skip_input_data = str_skip_input_data;
  src.pub.resync_to_restart = jpeg_resync_to_restart;
  src.pub.term_source = str_term_source;
  src.pub.bytes_in_buffer = 0;
  src.pub.next_input_byte = NULL;
  src.str = str;
  cinfo.src = (jpeg_source_mgr *)&src;
  cinfo.err = jpeg_std_error(&jerr);
  x = 0;
}

DCTStream::~DCTStream() {
  jpeg_destroy_decompress(&cinfo);
  delete str;
}

void DCTStream::reset() {
  int row_stride;

  str->reset();
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * cinfo.output_components;
  row_buffer = cinfo.mem->alloc_sarray((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
}

int DCTStream::getChar() {
  int c;

  if (x == 0) {
    if (cinfo.output_scanline < cinfo.output_height)
      jpeg_read_scanlines(&cinfo, row_buffer, 1);
    else return EOF;
  }
  c = row_buffer[0][x];
  x++;
  if (x == cinfo.output_width * cinfo.output_components)
    x = 0;
  return c;
}

int DCTStream::lookChar() {
  int c;
  c = row_buffer[0][x];
  return c;
}

GString *DCTStream::getPSFilter(int psLevel, char *indent) {
  GString *s;

  if (psLevel < 2) {
    return NULL;
  }
  if (!(s = str->getPSFilter(psLevel, indent))) {
    return NULL;
  }
  s->append(indent)->append("<< >> /DCTDecode filter\n");
  return s;
}

GBool DCTStream::isBinary(GBool /*last*/) {
  return str->isBinary(gTrue);
}
