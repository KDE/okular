/***************************************************************************
 *   Copyright (C) 2004-2007 by Georgy Yunaev, gyunaev@ulduzsoft.com       *
 *   Please do not use email address above for bug reports; see            *
 *   the README file                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <qregexp.h>

#include "libchmfile.h"
#include "libchmfileimpl.h"

//#define DEBUG_SEARCH(A)	qDebug A
#define DEBUG_SEARCH(A)


static inline void validateWord ( QString & word, bool & query_valid )
{
	QRegExp rxvalid ("[^\\d\\w_\\.]+");
	
	QString orig = word;
	word.remove ( rxvalid );
		
	if ( word != orig )
		query_valid = false;
}

static inline void validateWords ( QStringList & wordlist, bool & query_valid )
{
	QRegExp rxvalid ("[^\\d\\w_\\.]+");
	
	for ( int i = 0; i < wordlist.size(); i++ )
		validateWord ( wordlist[i], query_valid );
}


inline static void mergeResults ( LCHMSearchProgressResults & results, const LCHMSearchProgressResults & src, bool add )
{
	if ( results.empty() && add )
	{
		results = src;
		return;
	}
	
	for ( int s1 = 0; s1 < results.size(); s1++ )
	{
		bool found = false;
	
		for ( int s2 = 0; s2 < src.size(); s2++ )
		{
			if ( results[s1].urloff == src[s2].urloff )
			{
				found = true;
				break;
			}
		}

		// If we're adding, we only add the items found (i.e. any item, which is not found, is removed.
		// But if we're removing, we only remove the items found.
		if ( (found && !add) || (!found && add) )
		{
			results.erase ( results.begin() + s1 );
			s1--;
		}
	}
}


static inline void findNextWords ( QVector<uint64_t> & src, const QVector<uint64_t> & needle )
{
	for ( int s1 = 0; s1 < src.size(); s1++ )
	{
		bool found = false;
		uint64_t target_offset = src[s1] + 1;
		
		DEBUG_SEARCH (("Offset loop: offset at %u is %u, target %u", (unsigned int) s1,
					   (unsigned int) src[s1], (unsigned int) target_offset));
		
		// Search in the offsets list in attempt to find next word
		for ( int s2 = 0; s2 < needle.size(); s2++ )
		{
			if ( needle[s2] == target_offset )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			// Remove this offset, we don't need it anymore
			DEBUG_SEARCH (("Offset loop failed: offset %u not found", (unsigned int) target_offset));
			src.erase ( src.begin() + s1 );
			s1--;
		}
		else
		{
			DEBUG_SEARCH (("Offset loop succeed: offset %u found", (unsigned int) target_offset));
			src[s1]++;
		}
	}
}


inline bool searchPhrase( LCHMFileImpl * impl, const QStringList & phrase, LCHMSearchProgressResults & results )
{
	// Accumulate the phrase data here.
	LCHMSearchProgressResults phrasekeeper;

	// On the first word, just fill the phrasekeeper with every occupence of the first word
	DEBUG_SEARCH (("Search word(0): '%s'", phrase[0].ascii()));
	if ( !impl->searchWord ( phrase[0], true, false, phrasekeeper, true ) )
		return false; // the word not found, so the whole phrase is not found either.

	for ( int i = 1; i < phrase.size(); i++ )
	{
		LCHMSearchProgressResults srchtmp;

		DEBUG_SEARCH (("Search word(%d): '%s'", i, phrase[i].ascii()));
		if ( !impl->searchWord ( phrase[i], true, false, srchtmp, true ) )
			return false; // the ith word not found, so the whole phrase is not found either.

		// Iterate the both arrays, and remove every word in phrasekeeper, which is not found
		// in the srchtmp, or is found on a different position.
		for ( int p1 = 0; p1 < phrasekeeper.size(); p1++ )
		{
			bool found = false;
			
			DEBUG_SEARCH (("Ext loop (it %d): urloff %d", p1, phrasekeeper[p1].urloff));
			
			for ( int p2 = 0; p2 < srchtmp.size(); p2++ )
			{
				// look up for words on the same page
				if ( srchtmp[p2].urloff != phrasekeeper[p1].urloff )
					continue;
				
				// Now check every offset to find the one which is 1 bigger than the 
				findNextWords ( phrasekeeper[p1].offsets, srchtmp[p2].offsets );
				
				// If at least one next word is found, we leave the block intact, otherwise remove it.
				if ( !phrasekeeper[p1].offsets.empty() )
					found = true;
			}
			
			if ( !found )
			{
				DEBUG_SEARCH (("Ext loop: this word not found on %d, remove it", phrasekeeper[p1].urloff));
				phrasekeeper.erase ( phrasekeeper.begin() + p1 );
				p1--;
			}
		}
	}

	for ( int o = 0; o < phrasekeeper.size(); o++ )
		results.push_back ( LCHMSearchProgressResult (phrasekeeper[o].titleoff, phrasekeeper[o].urloff) );
			
	return !results.empty();
}



bool LCHMFile::searchQuery( const QString& inquery, QStringList * searchresults, unsigned int limit )
{
	QStringList words_must_exist, words_must_not_exist, words_highlight;
	QVector<QStringList> phrases_must_exist;
	QString query = inquery;
	bool query_valid = true;
	LCHMSearchProgressResults results;
	int pos;
	int i;	
		
	/*
	* Parse the search query with a simple state machine.
	* Query should consist of one of more words separated by a space with a possible prefix.
	 * A prefix may be:
	*   +   indicates that the word is required; any page without this word is excluded from the result.
	*   -   indicates that the word is required to be absent; any page with this word is excluded from
	*       the result.
	*   "." indicates a phrase. Anything between quotes indicates a phrase, which is set of space-separated
	*       words. Will be in result only if the words in phrase are in page in the same sequence, and
	*       follow each other.
	*   If there is no prefix, the word considered as required.
	*/
	
	QRegExp rxphrase( "\"(.*)\"" );
	QRegExp rxword( "([^\\s]+)" );
	rxphrase.setMinimal( TRUE );

	// First, get the phrase queries
	while ( (pos = rxphrase.indexIn (query, 0)) != -1 )
	{
		// A phrase query found. Locate its boundaries, and parse it.
		QStringList plist = rxphrase.cap ( 1 ).split ( QRegExp ("\\s+") );
		
		validateWords ( plist, query_valid );
		
		if ( plist.size() > 0 )
			phrases_must_exist.push_back( plist );
		
		query.remove (pos, rxphrase.matchedLength());
	}

	// Then, parse the rest query
	while ( (pos = rxword.indexIn( query, 0 )) != -1 )
	{
		// A phrase query found. Locate its boundaries, and parse it.
		QString word = rxword.cap ( 1 );
		QChar type = '+';
		
		if ( word[0] == '-' || word[0] == '+' )
		{
			type = word[0];
			word.remove (0, 1);
		}
		
		validateWord ( word, query_valid );
				
		if ( type == '-' )
			words_must_not_exist.push_back ( word );
		else
			words_must_exist.push_back ( word );
		
		query.remove (pos, rxword.matchedLength());
	}

