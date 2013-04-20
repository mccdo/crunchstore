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
#include "mongo/client/dbclient.h"

namespace crunchstore
{

class CRUNCHSTORE_EXPORT MongoStore : public Store
{
public:
    MongoStore();
    ~MongoStore();

    virtual void SetStorePath( const std::string& path );

    /**
     * Checks whether table with @c typeName exists in the current database.
     * @param typeName Name of table to check for
     * @return @c true if table exists, @c false otherwise.
     */
    virutal bool HasTypeName( const std::string& typeName );

    virtual void Attach();

    /**
      * Attempts clean shutdown of database connections and deletes the
      * working db file.
      */
    virtual void Detach();

    virtual void Remove( Persistable& persistable, Role role = DEFAULT_ROLE,
                         TransactionKey& transactionKey = TransactionKey() );

    virtual bool HasIDForTypename( const boost::uuids::uuid& id, const std::string& typeName );

    virtual void GetIDsForTypename( const std::string& typeName,
                                    std::vector< std::string >& resultIDs,
                                    Role role = DEFAULT_ROLE );

    virtual void Search( const std::string& typeName,
                         std::vector< SearchCriterion >& criteria,
                         const std::string& returnField,
                         std::vector< std::string >& results );

    virtual void ProcessBackgroundTasks();

    //virtual void SetChild( DataAbstractionLayerPtr child );

    virtual void Drop( const std::string& typeName, Role role = DEFAULT_ROLE  );

    void MapReduce( const std::string& typeName,
                    const std::string& jsMapFunction,
                    const std::string& jsReduceFunction,
                    mongo::BSONObj queryObj,
                    const std::string& outputUUID,
                    const std::string& outputcollection = ""  );

protected:
    virtual void SaveImpl( const Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           TransactionKey& transactionKey = TransactionKey() );

    virtual void LoadImpl( Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           TransactionKey& transactionKey = TransactionKey() );

private:

    unsigned int GetBoostAnyVectorSize( const boost::any& value );

    /// Holds the current db path
    std::string m_path;

    mongo::DBClientConnection* m_connection;
};

} // namespace crunchstore
