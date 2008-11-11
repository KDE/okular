/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_document_p.h"

#include <qwidget.h>

#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs/kjsarguments.h>

#include <kdebug.h>
#include <assert.h>

#include "../document_p.h"
#include "../page.h"
#include "../form.h"
#include "kjs_data_p.h"
#include "kjs_field_p.h"

using namespace Okular;

static KJSPrototype *g_docProto;

// Document.numPages
static KJSObject docGetNumPages( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSNumber( doc->m_pagesVector.count() );
}

// Document.pageNum (getter)
static KJSObject docGetPageNum( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSNumber( doc->m_parent->currentPage() );
}

// Document.pageNum (setter)
static void docSetPageNum( KJSContext* ctx, void* object,
                           KJSObject value )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    int page = value.toInt32( ctx );

    if ( page == (int)doc->m_parent->currentPage() )
        return;

    doc->m_parent->setViewportPage( page );
}

// Document.documentFileName
static KJSObject docGetDocumentFileName( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSString( doc->m_url.fileName() );
}

// Document.filesize
static KJSObject docGetFilesize( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSNumber( doc->m_docSize );
}

// Document.path
static KJSObject docGetPath( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSString( doc->m_url.pathOrUrl() );
}

// Document.URL
static KJSObject docGetURL( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    return KJSString( doc->m_url.prettyUrl() );
}

// Document.permStatusReady
static KJSObject docGetPermStatusReady( KJSContext *, void * )
{
    return KJSBoolean( true );
}

// Document.dataObjects
static KJSObject docGetDataObjects( KJSContext *ctx, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    const QList< EmbeddedFile * > *files = doc->m_generator->embeddedFiles();

    KJSArray dataObjects( ctx, files ? files->count() : 0 );
    if ( files )
    {
        QList< EmbeddedFile * >::ConstIterator it = files->begin(), itEnd = files->end();
        for ( int i = 0; it != itEnd; ++it, ++i )
        {
            KJSObject newdata = JSData::wrapFile( ctx, *it );
            dataObjects.setProperty( ctx, QString::number( i ), newdata );
        }
    }
    return dataObjects;
}

// Document.external
static KJSObject docGetExternal( KJSContext *, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );
    QWidget *widget = doc->m_parent->widget();

    const bool isShell = ( widget
                           && widget->parentWidget()
                           && widget->parentWidget()->objectName() == QLatin1String( "okular::Shell" ) );
    return KJSBoolean( !isShell );
}


static KJSObject docGetInfo( KJSContext *ctx, void *object )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    KJSObject obj;
    const DocumentInfo *docinfo = doc->m_generator->generateDocumentInfo();
    if ( docinfo )
    {
#define KEY_GET( key, property ) \
do { \
    const QString data = docinfo->get( key ); \
    if ( !data.isEmpty() ) \
    { \
        const KJSString newval( data ); \
        obj.setProperty( ctx, property, newval );                  \
        obj.setProperty( ctx, QString( property ).toLower(), newval );   \
    } \
} while ( 0 );
        KEY_GET( "title", "Title" );
        KEY_GET( "author", "Author" );
        KEY_GET( "subject", "Subject" );
        KEY_GET( "keywords", "Keywords" );
        KEY_GET( "creator", "Creator" );
        KEY_GET( "producer", "Producer" );
#undef KEY_GET
    }
    return obj;
}

#define DOCINFO_GET_METHOD( key, name ) \
static KJSObject docGet ## name( KJSContext *, void *object ) \
{ \
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object ); \
    const DocumentInfo *docinfo = doc->m_generator->generateDocumentInfo(); \
    return KJSString( docinfo->get( key ) ); \
}

DOCINFO_GET_METHOD( "author", Author )
DOCINFO_GET_METHOD( "creator", Creator )
DOCINFO_GET_METHOD( "keywords", Keywords )
DOCINFO_GET_METHOD( "producer", Producer )
DOCINFO_GET_METHOD( "title", Title )
DOCINFO_GET_METHOD( "subject", Subject )

#undef DOCINFO_GET_METHOD