#if defined (DUMP_SEARCH_QUERY)
	// Dump the search query
	QString qdump;
	for ( i = 0; i < phrases_must_exist.size(); i++ )
		qdump += QString(" \"") + phrases_must_exist[i].join (" ") + QString ("\"");

	for ( i = 0; i < words_must_not_exist.size(); i++ )
		qdump += QString (" -") + words_must_not_exist[i];
	
	for ( i = 0; i < words_must_exist.size(); i++ )
		qdump += QString (" +") + words_must_exist[i];

	qDebug ("Search query dump: %s", qdump.ascii());
#endif

	// First search for phrases
	if ( phrases_must_exist.size() > 0 )
	{
		LCHMSearchProgressResults tempres;
		
		for ( i = 0; i < phrases_must_exist.size(); i++ )
		{
			if ( !searchPhrase ( impl(), phrases_must_exist[i], tempres ) )
				return false;
			
			mergeResults ( results, tempres, true );
		}
	}

	for ( i = 0; i < words_must_exist.size(); i++ )
	{
		LCHMSearchProgressResults tempres;
		
		if ( !m_impl->searchWord ( words_must_exist[i], true, false, tempres, false ) )
			return false;

		mergeResults ( results, tempres, true );
	}

	for ( i = 0; i < words_must_not_exist.size(); i++ )
	{
		LCHMSearchProgressResults tempres;
		
		m_impl->searchWord ( words_must_not_exist[i], true, false, tempres, false );
		mergeResults ( results, tempres, false );
	}

	m_impl->getSearchResults( results, searchresults, limit );
	return true;
}
