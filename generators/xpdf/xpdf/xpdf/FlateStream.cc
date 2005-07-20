//========================================================================
//
// FlateStream.cc
//
// Copyright (C) 2005, Jeff Muizelaar
//
//========================================================================
#include "FlateStream.h"
FlateStream::FlateStream(Stream *strA, int predictor, int columns, int colors, int bits) :
  FilterStream(strA)
{
  if (predictor != 1) {
    pred = new StreamPredictor(this, predictor, columns, colors, bits);
  } else {
    pred = NULL;
  }
  out_pos = 0;
  memset(&d_stream, 0, sizeof(d_stream));
}

FlateStream::~FlateStream() {
  inflateEnd(&d_stream);
  delete str;
}

void FlateStream::reset() {
  //FIXME: what are the semantics of reset?
  //i.e. how much intialization has to happen in the constructor?
  str->reset();
  memset(&d_stream, 0, sizeof(d_stream));
  inflateInit(&d_stream);
  d_stream.avail_in = 0;
  status = Z_OK;
  out_pos = 0;
  out_buf_len = 0;
}

int FlateStream::getRawChar() {
  if (fill_buffer())
    return EOF;

  return out_buf[out_pos++];
}

int FlateStream::getChar() {
  if (pred)
    return pred->getChar();
  else
    return getRawChar();
}

int FlateStream::lookChar() {
  if (pred)
    return pred->lookChar();

  if (fill_buffer())
    return EOF;

  return out_buf[out_pos];
}

int FlateStream::fill_buffer() {
  if (out_pos >= out_buf_len) {
    if (status == Z_STREAM_END) {
      return -1;
    }
    d_stream.avail_out = sizeof(out_buf);
    d_stream.next_out = out_buf;
    out_pos = 0;
    /* buffer is empty so we need to fill it */
    if (d_stream.avail_in == 0) {
      int c;
      /* read from the source stream */
      while (d_stream.avail_in < sizeof(in_buf) && (c = str->getChar()) != EOF) {
	in_buf[d_stream.avail_in++] = c;
      }
      d_stream.next_in = in_buf;
    }
    while (d_stream.avail_out && d_stream.avail_in && (status == Z_OK || status == Z_BUF_ERROR)) {
      status = inflate(&d_stream, Z_SYNC_FLUSH);
    }
    out_buf_len = sizeof(out_buf) - d_stream.avail_out;
    if (status != Z_OK && status != Z_STREAM_END)
      return -1;
    if (!out_buf_len)
      return -1;
  }

  return 0;
}

GString *FlateStream::getPSFilter(int psLevel, const char *indent) {
  GString *s;

  if (psLevel < 3 || pred) {
    return NULL;
  }
  if (!(s = str->getPSFilter(psLevel, indent))) {
    return NULL;
  }
  s->append(indent)->append("<< >> /FlateDecode filter\n");
  return s;
}

GBool FlateStream::isBinary(GBool /*last*/) {
  return str->isBinary(gTrue);
}
