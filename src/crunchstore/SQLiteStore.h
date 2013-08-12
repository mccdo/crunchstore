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
#include <crunchstore/Logging.h>
#include <crunchstore/TransactionKey.h>
#include <crunchstore/SQLiteTransactionKey.h>

#include <Poco/Data/SQLite/SQLite.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SessionPool.h>
#include <Poco/Mutex.h>
#include <Poco/Data/Statement.h>

#include <boost/noncopyable.hpp>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT StmtObj : public boost::noncopyable
{
public:
    ///
    StmtObj(
        Poco::Data::Session const& session );
    ///
    ~StmtObj();
    ///
    Poco::Data::Session m_session;
    ///
    Poco::Data::Statement m_statement;
    ///
    Poco::Data::StatementImpl* m_statementImpl;
protected:
    ///
    StmtObj();
};


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

    virtual void Remove( Persistable& persistable, Role role = DEFAULT_ROLE,
                         const TransactionKey& = TransactionKey() );

    virtual bool HasIDForTypename( const boost::uuids::uuid& id,
                                   const std::string& typeName,
                                   Role role = DEFAULT_ROLE );

    virtual void GetIDsForTypename( const std::string& typeName,
                                    std::vector< std::string >& resultIDs,
                                    Role role = DEFAULT_ROLE );

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

    SQLiteTransactionKey BeginTransaction();

    virtual void EndTransaction( TransactionKey& transactionKey );

protected:
    virtual void SaveImpl( const Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           const TransactionKey& transactionKey = TransactionKey() );

    virtual void LoadImpl( Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           const TransactionKey& transactionKey = TransactionKey() );

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

    Poco::Data::Session GetSessionByKey( const TransactionKey& transactionKey, bool& transactionInProgress );

    ///Setup the properties on the sqlite db
    ///\param session The db to set the properties on
    void SetupDBProperties( Poco::Data::Session& session );

    /// Sleep for 100 ms
    void DoSleep();
    
    ///Update method
    void UpdatePersistable( const Persistable& persistable, Poco::Data::Session& session,
                           std::string& tableName, std::string& uuidString );
    ///Update method
    void UpdatePersistableVector( const Persistable& persistable, Poco::Data::Session& session,
                                 std::string& tableName, std::string& uuidString );

    ///Retries a query if DataException or InvalidAccessException is thrown
    void ExecuteRetry( StmtObj& stmtObj,
                       unsigned int const& maxRetryAttempts = 5,
                       unsigned int const& retrySleep = 100 );

    ///Get a session
    Poco::Data::Session GetSession( unsigned int const& maxRetryAttempts = 100,
                                   unsigned int const& retrySleep = 30 );

    /// Holds the session pool
    Poco::Data::SessionPool* m_pool;

    /// Holds the current db path
    std::string m_path;

    /// Signal registered as "DatabaseManager.ResyncFromDatabase" that is
    /// emitted during calls to LoadFrom.
    //boost::signals2::signal< void() > m_resyncFromDatabase;

    Poco::Mutex dbLock;

    Poco::Logger& m_logger;
    LogStreamPtr m_logStream;
};

} // namespace crunchstore
