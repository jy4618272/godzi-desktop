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
#include <Godzi/Application>

using namespace Godzi;

Application::Application()
{
    //nop
}

void
Application::setProject( Project* project, const std::string& projectLocation )
{
    if ( project )
    {
        _project = project;
        _projectLocation = projectLocation;
        _project->sync( _projectCheckpoint );
    }
}

bool
Application::isProjectDirty() const
{
    return !_project.valid() || _project->outOfSyncWith( _projectCheckpoint );
}

void
Application::setProjectClean()
{
    if ( _project.valid() )
        _project->sync( _projectCheckpoint );
}
