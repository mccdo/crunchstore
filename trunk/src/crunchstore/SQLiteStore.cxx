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
#include <crunchstore/SQLiteStore.h>
#include <crunchstore/Datum.h>
#include <crunchstore/Persistable.h>
#include <crunchstore/BindableAnyWrapper.h>
#include <crunchstore/SearchCriterion.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>


#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/BLOB.h>

#include <iostream>

namespace crunchstore
{

SQLiteStore::SQLiteStore():
            m_pool(0)
{
}
////////////////////////////////////////////////////////////////////////////////
SQLiteStore::~SQLiteStore()
{
    //std::cout << "$$$$$ ~SQLiteStore" << std::endl << std::flush;
    Detach();
}
////////////////////////////////////////////////////////////////////////////////
Poco::Data::SessionPool* SQLiteStore::GetPool()
{
    return m_pool;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::SetStorePath( const std::string& path )
{
    //std::cout << "SQLiteStore::SetStorePath: path = "
    //          << path << std::endl << std::flush;
    m_path = path;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Attach()
{
    //std::cout << "SQLiteStore::Attach: using path "
    //          << m_path << std::endl << std::flush;
    if( m_pool )
    {
        delete m_pool;
        m_pool = 0;
        Poco::Data::SQLite::Connector::unregisterConnector();
    }

    Poco::Data::SQLite::Connector::registerConnector();
    m_pool = new Poco::Data::SessionPool( "SQLite", m_path, 1, 32, 10 );
}
////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::HasTypeName( const std::string& typeName )
{
    //std::cout << "SQLiteStore::HasTypeName" << std::endl << std::flush;
    if( !m_pool )
    {
        return false;
    }

    bool exists = false;
    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );
    try
    {
        session << "SELECT 1 FROM sqlite_master WHERE name='" << typeName << "'",
                Poco::Data::into( exists ),
                Poco::Data::now;
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::HasTypename: " << e.displayText() << std::endl;
        exists = false;
    }
    return exists;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Detach()
{
    //std::cout << "SQLiteStore::Detach" << std::endl << std::flush;
    if( m_pool )
    {
        //std::cout << "Number of idle Poco::Sessions " << m_pool->idle()
        //    << " Number of dead Poco::Sessions " << m_pool->dead() << std::endl;
        //This must be deleted from the thread that it was created from
        delete m_pool;
        m_pool = 0;
    }
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
                   Role )
{
    //std::cout << "SQLiteStore::SaveImpl" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );
    Poco::Data::Statement statement( session );

    // Need to have explicitly named variables if we want to be able to bind
    // with Poco::Data
    std::string tableName = persistable.GetTypeName();
    std::string uuidString = persistable.GetUUIDAsString();

//    bool returnVal = false;

    if( tableName.empty() )
    {
        return;// false;
    }

    // These two don't get used until about 150 lines down, but need to befcatc
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
            Poco::Data::Statement sm( session );
            sm << "CREATE TABLE \"" << tableName << "\" (" << columnHeaderString << ")";
            sm.execute();
        }

        // Determine whether a record already exists for this PropertySet.
        // This query will return a non-zero, positive id iff the record exists
        //int idTest = 0;
        std::string idTest;

        session << "SELECT uuid FROM \"" << tableName << "\" WHERE uuid=:uuid",
                Poco::Data::into( idTest ),
                Poco::Data::use( uuidString ),
                Poco::Data::now;

        // Since the data binding part will be the same for INSERT and UPDATE
        // operations on this Persistable, we only need to build the string part
        // of the query separately.

        // Will hold the string part of any query we do.
        std::string query;
        // Will hold the list of fields in the order added to query.
        std::vector< std::string > fieldNames;

//        if( idTest == 0 ) //  Record does not exist; perform an INSERT
        if( idTest.empty() )
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

            //PropertyPtr property;
            //PropertyMap::const_iterator iterator = mPropertyMap.begin();
            DatumPtr property;
            std::vector< std::string > dataList = persistable.GetDataList();
            std::vector< std::string >::const_iterator it = dataList.begin();
            while( it != dataList.end() )
            {
                property = persistable.GetDatum( *it );
                // Check for a known type
                if( ( property->IsBool() ) || ( property->IsDouble() ) ||
                        ( property->IsFloat() ) || ( property->IsInt() ) ||
                        ( property->IsString() ) )
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

//            query.append( " WHERE id=" );
//            query.append( boost::lexical_cast<std::string > ( mID ) );
            query.append( " WHERE uuid=\"" );
            query.append( uuidString );
            query.append("\"");
            //std::cout << query << std::endl << std::flush;
        }

        // Turn the query into a statement that can accept bound values
        //Poco::Data::Statement statement( session );
        statement << query;

        // The data binding looks the same for either query type (INSERT or UPDATE)
        BindableAnyWrapper* bindable; // Bindable wrapper for property data
        DatumPtr property;
        std::vector<std::string>::iterator it = fieldNames.begin();
        while( it != fieldNames.end() )
        {
            std::string currentFieldName = ( *it );
            //property = mPropertyMap[ currentFieldName ];
            property = persistable.GetDatum( currentFieldName );

            bindable = new BindableAnyWrapper;
            bindableVector.push_back( bindable );
  // !!! FIXME: This won't work since datum doesn't have attributes!
            // Force enums to save their associated string value
            /*if( ( property->IsEnum() ) )
            {
                bindable->BindValue( &statement, property->GetAttribute( "enumCurrentString" ) );
            }
            else
            {*/
                bindable->BindValue( &statement, property->GetValue() );
            //}

            ++it;
        }

        //std::cout << statement.toString() << std::endl;

        statement.execute();
        // If we've made it here, we successfully wrote to database
        //returnVal = true;
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::SaveImpl: " << e.displayText() << std::endl;
    }
    catch( ... )
    {
        std::cout << "SQLiteStore::SaveImpl: Unspecified error when writing to database." << std::endl;
    }

    // Delete the BindableAnyWrapperS that were created in the binding loop
    std::vector< BindableAnyWrapper* >::iterator biterator =
            bindableVector.begin();
    while( biterator != bindableVector.end() )
    {
        delete ( *biterator );
        ++biterator;
    }


    // After the top-level entries have been written, we write out any vectorized
    // quantities which must go in their own sub-table(s)
    std::vector< BindableAnyWrapper* > bindVector;

    // Open a db transaction. This allows multiple INSERTs and UPDATEs to
    // happen very quickly. Failure to use a transaction in this instance
    // will cause lists to take roughly .25 seconds *per item*. With a transaction,
    // 10,000 items can be inserted or updated in ~1 second.
    session.begin();
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
                    session << "CREATE TABLE \"" << newTableName <<
                            "\" (id INTEGER PRIMARY KEY,PropertySetParentID TEXT,"
                            << fieldName << " " << columnType << ")", Poco::Data::now;
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

                session << listQuery, Poco::Data::now;
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
                    Poco::Data::Statement listStatement( session );
                    listStatement << listQuery;
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
                    if( !bindable->BindValue( &listStatement, currentValue ) )
                    {
                        std::cout << "Error in binding data" << std::endl;
                    }
                    else
                    {
                        listStatement.execute();
                    }
                }
            }
        }
        ++it;
    }
    // Close the db transaction
    session.commit();

    { // Braces protect scoping of biterator
        std::vector< BindableAnyWrapper* >::iterator biterator =
                bindVector.begin();
        while( biterator != bindVector.end() )
        {
            delete ( *biterator );
            ++biterator;
        }
    }
