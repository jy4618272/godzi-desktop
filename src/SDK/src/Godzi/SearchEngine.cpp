/* --*-c++-*-- */
/**
 * Godzi
 * Copyright 2010 Pelican Mapping
 * http://pelicanmapping.com
 * http://github.com/gwaldron/godzi
 *
 * Godzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 */
#include <Godzi/SearchEngine>
#include <Godzi/Application>

using namespace Godzi;

//---------------------------------------------------------------------------

SearchResult::SearchResult( const PlaceList& places ) :
_places( places )
{
    //nop
}

//---------------------------------------------------------------------------

SearchEngine::SearchEngine()
{
    //nop
}

SearchResults
SearchEngine::search( const SearchQuery& query )
{
    //TODO: try the cache first
    SearchResults results = doSearch( query );
    //TODO: cache results
    return results;
}

//---------------------------------------------------------------------------

SearchAction::SearchAction( const SearchQuery& query ) :
_query( query )
{
    //nop
}

bool
SearchAction::doAction( const ActionContext& ac, Application* app )
{
    SearchEngine* engine = app->getSearchEngine();
    if ( engine )
    {
        _results = engine->search( _query );
    }
    return true;
}