// Document.getField()
static KJSObject docGetField( KJSContext *context, void *object,
                              const KJSArguments &arguments )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    QString cName = arguments.at( 0 ).toString( context );

    QVector< Page * >::const_iterator pIt = doc->m_pagesVector.constBegin(), pEnd = doc->m_pagesVector.constEnd();
    for ( ; pIt != pEnd; ++pIt )
    {
        const QLinkedList< Okular::FormField * > pageFields = (*pIt)->formFields();
        QLinkedList< Okular::FormField * >::const_iterator ffIt = pageFields.constBegin(), ffEnd = pageFields.constEnd();
        for ( ; ffIt != ffEnd; ++ffIt )
        {
            if ( (*ffIt)->name() == cName )
            {
                return JSField::wrapField( context, *ffIt, *pIt );
            }
        }
    }
    return KJSUndefined();
}

// Document.getPageLabel()
static KJSObject docGetPageLabel( KJSContext *ctx,void *object,
                                  const KJSArguments &arguments )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );
    int nPage = arguments.at( 0 ).toInt32( ctx );
    Page *p = doc->m_pagesVector.value( nPage );
    return KJSString( p ? p->label() : QString() );
}

// Document.getPageRotation()
static KJSObject docGetPageRotation( KJSContext *ctx, void *object,
                                     const KJSArguments &arguments )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );
    int nPage = arguments.at( 0 ).toInt32( ctx );
    Page *p = doc->m_pagesVector.value( nPage );
    return KJSNumber( p ? p->orientation() * 90 : 0 );
}

// Document.gotoNamedDest()
static KJSObject docGotoNamedDest( KJSContext *ctx, void *object,
                                   const KJSArguments &arguments )
{
    DocumentPrivate *doc = reinterpret_cast< DocumentPrivate* >( object );

    QString dest = arguments.at( 0 ).toString( ctx );

    DocumentViewport viewport( doc->m_generator->metaData( "NamedViewport", dest ).toString() );
    if ( !viewport.isValid() )
        return KJSUndefined();

    doc->m_parent->setViewport( viewport );

    return KJSUndefined();
}

// Document.syncAnnotScan()
static KJSObject docSyncAnnotScan( KJSContext *, void *,
                                   const KJSArguments & )
{
    return KJSUndefined();
}

void JSDocument::initType( KJSContext *ctx )
{
    assert( g_docProto );

    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    g_docProto->defineProperty( ctx, "numPages", docGetNumPages );
    g_docProto->defineProperty( ctx, "pageNum", docGetPageNum, docSetPageNum );
    g_docProto->defineProperty( ctx, "documentFileName", docGetDocumentFileName );
    g_docProto->defineProperty( ctx, "filesize", docGetFilesize );
    g_docProto->defineProperty( ctx, "path", docGetPath );
    g_docProto->defineProperty( ctx, "URL", docGetURL );
    g_docProto->defineProperty( ctx, "permStatusReady", docGetPermStatusReady );
    g_docProto->defineProperty( ctx, "dataObjects", docGetDataObjects );
    g_docProto->defineProperty( ctx, "external", docGetExternal );

    // info properties
    g_docProto->defineProperty( ctx, "info", docGetInfo );
    g_docProto->defineProperty( ctx, "author", docGetAuthor );
    g_docProto->defineProperty( ctx, "creator", docGetCreator );
    g_docProto->defineProperty( ctx, "keywords", docGetKeywords );
    g_docProto->defineProperty( ctx, "producer", docGetProducer );
    g_docProto->defineProperty( ctx, "title", docGetTitle );
    g_docProto->defineProperty( ctx, "subject", docGetSubject );

    g_docProto->defineFunction( ctx, "getField", docGetField );
    g_docProto->defineFunction( ctx, "getPageLabel", docGetPageLabel );
    g_docProto->defineFunction( ctx, "getPageRotation", docGetPageRotation );
    g_docProto->defineFunction( ctx, "gotoNamedDest", docGotoNamedDest );
    g_docProto->defineFunction( ctx, "syncAnnotScan", docSyncAnnotScan );
}

KJSGlobalObject JSDocument::wrapDocument( DocumentPrivate *doc )
{
    if ( !g_docProto )
        g_docProto = new KJSPrototype();
    return g_docProto->constructGlobalObject( doc );
}