// TODO: need to add something of this to Persistable interface
//    m_writeDirty = false;
//    m_liveWriteDirty = false;

    //return returnVal;
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::LoadImpl( Persistable& persistable, Role )
{
    //std::cout << "SQLiteStore::LoadImpl" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );

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
    //mID = ID;

    Poco::Data::Statement statement( session );
    statement << "SELECT * FROM \"" << tableName << "\" WHERE uuid=:0", Poco::Data::use( uuidString );
    try
    {
        statement.execute();
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::LoadImpl: " << e.displayText() << std::endl;
    }

    Poco::Data::RecordSet recordset( statement );

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
                std::cout << "Didn't find conversion type" << std::endl << std::flush;
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
            Poco::Data::Statement statement( session );
            statement << "SELECT " << fieldName << " FROM \"" << persistable.GetTypeName()
                    << "_" << *it << "\" WHERE PropertySetParentID=:0"
                    , Poco::Data::use( mUUIDString );
            try
            {
                statement.execute();
            }
            catch( Poco::Data::DataException &e )
            {
                std::cout << "SQLiteStore::LoadImpl_vectorized: " << e.displayText() << std::endl;
            }
            Poco::Data::RecordSet recordset( statement );
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

    //std::cout << "SQLiteStore::LoadImpl done." << std::endl << std::flush;

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
void SQLiteStore::Remove( Persistable& persistable, Role )
{
    //std::cout << "SQLiteStore::Remove" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    std::string typeName = persistable.GetTypeName();
    if( HasIDForTypename( persistable.GetUUID(), persistable.GetTypeName() ) )
    {
        std::string idString = persistable.GetUUIDAsString();
        Poco::Data::Session session( m_pool->get() );
        session.setProperty( "maxRetryAttempts", 4 );
        session.setProperty( "transactionMode", std::string("IMMEDIATE") );
        try
        {
            session << "DELETE FROM \"" << typeName << "\" WHERE uuid=:uuid",
                Poco::Data::use( idString ),
                Poco::Data::now;
        }
        catch( Poco::Data::DataException &e )
        {
            std::cout << "SQLiteStore::Remove: " << e.displayText() << std::endl;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
bool SQLiteStore::HasIDForTypename( const boost::uuids::uuid& id, const std::string& typeName )
{
    //std::cout << "SQLiteStore::HasIDForTypename" << std::endl << std::flush;
    if( !m_pool )
    {
        return false;
    }

    if( !HasTypeName( typeName ) )
    {
        //std::cout << "Don't even have typename " << typeName << std::endl << std::flush;
        return false;
    }

    // This query will return a non-empty string iff the record exists
    std::string idTest;
    std::string idString = boost::lexical_cast< std::string >( id );
    //std::cout << "SQLiteStore::HasIDForTypename id = " << idString << std::endl << std::flush;

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );
    try
    {
        session << "SELECT uuid FROM \"" << typeName << "\" WHERE uuid=:uuid",
            Poco::Data::into( idTest ),
            Poco::Data::use( idString ),
            Poco::Data::now;
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::HasIDForTypename: " << e.displayText() << std::endl;
    }

    if( idTest.empty() )
    {
        return false;
    }
    else
    {
        return true;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::GetIDsForTypename( const std::string& typeName,
                                std::vector< std::string >& resultIDs )
{
    //std::cout << "SQLiteStore::GetIDsForTypename" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );
    Poco::Data::Statement statement( session );

    statement << "SELECT uuid FROM " << typeName, Poco::Data::into( resultIDs );
    try
    {
        statement.execute();
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::GetIDsForTypename: " << e.displayText() << std::endl;
    }
}
////////////////////////////////////////////////////////////////////////////////
void SQLiteStore::Search( const std::string& typeName,
                          std::vector< SearchCriterion >& criteria,
                          const std::string& returnField,
                          std::vector< std::string >& results )
{
    //std::cout << "SQLiteStore::Search" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    if( !HasTypeName( typeName ) )
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

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );
    Poco::Data::Statement statement( session );
    statement << "SELECT " << returnField << " FROM \"" << typeName << "\"";
    if( !wherePredicate.empty() )
    {
        statement << " WHERE " << wherePredicate;
    }
    statement, Poco::Data::into( results );

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
                bindable->BindValue( &statement, sc.m_value );
            }
        }
    }

    try
    {
        statement.execute();
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::Search: " << e.displayText() << std::endl;
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
    bool tableExists = false;

    // "SELECT 1 ... will put a 1 (true) into the boolean value if the tablename
    // is found in the database.
    try
    {
        session << "SELECT 1 FROM sqlite_master WHERE name=:name",
            Poco::Data::into( tableExists ),
            Poco::Data::use( TableName ),
            Poco::Data::now;
    }
    catch( Poco::Data::DataException &e )
    {
        std::cout << "SQLiteStore::_tableExists: " << e.displayText() << std::endl;
    }

    return tableExists;
}
////////////////////////////////////////////////////////////////////////////////
std::string SQLiteStore::_buildColumnHeaderString( const Persistable& persistable )
{
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
            std::cout << "Empty any looked like: " << dataType << std::endl;
            dataType = "UNKNOWN";
        }

        if( _containsIllegalCharacter( *it ) )
        {
            // This will cause the property to be skipped in db writes:
            dataType = "UNKNOWN";

            std::cout << "Error: Property " << (*it) << " contains a"
                    << " disallowed character. Allowed characters are digits"
                    << " 0-9, letters, and underscore. This property will not"
                    << " be written to the database." << std::endl;
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
    //std::cout << "SQLiteStore::Drop" << std::endl << std::flush;
    if( !m_pool )
    {
        return;
    }

    Poco::Data::Session session( m_pool->get() );
    session.setProperty( "maxRetryAttempts", 4 );
    session.setProperty( "transactionMode", std::string("IMMEDIATE") );

    if( _tableExists( session, typeName ) )
    {
        try
        {
            session << "DROP TABLE " << typeName, Poco::Data::now;
        }
        catch( Poco::Data::DataException &e )
        {
            std::cout << "SQLiteStore::Drop: " << e.displayText() << std::endl;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
