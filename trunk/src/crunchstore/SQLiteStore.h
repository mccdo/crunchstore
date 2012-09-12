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

#include <crunchstore/Store.h>
#include <crunchstore/SQLiteStorePtr.h>
#include <Poco/Data/SQLite/SQLite.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SessionPool.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT SQLiteStore : public Store
{
public:
    SQLiteStore();
    ~SQLiteStore();

    void SetStorePath( const std::string& path );

    /**
     * Checks whether table with @c typeName exists in the current database.
     * @param typeName Name of table to check for
     * @return @c true if table exists, @c false otherwise.
     */
    bool HasTypeName( const std::string& typeName );

    virtual void Attach();

    /**
      * Attempts clean shutdown of database connections and deletes the
      * working db file.
      */
    virtual void Detach();

    virtual void Remove( Persistable& persistable, Role role = DEFAULT_ROLE );

    virtual bool HasIDForTypename( const boost::uuids::uuid& id, const std::string& typeName );

    virtual void GetIDsForTypename( const std::string& typeName,
                                    std::vector< std::string >& resultIDs );

    virtual void Search( const std::string& typeName,
                         std::vector< SearchCriterion >& criteria,
                         const std::string& returnField,
                         std::vector< std::string >& results );

    virtual void ProcessBackgroundTasks();

    virtual void Drop( const std::string& typeName, Role role = DEFAULT_ROLE  );

    //virtual void SetChild( DataAbstractionLayerPtr child );

    ///
    /// Returns a pointer to the session pool. Callers can get a valid session
    /// like so:
    /// @code
    /// Poco::Data::Session mySession( GetPool()->get() );
    /// @endcode
    /// The session created in this way will be automatically returned to the
    /// SessionPool when mySession goes out of scope.
    Poco::Data::SessionPool* GetPool();

protected:
    virtual void SaveImpl( const Persistable& persistable,
                           Role role = DEFAULT_ROLE  );

    virtual void LoadImpl( Persistable& persistable,
                           Role role = DEFAULT_ROLE );

private:

    ///
    /// Internal function that looks through the property set and builds an
    /// appropriate string for creating an sqlite table for storing the data
    /// contained in this property set. If the default function is not doing
    /// what you need, override this function to create a custom table.
    std::string _buildColumnHeaderString( const Persistable& persistable );

    ///
    /// Tests for presence of characters disallowed in database column names in
    /// string value. For sqlite, allowed characters are digits 0-9, lower- and
    /// upper-case letters, and the underscore. All other characters are illegal.
    bool _containsIllegalCharacter( const std::string& value );

    ///
    /// Helper function to determine whether a given TableName exists in the db.
    bool _tableExists( Poco::Data::Session& session, const std::string& TableName );

    size_t GetBoostAnyVectorSize( const boost::any& value );

    /// Holds the session pool
    Poco::Data::SessionPool* m_pool;

    /// Holds the current db path
    std::string m_path;

    /// Signal registered as "DatabaseManager.ResyncFromDatabase" that is
    /// emitted during calls to LoadFrom.
    //boost::signals2::signal< void() > m_resyncFromDatabase;
};

} // namespace crunchstore
