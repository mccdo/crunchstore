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
//#define CRUNCHSTORE_DEBUG
#include <crunchstore/SQLiteStore.h>
#include <crunchstore/Datum.h>
#include <crunchstore/Persistable.h>
#include <crunchstore/BindableAnyWrapper.h>
#include <crunchstore/SearchCriterion.h>
#include <crunchstore/Exception.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/concept_check.hpp>

#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Data/DataException.h>
#include <Poco/Thread.h>
DIAG_OFF(unused-parameter)
#include <Poco/Version.h>
#if POCO_VERSION > 0x01050000
    #include <Poco/Data/LOB.h>
    #define POCO_KEYWORD_NAMESPACE Poco::Data::Keywords::
#else
    #include <Poco/Data/BLOB.h>
    #define POCO_KEYWORD_NAMESPACE Poco::Data::
#endif
DIAG_ON(unused-parameter)

#if defined(__GNUC__)
    #include <unistd.h>
#else
    #if defined(_MSC_VER)
        #include <windows.h>
    #endif
#endif

#include <stdexcept>
#include <iostream>

#define DB_LOCK_TIME 2000

#define CS_SQRETRY_PRE for (int i = 0; i < 6; i++) { try {
        // Your repeatable statement is inserted here.

#define CS_SQRETRY_POST break; }\
    catch ( Poco::Exception& e ) {\
    CRUNCHSTORE_LOG_DEBUG( "Retry logic: " << e.displayText() << ", i=" << i );\
    if (i < 5) DoSleep(); else throw; } \
    catch ( ... ) { CRUNCHSTORE_LOG_ERROR( "Unknown error in retry logic." ); } \
    }

