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

#include <map>
#include <string>
#include <vector>

#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>

#include <crunchstore/Datum.h>
#include <crunchstore/PersistablePtr.h>

// Defines VE_DATA_EXPORTS
#include <crunchstore/ExportConfig.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT Persistable
{
public:

    Persistable();

    /// Construct and set typename in one step
    Persistable( const std::string& typeName );

    /// Copy constructor
    Persistable( const Persistable& orig );

    virtual ~Persistable();

// UUID operations --
    /// Sets the UUID via a string; the UUID allows this persistable to be
    /// uniquely identified in the data store.
    virtual void SetUUID( std::string const& uuid );

    /// Sets the UUID via a boost::uuid
    virtual void SetUUID( boost::uuids::uuid const& uuid );

    /// Gets the UUID as a boost::uuid
    virtual boost::uuids::uuid const& GetUUID() const;

    /// Gets the UUID as a string
    virtual std::string const& GetUUIDAsString() const;
// --/


// TypeName operations --
    /** Sets the typename of the persistable. The application is responsible for
      * setting up an appropriate typename before saving or loading a
      * persistable. The typename defaults to the UUID of the persistable with
      * the dashes removed if left unset. It is not reccommended to leave this
      * default unchanged, as in most relational databases, this will result in
      * a new table for every single persistable -- and many RDBMS have a limit
      * on the number of tables allowed.
      *
      * The type should ideally be a unique human-readable name that
      * identifies "type" or "class" of data being stored. For example, a
      * retail sales system might have types corresponding to "customers",
      * "products", "stores", etc. In many cases, the typename will be the same
      * or similar to the classname of the data being stored. In all cases, any
      * two PersistableS with the same typename must have exactly the same
      * data fields -- in (data)type, number, and name. This is because some
      * common back-end store implementations (notably those that use SQL)
      * will use the typename as the tablename in the database.
      **/
    virtual void SetTypeName( std::string name );

    /** Returns the typename.
      **/
    virtual std::string const& GetTypeName() const;
// --/


// datum operations --
    /// Adds a new datum named datumName with main value value
    virtual void AddDatum( const std::string& datumName,
                              boost::any value );

    /// Adds a new datum named datumName from a pre-created datum object
    virtual void AddDatumPtr( const std::string& datumName,
                              DatumPtr datum );

    /// Returns a pointer to the datum identified by datumName
    /// @throws std::runtime_error if datum does not exist
    virtual DatumPtr GetDatum( std::string const& datumName ) const;

    /// Sets the main value of the datum identified by datumName to value.
    virtual bool SetDatumValue( std::string const& datumName,
                                boost::any value );

    /// Returns the main value of the datum identified by datumName, converted
    /// to type T. Example: persist.GetDatumValue< double >( "MyDouble" );
    /// @throws std::runtime_error if datum does not exist
    template <typename T>
            T GetDatumValue( std::string const& datumName ) const
    {
        return GetDatum( datumName )->extract<T>();
    }

    /// Returns true if this Persistable owns a datum with name datumName,
    /// false if not.
    virtual bool DatumExists( std::string const& datumName ) const;

    /// Returns a vector containing the identifying names of all data
    /// contained in this set. Names are in the order in which they were added
    /// to the Persistable.
    virtual const std::vector<std::string>& GetDataList() const;
// --/

protected:

    /// Creates a new UUID for this Persistable. Used for lazy creation of
    /// UUIDs.
    void CreateUUID() const;

    /// Creates a default TypeName based on the UUID if a TypeName has not
    /// previously been set. Used for lazy TypeName creation since UUID
    /// creation is also lazy.
    void CreateTypeNameFromUUID() const;

    /// Typedef for datum map to make it easier to switch to a different
    /// underlying type in future if needed.
    typedef std::map< std::string, DatumPtr > DataMap;

    /// Map holding the collection of data.
    DataMap m_dataMap;

    /// Maintains a list of available data sorted by order of addition
    std::vector< std::string > m_dataList;

    /// The uuid; mutable since it might need to be created in first call
    /// to GetUUID, GetUUIDString, or first time it is saved to a store.
    mutable boost::uuids::uuid m_UUID;
    /// The uuid as a std::string; mutable for same reason as m_UUID.
    mutable std::string m_UUIDString;

private:
    /// Flag to tell whether we've assigned a UUID. The creation of
    /// a UUID is expensive, so we delay creation until it is needed because
    /// there will be many cases in which application code explicitly sets
    /// a UUID (so we don't need to generate one), or where the persistable
    /// is used strictly as a holder for data without need for a UUID. Mutable
    /// since this is associated with lazy UUID creation.
    mutable bool m_uuidSet;

    /// Holds the typename; mutable since it is (sometimes) associated with
    /// lazy creation of UUID.
    mutable std::string m_typename;

    /// Empty string to use when need to return an empty string by reference
    std::string emptyString;

    /// Empty vector of strings to use whenever we need to return one by
    /// reference
    std::vector<std::string> emptyVectorOfStrings;

};

} // namespace crunchstore
