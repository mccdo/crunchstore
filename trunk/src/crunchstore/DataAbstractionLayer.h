/*************** <auto-copyright.rb BEGIN do not edit this line> **************
 *
 * Copyright 2012-2012 by Ames Laboratory
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *************** <auto-copyright.rb END do not edit this line> ***************/
#pragma once

#include <crunchstore/DataAbstractionLayerPtr.h>
#include <crunchstore/Persistable.h>
#include <crunchstore/SearchCriterion.h>

#include <crunchstore/ExportConfig.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT DataAbstractionLayer
{
public:
    DataAbstractionLayer();
    virtual ~DataAbstractionLayer();

    /// Persistables can be used in four roles. This is a binary flag, so
    /// DEFAULT_ROLE = WORKING_ROLE + BACKING_ROLE
    enum Role{ WORKING_ROLE = 1,
               BACKING_ROLE = 2,
               DEFAULT_ROLE = 3,
               VERSIONING_ROLE = 4 };

    /// Saves persistable to data store. Unless a version branch is specifically
    /// being tagged, role should be left to the default.
    virtual void Save( const Persistable& persistable, Role role = DEFAULT_ROLE );

    /// Loads persistable from data store.
    virtual void Load( Persistable& persistable, Role role = DEFAULT_ROLE );

    /// Removes persistable from data store.
    virtual void Remove( Persistable& persistable, Role role = DEFAULT_ROLE );

    /// Does this DAL object have a datum with this ID?
    virtual bool HasIDForTypename( const boost::uuids::uuid& id,
                                   const std::string& typeName,
                                   Role role = DEFAULT_ROLE  );

    /// Gets all available IDs for a given type.
    virtual void GetIDsForTypename( const std::string& typeName,
                                    std::vector< std::string >& resultIDs );

    /// Gets the value of @c returnField for a given type where criteria are
    /// met. If @c criteria is an empty vector< SearchCriterion >, the value
    /// for @c returnField of every entry of @c typeName is returned.
    virtual void Search( const std::string& typeName,
                         std::vector< SearchCriterion >& criteria,
                         const std::string& returnField,
                         std::vector< std::string >& results );


    /// Gives the DAL a chance to do background tasks such as buffering data,
    /// performing heuristic analysis, building indices, etc.
    virtual void ProcessBackgroundTasks();

    /// Sets the store-side child of this DAL. Calls to all other methods in
    /// this interface should be forwarded to the store-side child after any
    /// local processing occurs.
    virtual void SetChild( DataAbstractionLayerPtr child );

    /// Delete all records with typename typeName in all stores of with role Role
    virtual void Drop( const std::string& typeName, Role role = DEFAULT_ROLE  );

protected:
    /// Holds the store-side child of this DAL
    DataAbstractionLayerPtr m_child;
};

} // namespace crunchstore