namespace crunchstore
{

////////////////////////////////////////////////////////////////////////////////
StmtObj::StmtObj(
    Poco::Data::Session const& session )
    :
    m_session( session ),
    m_statement( m_session ),
    m_statementImpl( m_session.createStatementImpl() )
{
    Poco::Data::Statement stmt( m_statementImpl );
    m_statement.swap( stmt );
}
////////////////////////////////////////////////////////////////////////////////
StmtObj::~StmtObj()
{
    ;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SQLiteStore::SQLiteStore():
            m_pool(0),
            m_logger( Poco::Logger::get("Crunchstore::SQLiteStore") )
{
    m_logStream = LogStreamPtr( new Poco::LogStream( m_logger ) );
    CRUNCHSTORE_LOG_TRACE( "ctor" );
}
////////////////////////////////////////////////////////////////////////////////
SQLiteStore::~SQLiteStore()
{
    Detach();
}
////////////////////////////////////////////////////////////////////////////////
Poco::Data::SessionPool* SQLiteStore::GetPool()
{
    CRUNCHSTORE_LOG_TRACE( "GetPool" );
    return m_pool;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::SetStorePath( const std::string& path )
{
    CRUNCHSTORE_LOG_TRACE( "SetStorePath: " << path );
    //std::cout << "SQLiteStore::SetStorePath: path = "
    //          << path << std::endl << std::flush;
    m_path = path;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Attach()
{
    CRUNCHSTORE_LOG_TRACE( "Attach" );
    if( m_pool )
    {
        delete m_pool;
        m_pool = 0;
        Poco::Data::SQLite::Connector::unregisterConnector();
    }

    Poco::Data::SQLite::Connector::registerConnector();
    try
    {
        m_pool = new Poco::Data::SessionPool( "SQLite", m_path, 1, 32, 10 );
    }
    catch( Poco::Exception& e )
    {
        CRUNCHSTORE_LOG_ERROR( "Attach: " << e.displayText() );
    }
}
////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::HasTypeName( const std::string& typeName )
{
    CRUNCHSTORE_LOG_TRACE( "HasTypeName: " << typeName );
    if( !m_pool )
    {
        return false;
    }

    Poco::Data::Session session( GetSession() );
    SetupDBProperties( session );

    return _tableExists( session, typeName );
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Detach()
{
    CRUNCHSTORE_LOG_TRACE( "Detach" );
    //With poco 1.5.x unregisterConnector should not be called multiple times
    //for a single connection.
    if( !m_pool )
    {
        return;
    }

    //std::cout << "Number of idle Poco::Sessions " << m_pool->idle()
    //    << " Number of dead Poco::Sessions " << m_pool->dead() << std::endl;
    //This must be deleted from the thread that it was created from
    delete m_pool;
    m_pool = 0;

    try
    {
        Poco::Data::SQLite::Connector::unregisterConnector();
    }
    catch( ... )
    {
        ;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::SaveImpl( const Persistable& persistable,
                   Role, const TransactionKey& transactionKey )
{
    CRUNCHSTORE_LOG_TRACE( "SaveImpl" );
    if( !m_pool )
    {
        return;
    }

    Poco::Mutex::ScopedLock lock( dbLock );
    // Need to have explicitly named variables if we want to be able to bind
    // with Poco::Data
    std::string tableName = persistable.GetTypeName();
    std::string uuidString = persistable.GetUUIDAsString();

    if( tableName.empty() )
    {
        return;
    }

    try
    {
        bool transactionInProgress;
        Poco::Data::Session session( GetSessionByKey( transactionKey, transactionInProgress ) );
        SetupDBProperties( session );

            if( !transactionInProgress )
            {
                // If we're not inside of a larger transaction, make this save
                // operation its own single transaction.
                //CS_SQRETRY_PRE
                session.begin();
                //CS_SQRETRY_POST
            }
            UpdatePersistable( persistable, session, tableName, uuidString );
            UpdatePersistableVector( persistable, session, tableName, uuidString );
            if( !transactionInProgress )
            {
                // Commit the transaction only if we opened one just above.
                session.commit();
            }
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl DataException : " << e.displayText() );
        throw;
    }
    catch( std::runtime_error &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl runtime_error : " << e.what() );
        throw;
    }
    catch( ... )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl : Unspecified error when writing to database." );
        throw;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::LoadImpl( Persistable& persistable, Role,
                            const TransactionKey& transactionKey )
{
    CRUNCHSTORE_LOG_TRACE( "LoadImpl" );

    if( !m_pool )
    {
        return;
    }

    bool transactionInProgress;
    Poco::Data::Session session = GetSessionByKey( transactionKey, transactionInProgress );
    SetupDBProperties( session );

    // Need to have explicitly named variables if we want to be able to bind
    // with Poco::Data
    std::string tableName = persistable.GetTypeName();
    std::string uuidString = persistable.GetUUIDAsString();

    // Check for existence of TableName in db and fail if false.
    if( !_tableExists(session, tableName) )
    {
        return;// false;
    }

    // Get the entire record we need with one query
    StmtObj statement( session );
    //Poco::Data::Statement statement( session );
    statement.m_statement << "SELECT * FROM \"" << tableName << "\" WHERE uuid=:0", POCO_KEYWORD_NAMESPACE use( uuidString );
    try
    {
        //CS_SQRETRY_PRE
        //statement.execute();
        //CS_SQRETRY_POST
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::LoadImpl: " << e.displayText() );
    }

    Poco::Data::RecordSet recordset( statement.m_statement );

    if( recordset.rowCount() == 0 )
    {
        return;// false;
    }

    // Step through the property list and look for matching column names. If
    // there's a match, load the data into the correct property. It's important
    // that this operation be done in the order of properties in mPropertyList
    // since that implicitly sets the load/execution order of properties.
    std::vector< std::string > mPropertyList = persistable.GetDataList();
    for( size_t s = 0; s < mPropertyList.size(); ++s )
    {
        std::string name = mPropertyList.at( s );
        size_t index = 0;
        bool found = false;
        while( !found && index < recordset.columnCount() )
        {
            if( recordset.columnName( index ) == name )
            {
                found = true;
                break;
            }
            ++index;
        }

        // If column name corresponds to a property name in this set, extract
        // the value from the column and set the property value
        if( found )
        {
            boost::any bValue;
            Poco::DynamicAny value = recordset[index];
            Poco::Data::MetaColumn::ColumnDataType dataType = recordset.columnType( index );
            std::string columnName = recordset.columnName( index );
            if( !value.isEmpty() )
            {
                switch( dataType )
                {
                case Poco::Data::MetaColumn::FDT_BOOL: // Never gets used by SQLite
                    bValue = value.convert<bool>();
                    break;
                case Poco::Data::MetaColumn::FDT_INT8:
                    bValue = value.convert<int>();
                    break;
                case Poco::Data::MetaColumn::FDT_INT16:
                    bValue = value.convert<int>();
                    break;
                case Poco::Data::MetaColumn::FDT_INT32: // Bools appear to also be int32
                    if( persistable.GetDatum( columnName )->IsBool() )//mPropertyMap[columnName]->IsBool() )
                    {
                        bValue = value.convert<bool>();
                    }
                    else
                    {
                        bValue = value.convert<int>();
                    }
                    break;
                case Poco::Data::MetaColumn::FDT_INT64:
                    bValue = value.convert<int>();
                    break;
                case Poco::Data::MetaColumn::FDT_FLOAT:
                    bValue = value.convert<float>();
                    break;
                case Poco::Data::MetaColumn::FDT_DOUBLE:
                    bValue = value.convert<double>();
                    break;
                case Poco::Data::MetaColumn::FDT_STRING:
                    bValue = value.convert<std::string>();
                    break;
                case Poco::Data::MetaColumn::FDT_BLOB:
                {
                    std::string tmp( value.convert<std::string>() );
                    std::vector< char > charVersion( tmp.begin(), tmp.end() );
                    bValue = charVersion;
                    break;
                }
                default:
                    //std::cout << "Didn't find conversion type" << std::endl << std::flush;
                    break;
                }
            }

            if( !bValue.empty() )
            {
                //std::cout << "SQLiteStore::LoadImpl: Setting value for " << name << std::endl << std::flush;
                persistable.SetDatumValue( name, bValue );
            }
        }
    }

    //std::cout << "SQLiteStore::LoadImpl: vec part" << std::endl << std::flush;

    // Look through PropertySet for vectorized data types. These will not have been
    // stored in the main table and must be looked for elsewhere in the database.
    DatumPtr property;
    std::vector< std::string > dataList = persistable.GetDataList();
    std::vector< std::string >::iterator it = dataList.begin();
    while( it != dataList.end() )
    {
        property = persistable.GetDatum( *it );
        if( property->IsVectorized() )
        {
            std::string mUUIDString = persistable.GetUUIDAsString();
            std::string fieldName = *it;
            //Poco::Data::Statement statement( session );
            StmtObj statement( session );
            statement.m_statement << "SELECT " << fieldName << " FROM \"" << persistable.GetTypeName()
                    << "_" << *it << "\" WHERE PropertySetParentID=:0"
                    , POCO_KEYWORD_NAMESPACE use( mUUIDString );
            try
            {
                //CS_SQRETRY_PRE
                //statement.execute();
                //CS_SQRETRY_POST
                ExecuteRetry( statement );
            }
            catch( Poco::Data::DataException &e )
            {
                CRUNCHSTORE_LOG_ERROR( "SQLiteStore::LoadImpl_vectorized: " << e.displayText() );
            }
            Poco::Data::RecordSet recordset( statement.m_statement );
            //std::cout << fieldName << " " << mTableName << " " << iterator->first << std::endl;
            if( property->IsIntVector() )
            {
                std::vector< int > vec;
                Poco::DynamicAny value;
                size_t rowCount = recordset.rowCount();
                for( size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex )
                {
                    value = recordset.value( 0, rowIndex );
                    vec.push_back( value.convert< int >() );
                }
                property->SetValue( vec );
            }
            else if( property->IsFloatVector() )
            {
                std::vector< float > vec;
                Poco::DynamicAny value;
                size_t rowCount = recordset.rowCount();
                for( size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex )
                {
                    value = recordset.value( 0, rowIndex );
                    vec.push_back( value.convert< float >() );
                }
                property->SetValue( vec );
            }
            else if( property->IsDoubleVector() )
            {
                std::vector< double > vec;
                Poco::DynamicAny value;
                size_t rowCount = recordset.rowCount();
                for( size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex )
                {
                    value = recordset.value( 0, rowIndex );
                    vec.push_back( value.convert< double >() );
                }
                property->SetValue( vec );
            }
            else if( property->IsStringVector() )
            {
                std::vector< std::string > vec;
                Poco::DynamicAny value;
                size_t rowCount = recordset.rowCount();
                for( size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex )
                {
                    value = recordset.value( 0, rowIndex );
                    vec.push_back( value.convert< std::string > () );
                }
                property->SetValue( vec );
            }
        }
        ++it;
    }

    // If we have just loaded a dataset, the change accumulator will be full
    // of changes and it will appear as though the set is dirty and needs to
    // be written back to the database. To prevent an unnecessary write, we
    // set this false:
// TODO: deal with this in persistable interface
//    m_writeDirty = false;
//    m_liveWriteDirty = false;

    //return true;

}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Remove( Persistable& persistable, Role,
                          const TransactionKey& transactionKey )
{
    CRUNCHSTORE_LOG_TRACE( "Remove" );
    if( !m_pool )
    {
        return;
    }
    Poco::Mutex::ScopedLock lock( dbLock );
    std::string typeName = persistable.GetTypeName();
    if( HasIDForTypename( persistable.GetUUID(), persistable.GetTypeName() ) )
    {
        std::string idString = persistable.GetUUIDAsString();
        bool transactionInProgress;
        Poco::Data::Session session = GetSessionByKey( transactionKey, transactionInProgress );
        SetupDBProperties( session );

        //Poco::Data::Statement statement( session );
        StmtObj statement( session );

        statement.m_statement << "DELETE FROM \"" << typeName << "\" WHERE uuid=:uuid",
                POCO_KEYWORD_NAMESPACE use( idString );
                //POCO_KEYWORD_NAMESPACE now;

        try
        {
            //if( dbLock.tryLock( DB_LOCK_TIME ) )
            {
                //CRUNCHSTORE_LOG_TRACE( "Remove Lock" );
//                CS_SQRETRY_PRE
//                session << "DELETE FROM \"" << typeName << "\" WHERE uuid=:uuid",
//                        POCO_KEYWORD_NAMESPACE use( idString ),
//                        POCO_KEYWORD_NAMESPACE now;
//                CS_SQRETRY_POST
                ExecuteRetry( statement );
                //dbLock.unlock();
                //CRUNCHSTORE_LOG_TRACE( "Remove Unlock" );
            }

        }
        catch( Poco::Data::DataException &e )
        {
            CRUNCHSTORE_LOG_ERROR( "SQLiteStore::Remove: " << e.displayText() );
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::HasIDForTypename( const boost::uuids::uuid& id,
                                    const std::string& typeName,
                                    Role )
{
    CRUNCHSTORE_LOG_TRACE( "HasIDForTypename" );
    if( !m_pool )
    {
        return false;
    }

    Poco::Data::Session session( GetSession() );
    SetupDBProperties( session );

    if( !_tableExists( session, typeName ) )
    {
        //std::cout << "Don't even have typename " << typeName << std::endl << std::flush;
        return false;
    }

    // This query will return a non-empty string iff the record exists
    int idTest;
    std::string idString = boost::lexical_cast< std::string >( id );

    StmtObj statement( session );
    statement.m_statement << "SELECT COUNT (*) FROM \"" << typeName << "\" WHERE uuid=?",
            POCO_KEYWORD_NAMESPACE into( idTest ),
            POCO_KEYWORD_NAMESPACE use( idString );
//            POCO_KEYWORD_NAMESPACE now;

    try
    {
//        CS_SQRETRY_PRE
//        session << "SELECT uuid FROM \"" << typeName << "\" WHERE uuid=:uuid",
//            POCO_KEYWORD_NAMESPACE into( idTest ),
//            POCO_KEYWORD_NAMESPACE use( idString ),
//            POCO_KEYWORD_NAMESPACE now;
//        CS_SQRETRY_POST
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::HasIDForTypename: " << e.displayText() );
    }


    if( idTest == 0 )
    {
        std::cout << "SQLiteStore::HasIDForTypename: The uuid "
            << idString << " for this " << typeName << " may not be valid." << std::endl;
        return false;
    }
    else
    {
        return true;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::GetIDsForTypename( const std::string& typeName,
                                     std::vector< std::string >& resultIDs,
                                     Role )
{
    CRUNCHSTORE_LOG_TRACE( "GetIDsForTypename" );
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( GetSession() );
    SetupDBProperties( session );

    StmtObj statement( session );

    statement.m_statement << "SELECT uuid FROM " << typeName, POCO_KEYWORD_NAMESPACE into( resultIDs );
    try
    {
//        CS_SQRETRY_PRE
//        statement.execute();
//        CS_SQRETRY_POST
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::GetIDsForTypename: " << e.displayText() );
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Search( const std::string& typeName,
                          std::vector< SearchCriterion >& criteria,
                          const std::string& returnField,
                          std::vector< std::string >& results )
{
    CRUNCHSTORE_LOG_TRACE( "Search" );
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( GetSession() );
    SetupDBProperties( session );

    if( !_tableExists( session, typeName ) )
    {
        //std::cout << "SQLiteStore::Search: Error: No table named " << typeName << std::endl << std::flush;
        return;
    }

    // For now, we only treat a single keyvalue criterion. More advanced processing
    // can be added later.
    std::string wherePredicate;
    std::string bindings;
    SearchCriterion sc;
    /*if( !criteria.empty() )
    {
        sc = criteria.at(0);
        wherePredicate = sc.m_key;
        wherePredicate += " ";
        wherePredicate += sc.m_comparison;
    }*/
    unsigned int monotonic = 0;
    for( size_t index = 0; index < criteria.size(); ++index )
    {
        sc = criteria.at(index);
        // Build up the wherePredicate
        if( sc.m_isOperatorCriterion )
        {
            // Put in any AND, OR, LIKE, BETWEEN operators
            switch( sc.m_operator )
            {
                case SearchCriterion::AND:
                {
                    wherePredicate += " AND ";
                    break;
                }
                case SearchCriterion::OR:
                {
                    wherePredicate += " OR ";
                    break;
                }
                case SearchCriterion::LIKE:
                {
                    wherePredicate += " LIKE ";
                    break;
                }
                case SearchCriterion::BETWEEN:
                {
                    wherePredicate += " BETWEEN ";
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
            // Put in a [key] [comparison] [value] clause
            wherePredicate += sc.m_key + " " + sc.m_comparison + " ";
            wherePredicate += ":" + boost::lexical_cast<std::string >( monotonic );
            ++monotonic;
        }
    }

    //Poco::Data::Statement statement( session );
    StmtObj statement( session );
    statement.m_statement << "SELECT " << returnField << " FROM \"" << typeName << "\"";
    if( !wherePredicate.empty() )
    {
        statement.m_statement << " WHERE " << wherePredicate;
    }
    statement.m_statement, POCO_KEYWORD_NAMESPACE into( results );

    std::vector< BindableAnyWrapper* > bindableVector;
    BindableAnyWrapper* bindable;
    if( !wherePredicate.empty() )
    {
        for( size_t index = 0; index < criteria.size(); ++index )
        {
            sc = criteria.at(index);
            if( !sc.m_isOperatorCriterion )
            {
                bindable = new BindableAnyWrapper;
                bindableVector.push_back( bindable );
                bindable->BindValue( &statement.m_statement, sc.m_value );
            }
        }
    }

    try
    {
//        CS_SQRETRY_PRE
//        statement.execute();
//        CS_SQRETRY_POST
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::Search: " << e.displayText() );
    }

    std::vector< BindableAnyWrapper* >::iterator biterator =
                bindableVector.begin();
    while( biterator != bindableVector.end() )
    {
        delete ( *biterator );
        ++biterator;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::ProcessBackgroundTasks()
{
    // No bg tasks for SQLite yet.
}

////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::_tableExists( Poco::Data::Session& session, std::string const& TableName )
{
    CRUNCHSTORE_LOG_TRACE( "_tableExists" );
    bool tableExists = false;

    //Poco::Data::Statement statement( session );
    StmtObj statement( session );
    statement.m_statement << "SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name",
            POCO_KEYWORD_NAMESPACE into( tableExists ),
#if POCO_VERSION > 0x01050000
            POCO_KEYWORD_NAMESPACE useRef( TableName );
#else
            POCO_KEYWORD_NAMESPACE use( TableName );
#endif
    // "SELECT 1 ... will put a 1 (true) into the boolean value if the tablename
    // is found in the database.
    try
    {

//            CS_SQRETRY_PRE
//            session << "SELECT 1 FROM sqlite_master WHERE type='table' AND name=:name",
//                POCO_KEYWORD_NAMESPACE into( tableExists ),
//                POCO_KEYWORD_NAMESPACE useRef( TableName ),
//                POCO_KEYWORD_NAMESPACE now;
//            CS_SQRETRY_POST
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::_tableExists: " << e.displayText() );
    }

    return tableExists;
}
////////////////////////////////////////////////////////////////////////////////
std::string SQLiteStore::_buildColumnHeaderString( const Persistable& persistable )
{
    CRUNCHSTORE_LOG_TRACE( "_buildColumnHeaderString" );
    std::string result;

    // Forcing the primary key to autoincrement ensures that we can always
    // find the most recently inserted entry simply by issuing
    // SELECT MAX(id) from table_name
    result.append( "id INTEGER PRIMARY KEY AUTOINCREMENT, uuid TEXT," );

    DatumPtr property;
    std::vector< std::string > dataList = persistable.GetDataList();
    std::vector< std::string >::iterator it = dataList.begin();
    while( it != dataList.end() )
    {
        property = persistable.GetDatum( *it );
        std::string dataType;

        // Figure out what to put in for the colum data type
        if( property->IsBool() )
        {
            dataType = "INTEGER";
        }
        else if( property->IsInt() )
        {
            dataType = "INTEGER";
        }
        else if( property->IsFloat() )
        {
            dataType = "FLOAT";
        }
        else if( property->IsDouble() )
        {
            dataType = "DOUBLE";
        }
        else if( property->IsString() )
        {
            dataType = "TEXT";
        }
        else if( property->IsBLOB() )
        {
            dataType = "BLOB";
        }
        else
        {
            dataType = "UNKNOWN";
        }

        // One more test... if we have an empty boost::any, don't write it.
        if( (dataType != "UNKNOWN") && (property->GetValue().empty()) )
        {
            CRUNCHSTORE_LOG_ERROR( "Empty any looked like: " << dataType );
            dataType = "UNKNOWN";
        }

        if( _containsIllegalCharacter( *it ) )
        {
            // This will cause the property to be skipped in db writes:
            dataType = "UNKNOWN";

            CRUNCHSTORE_LOG_ERROR( "Error: Property " << (*it) << " contains a"
                    << " disallowed character. Allowed characters are digits"
                    << " 0-9, letters, and underscore. This property will not"
                    << " be written to the database." );
        }
        // Put the property's name string in as the column name
        // If the property is of an unknown type, we skip it for now
        // and there will be no db column for it.
        if( dataType != "UNKNOWN" )
        {
            result.append( *it );
            result.append( " " );
            result.append( dataType );
            result.append( "," );

            ++it;
        }
        else
        {
            //std::cout << "Datum named " << (*it) << " will not be written" << std::endl << std::flush;
            ++it;
        }
    }

    // There May be an extra comma at the end of the result that must be
    // removed. Test for it and remove if it is there.
    if( result.substr( result.size() - 1, result.size() ) == "," )
    {
        result.erase( --result.end() );
    }

    //std::cout << result << std::endl << std::flush;

    return result;
}
////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::_containsIllegalCharacter( std::string const& value )
{
    size_t position = value.find_first_not_of(
            "1234567890_aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ" );
    CRUNCHSTORE_LOG_TRACE( "_containsIllegalCharacter " << position << " " << value );
    if( position != value.npos )
    {
        return true;
    }
    else
    {
        return false;
    }
}
////////////////////////////////////////////////////////////////////////////////
size_t SQLiteStore::GetBoostAnyVectorSize( const boost::any& value )
{
    CRUNCHSTORE_LOG_TRACE( "GetBoostAnyVectorSize" );
    size_t size = 0;
    Datum temp( 0 );
    if( temp.IsIntVector( value ) )
    {
        size = boost::any_cast< std::vector<int> >( value ).size();
    }
    else if( temp.IsFloatVector( value ) )
    {
        size = boost::any_cast< std::vector<float> >( value ).size();
    }
    else if( temp.IsDoubleVector( value ) )
    {
        size = boost::any_cast< std::vector<double> >( value ).size();
    }
    else if( temp.IsStringVector( value ) )
    {
        size = boost::any_cast< std::vector<std::string> >( value ).size();
    }

    return size;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Drop( const std::string& typeName, Role )
{
    CRUNCHSTORE_LOG_TRACE( "Drop" );
    //std::cout << "SQLiteStore::Drop" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }


    Poco::Data::Session session( GetSession() );
    CRUNCHSTORE_LOG_TRACE( "Drop: session acquired" );
    SetupDBProperties( session );

    //Poco::Data::Statement statement( session );
    StmtObj statement( session );
    statement.m_statement << "DROP TABLE " << typeName;

    if( _tableExists( session, typeName ) )
    {
        try
        {
//            CS_SQRETRY_PRE
//            session << "DROP TABLE " << typeName, POCO_KEYWORD_NAMESPACE now;
//            CS_SQRETRY_POST
            ExecuteRetry( statement );
        }
        catch( Poco::Exception &e )
        {
            CRUNCHSTORE_LOG_ERROR( "Drop: " << e.displayText() );
            throw;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
SQLiteTransactionKey SQLiteStore::BeginTransaction()
{
    CRUNCHSTORE_LOG_INFO( "Opening bulk mode on SQLiteStore" );
    std::cout <<  "Opening bulk mode on SQLiteStore" << std::endl << std::flush;

    Poco::Data::Session session( GetSession() );
    session.begin();
    SQLiteTransactionKey key( session );
    return key;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::EndTransaction( TransactionKey& transactionKey )
{
    CRUNCHSTORE_LOG_INFO( "Closing bulk mode on SQLiteStore" );

    if( SQLiteTransactionKey* d = dynamic_cast< SQLiteTransactionKey* >( &transactionKey ) )
    {
        Poco::Data::Session session( d->GetSession() );
        session.commit();
        session.close();
    }
}
////////////////////////////////////////////////////////////////////////////////
Poco::Data::Session SQLiteStore::GetSessionByKey( const TransactionKey& transactionKey, bool& transactionInProgress )
{
    CRUNCHSTORE_LOG_TRACE( "GetSessionByKey" );
    if( transactionKey.GetTypeString() == "SQLite" )
    {
        SQLiteTransactionKey key = static_cast< const SQLiteTransactionKey& >( transactionKey );
        if( key.IsSet() )
        {
            //std::cout << "Returning the saved session..." << std::flush;
            transactionInProgress = true;
            return key.GetSession();
        }
        else
        {
            //std::cout << "Key not set..." << std::flush;
            transactionInProgress = false;
            return GetSession();
        }
    }
    else
    {
        //std::cout << "Wrong key type..." << transactionKey.GetTypeString() << std::flush;
        transactionInProgress = false;
        return GetSession();
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::SetupDBProperties( Poco::Data::Session& session )
{
#if POCO_VERSION < 0x01050000
    try
    {
        session.setProperty( "maxRetryAttempts", 4 );
        session.setProperty( "transactionMode", std::string("IMMEDIATE") );
        session.setProperty( "maxRetrySleep", 100 );
        session.setProperty( "minRetrySleep", 50 );
    }
    catch( Poco::Data::DataException& ex )
    {
        CRUNCHSTORE_LOG_INFO( ex.displayText() );
    }
#elif POCO_VERSION > 0x01040900
    try
    {
        //session.setProperty( "connectionTimeout", 100 );
        session.setConnectionTimeout( 100 );
    }
    catch( Poco::Data::DataException& ex )
    {
        CRUNCHSTORE_LOG_INFO( ex.displayText() );
    }
#else
    boost::ignore_unused_variable_warning( session );
#endif
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::DoSleep()
{
    CRUNCHSTORE_LOG_TRACE( "DoSleep" );
#if defined(__GNUC__)
    usleep( 100000 );
#else
    #if defined(_MSC_VER)
        Sleep( 100 );
    #endif
#endif
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::UpdatePersistable( const Persistable& persistable, Poco::Data::Session& session,
                                        std::string& tableName, std::string& uuidString )
{
    // These two don't get used until about 150 lines down, but need to be
    // declared outside the try{} block so memory can be properly cleaned
    // up in case of an exception during writing to database
    // Stores bindable wrappers for later deletion
    std::vector< BindableAnyWrapper* > bindableVector;
    
    try
    {
        // See if a table for this type already exists; if not create the table

        if( !_tableExists( session, tableName ) ) // Table doesn't exist
        {
            std::string columnHeaderString = _buildColumnHeaderString( persistable );
            //Poco::Data::Statement sm( session );
            StmtObj statement( session );
            statement.m_statement << "CREATE TABLE \"" << tableName << "\" (" << columnHeaderString << ")";
            ExecuteRetry( statement );
        }

        // Determine whether a record already exists for this PropertySet.
        // This query will return a non-zero, positive id if the record exists
        //int idTest = 0;
        int idTest;
        
        {
            StmtObj statement( session );
            statement.m_statement << "SELECT COUNT (*) FROM \"" << tableName << "\" WHERE uuid=?",
                   POCO_KEYWORD_NAMESPACE into( idTest ),
                   POCO_KEYWORD_NAMESPACE use( uuidString );
            try
            {
                ExecuteRetry( statement );
            }
            catch( Poco::Data::ExtractException const& e )
            {
                //if we have already extracted this data then the statement is already present
                //idTest = "1";
            }
        }

        // Since the data binding part will be the same for INSERT and UPDATE
        // operations on this Persistable, we only need to build the string part
        // of the query separately.
        
        // Will hold the string part of any query we do.
        std::string query;
        // Will hold the list of fields in the order added to query.
        std::vector< std::string > fieldNames;
        
        //        if( idTest == 0 ) //  Record does not exist; perform an INSERT
        if( idTest == 0 )
        {
            // Build a query that looks like this:
            // "INSERT INTO tablename (field1name_1,fieldname_2,...) VALUES (:1,:2,...)"
            query = "INSERT INTO \"";
            query.append( tableName );
            query.append( "\" (uuid," );
            
            DatumPtr property;
            std::vector< std::string > dataList = persistable.GetDataList();
            std::vector< std::string >::const_iterator it = dataList.begin();
            while( it != dataList.end() )
            {
                property = persistable.GetDatum( *it );
                // Check for a known type
                if( ( property->IsBool() ) || ( property->IsDouble() ) ||
                   ( property->IsFloat() ) || ( property->IsInt() ) ||
                   ( property->IsString() ) || property->IsBLOB() )
                {
                    // Skip the property if its name contains illegal characters
                    if( !_containsIllegalCharacter( *it ) )
                    {
                        query.append( *it );
                        query.append( "," );
                        fieldNames.push_back( *it );
                    }
                    ++it;
                }
                else // Didn't put in field name because we had an unknown type
                {
                    ++it;
                }
            }
            
            // There should be an extra comma at the end of the query that must be
            // removed. Test for it and remove if it is there.
            if( query.substr( query.size() - 1, query.size() ) == "," )
            {
                query.erase( --query.end() );
            }
            
            query.append( ") VALUES (" );
            // Put in uuid explicitly since it appears only in INSERT and not
            // in UPDATE queries
            query.append( "\"");
            query.append( uuidString );
            query.append( "\",");
            
            // Put in the binding labels (:0,:1,...) for Poco::Data
            size_t max = fieldNames.size();
            for( size_t count = 0; count < max; count++ )
            {
                query.append( ":" );
                query.append( boost::lexical_cast<std::string > ( count ) );
                query.append( "," );
            }
            // There should be an extra comma at the end of the query that must be
            // removed. Test for it and remove if it is there.
            if( query.substr( query.size() - 1, query.size() ) == "," )
            {
                query.erase( --query.end() );
            }
            
            query.append( ")" );
        }
        else // Record exists; perform an UPDATE
        {
            // Build a query that looks like:
            // "UPDATE tablename SET field_0=:0, field_1=:1 WHERE id=mID"
            query = "UPDATE \"";
            query.append( tableName );
            query.append( "\" SET " );
            
            DatumPtr property;
            std::vector< std::string > dataList = persistable.GetDataList();
            std::vector< std::string >::const_iterator it = dataList.begin();
            while( it != dataList.end() )
            {
                property = persistable.GetDatum( *it );
                // Check for a known type
                if( ( property->IsBool() ) || ( property->IsDouble() ) ||
                   ( property->IsFloat() ) || ( property->IsInt() ) ||
                   ( property->IsString() ) || ( property->IsBLOB() ) )
                {
                    // Skip the property if its name contains illegal characters
                    if( !_containsIllegalCharacter( *it ) )
                    {
                        query.append( *it );
                        query.append( "=:" );
                        query.append( boost::lexical_cast<std::string > ( fieldNames.size() ) );
                        query.append( "," );
                        
                        fieldNames.push_back( *it );
                    }
                    ++it;
                }
                else // Didn't put in field name because we had an unknown type
                {
                    ++it;
                }
            }
            // There should be an extra comma at the end of the query that must be
            // removed. Test for it and remove if it is there.
            if( query.substr( query.size() - 1, query.size() ) == "," )
            {
                query.erase( --query.end() );
            }
            
            query.append( " WHERE uuid=\"" );
            query.append( uuidString );
            query.append("\"");
        }
        
        // Turn the query into a statement that can accept bound values
        StmtObj statement( session );
        statement.m_statement << query;
        
        // The data binding looks the same for either query type (INSERT or UPDATE)
        BindableAnyWrapper* bindable; // Bindable wrapper for property data
        DatumPtr property;
        std::vector<std::string>::iterator it = fieldNames.begin();
        while( it != fieldNames.end() )
        {
            const std::string currentFieldName = ( *it );
            property = persistable.GetDatum( currentFieldName );
            bindable = new BindableAnyWrapper;
            bindableVector.push_back( bindable );
            bindable->BindValue( &statement.m_statement, property->GetValue() );
            ++it;
        }
        
        
        //std::cout <<statement.m_statementImpl->getState() << " " << statement.m_statement.toString() << std::endl << std::flush;
        ExecuteRetry( statement );
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl 2: " << e.displayText() );
        //throw;
    }
    catch( std::runtime_error &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl 2: " << e.what() );
        //throw;
    }
    catch( ... )
    {
        CRUNCHSTORE_LOG_ERROR( "SaveImpl 2: Unspecified error when writing to database." );
        //throw;
    }
    
    // Delete the BindableAnyWrapperS that were created in the binding loop
    std::vector< BindableAnyWrapper* >::iterator biterator =
    bindableVector.begin();
    while( biterator != bindableVector.end() )
    {
        delete ( *biterator );
        ++biterator;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::UpdatePersistableVector( const Persistable& persistable, Poco::Data::Session& session,
                                              std::string& tableName, std::string& uuidString )
{
    // After the top-level entries have been written, we write out any vectorized
    // quantities which must go in their own sub-table(s)
    std::vector< BindableAnyWrapper* > bindVector;
    
    try
    {
        // Open a db transaction. This allows multiple INSERTs and UPDATEs to
        // happen very quickly. Failure to use a transaction in this instance
        // will cause lists to take roughly .25 seconds *per item*. With a transaction,
        // 10,000 items can be inserted or updated in ~1 second.

            DatumPtr property;
            std::vector< std::string > dataList = persistable.GetDataList();
            std::vector< std::string >::const_iterator it = dataList.begin();
            while( it != dataList.end() )
            {
                property = persistable.GetDatum( *it );
                if( property->IsVectorized() )
                {
                    // Vectors (lists) are written into separate tables named
                    // ParentTable_[ThisPropertyName]. Prefixing with the parent table
                    // name ensures that property names need not be unique across all property
                    // sets. For example, PropertySetA might have a property called
                    // 'Directions' that is a list of strings, and PropertySetB might have a property
                    // set called 'Directions' that is a list of integers. If the child table is
                    // simply called "Directions", there could be foreign key overlap in the child table,
                    // as well as problems with type mis-match. Prefixing with the parent table's
                    // name prevents such issues.
                    
                    enum LISTTYPE
                    {
                        UNKNOWN, INTEGER, FLOAT, DOUBLE, STRING
                    };
                    LISTTYPE listType = UNKNOWN;
                    std::string columnType( "" );
                    
                    // Determine the list type and set the column type in case the table
                    // must be created.
                    if( property->IsIntVector() )
                    {
                        listType = INTEGER;
                        columnType = "INTEGER";
                    }
                    else if( property->IsFloatVector() )
                    {
                        listType = FLOAT;
                        columnType = "FLOAT";
                    }
                    else if( property->IsDoubleVector() )
                    {
                        listType = DOUBLE;
                        columnType = "DOUBLE";
                    }
                    else if( property->IsStringVector() )
                    {
                        listType = STRING;
                        columnType = "TEXT";
                    }
                    
                    if( listType != UNKNOWN )
                    {
                        // New (sub)table gets the name
                        // [ParentTableName]_[currentFieldName]
                        std::string newTableName( tableName );
                        newTableName += "_";
                        std::string fieldName = *it;
                        newTableName += fieldName;
                        
                        // Check for existing table; if table doesn't exist, create it.
                        if( !_tableExists( session, newTableName ) )
                        {
//                            CS_SQRETRY_PRE
//                            session << "CREATE TABLE \"" << newTableName <<
//                            "\" (id INTEGER PRIMARY KEY,PropertySetParentID TEXT,"
//                            << fieldName << " " << columnType << ")", POCO_KEYWORD_NAMESPACE now;
//                            CS_SQRETRY_POST
                            //Poco::Data::Statement statement( session );
                            StmtObj statement( session );
                            statement.m_statement << "CREATE TABLE \"" << newTableName <<
                                         "\" (id INTEGER PRIMARY KEY,PropertySetParentID TEXT,"
                                         << fieldName << " " << columnType << ")";
                            ExecuteRetry( statement );
                        }
                        
                        // First part of query will delete everything from the sub-table
                        // that this propertyset owns. We do this because it is
                        // much easier than checking whether the data exists
                        // and attempting to do an update. In the case of a list like this,
                        // we'd not only have to see if the list already exists in the table,
                        // but whether it's the same length. Then we'd have to either delete
                        // rows from the table or insert rows to make the lengths match
                        // before doing an update. Far easier to wipe out what's there
                        // and start fresh. It's probably a little slower for large
                        // lists to do it this way. If lists become a performance
                        // problem, this is one place to look for possible speedups.
                        // "DELETE FROM [newtablename] WHERE PropertySetParentID=[id]"
                        std::string listQuery;
                        listQuery += "DELETE FROM \"";
                        listQuery += newTableName;
                        listQuery += "\" WHERE ";
                        listQuery += "PropertySetParentID=";
                        //listQuery += boost::lexical_cast<std::string > ( mID );
                        listQuery += "\"";
                        listQuery += uuidString;
                        listQuery += "\"";
                        
//                        CS_SQRETRY_PRE
//                        session << listQuery, POCO_KEYWORD_NAMESPACE now;
//                        CS_SQRETRY_POST
                        //Poco::Data::Statement statement( session );
                        StmtObj statement( session );
                        statement.m_statement << listQuery;
                        ExecuteRetry( statement );
                        listQuery.clear();
                        
                        BindableAnyWrapper* bindable;
                        size_t max = GetBoostAnyVectorSize( property->GetValue() );
                        for( size_t index = 0; index < max; ++index )
                        {
                            // Build up query:
                            // INSERT INTO [newTableName]
                            // ([fieldName],PropertySetParentID) VALUES (:num,[mID])
                            listQuery += "INSERT INTO \"";
                            listQuery += newTableName;
                            listQuery += "\" (";
                            listQuery += fieldName;
                            listQuery += ",PropertySetParentID) VALUES (:";
                            listQuery += boost::lexical_cast<std::string > ( index );
                            listQuery += ",";
                            //listQuery += boost::lexical_cast<std::string > ( mID );
                            listQuery += "\"";
                            listQuery += uuidString;
                            listQuery += "\"";
                            listQuery += ")";
                            
                            // Turn into a prepared statement that can accept bindings
                            //Poco::Data::Statement listStatement( session );
                            StmtObj listStatement( session );
                            listStatement.m_statement << listQuery;
                            listQuery.clear();
                            
                            // Extract data from vector for binding into query
                            boost::any currentValue;
                            switch( listType )
                            {
                                case INTEGER:
                                {
                                    std::vector<int> vec = property->extract< std::vector< int > >();
                                    currentValue = vec.at( index );
                                    break;
                                }
                                case FLOAT:
                                {
                                    std::vector<float> vec = property->extract< std::vector< float > >();
                                    currentValue = vec.at( index );
                                    break;
                                }
                                case DOUBLE:
                                {
                                    std::vector<double> vec = property->extract< std::vector< double > >();
                                    currentValue = vec.at( index );
                                    break;
                                }
                                case STRING:
                                {
                                    std::vector<std::string> vec = property->extract< std::vector< std::string > >();
                                    currentValue = vec.at( index );
                                    break;
                                }
                                case UNKNOWN:
                                {
                                    break;
                                }
                            }
                            
                            // Bind the data and execute the statement if no binding errors
                            bindable = new BindableAnyWrapper;
                            bindVector.push_back( bindable );
                            if( !bindable->BindValue( &listStatement.m_statement, currentValue ) )
                            {
                                CRUNCHSTORE_LOG_ERROR( "Error in binding data" );
                            }
                            else
                            {
//                                CS_SQRETRY_PRE
//                                listStatement.execute();
//                                CS_SQRETRY_POST
                                ExecuteRetry( listStatement );
                            }
                        }
                    }
                }
                ++it;
            }
    }
    catch( Poco::Data::DataException &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::SaveImpl 3: " << e.displayText() );
        throw;
    }
    catch( std::runtime_error &e )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::SaveImpl 3: " << e.what() );
        throw;
    }
    catch( ... )
    {
        CRUNCHSTORE_LOG_ERROR( "SQLiteStore::SaveImpl 3: Unspecified error when writing to database." );
        throw;
    }

    {
        // Braces protect scoping of biterator
        std::vector< BindableAnyWrapper* >::iterator biterator =
        bindVector.begin();
        while( biterator != bindVector.end() )
        {
            delete ( *biterator );
            ++biterator;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::ExecuteRetry( StmtObj& stmtObj,
                                unsigned int const& maxRetryAttempts,
                                unsigned int const& retrySleep )
{
    Poco::Data::Statement& stmt = stmtObj.m_statement;
#if POCO_VERSION > 0x01050000
    if( stmt.isAsync() )
    {
        throw Exception( "SQLiteStore::ExecuteRetry error",
            "Attempted to use unsupported Async mode." );
    }
#endif 
    Poco::Data::StatementImpl& impl = *( stmtObj.m_statementImpl );
    unsigned int cnt( 0 );
    while( ++cnt <= maxRetryAttempts )
    {
        try
        {
            if( cnt == 1 )
            {
                //The first time we try to execute the statement. Every time
                //after this if the first time failed we just need to reset
                //and execute the implmentation. If we reset the statement
                //then we will cause a recompile and a rebind of all of the
                //variables.
                //To find more details about this method look at
                //Poco::Data::Statement.cpp file in the execute method.
                stmt.execute();
            }
            else
            {
                //Just reset the statement implementation so that we do not
                //recompile the complete statement.
                impl.reset();
#if POCO_VERSION > 0x01050000
                impl.execute( true );
#else
                impl.execute();
#endif
            }
            return;
        }
        catch( Poco::Data::SQLite::DBLockedException const& )
        {
            ;
        }
        catch( Poco::Data::SQLite::TableLockedException const& )
        {
            ;
        }
        catch( Poco::Exception const& ex )
        {
            ex.rethrow();
        }
        Poco::Thread::sleep( retrySleep );
    }
    
    throw Exception( "ExecuteRetry failed", "SQLiteStore::ExecuteRetry failed" );
}
////////////////////////////////////////////////////////////////////////////////
Poco::Data::Session SQLiteStore::GetSession(
                   unsigned int const& maxRetryAttempts,
                   unsigned int const& retrySleep )
{
    unsigned int cnt( 0 );
    while( ++cnt <= maxRetryAttempts )
    {
        try
        {
            return m_pool->get();
        }
        catch( Poco::Data::ExecutionException const& )
        {
            ;
        }
        catch( Poco::Exception const& ex )
        {
            ex.rethrow();
        }
        Poco::Thread::sleep( retrySleep );
    }
    
    throw Exception( "No session returned",
                     "SQLiteStore::GetSession failed to return a session" );
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
