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
#include <crunchstore/Persistable.h>
#include <crunchstore/Exception.h>
#include <crunchstore/Datum.h>

#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/erase.hpp>

namespace crunchstore
{

////////////////////////////////////////////////////////////////////////////////
Persistable::Persistable():
    m_uuidSet( false )
{
}
////////////////////////////////////////////////////////////////////////////////
Persistable::Persistable( const std::string& typeName ):
    m_uuidSet( false )
{
    m_typename = typeName;
}
////////////////////////////////////////////////////////////////////////////////
Persistable::Persistable( const Persistable& orig ):
        m_dataList( orig.m_dataList ),
        m_typename( orig.m_typename )
{
    // Since we use lazy generation of UUIDs, we have to ensure that a unique
    // one has been generated before doing a copy; otherwise, if no call has
    // been made to generate a UUID on the original, the copy and the original
    // will end up with different UUIDs.
    m_UUID = orig.GetUUID();
    m_UUIDString = orig.m_UUIDString;
    m_uuidSet = orig.m_uuidSet;
    // Since DataMap holds smart pointers to instances of Datum, we have to
    // create new instances of the underlying Datum
    DataMap::const_iterator it = orig.m_dataMap.begin();
    while( it != orig.m_dataMap.end() )
    {
        DatumPtr datum( new Datum( *(it->second) ) );
        m_dataMap[ it->first ] = datum;
        ++it;
    }
}
////////////////////////////////////////////////////////////////////////////////
Persistable::~Persistable()
{

}
////////////////////////////////////////////////////////////////////////////////
void Persistable::CreateUUID() const
{
    m_UUID = boost::uuids::random_generator()();
    m_UUIDString = boost::lexical_cast< std::string >( m_UUID );
    m_uuidSet = true;

    if( m_typename.empty() )
    {
        CreateTypeNameFromUUID();
    }
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::CreateTypeNameFromUUID() const
{
    m_typename = m_UUIDString;
    boost::algorithm::erase_all( m_typename, "-" );
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::SetUUID( std::string const& uuid )
{
    m_UUIDString = uuid;
    boost::uuids::string_generator gen;
    m_UUID = gen( uuid );
    m_uuidSet = true;
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::SetUUID( boost::uuids::uuid const& uuid )
{
    m_UUID = uuid;

    std::stringstream ss;
    ss << m_UUID;
    m_UUIDString = ss.str();

    m_uuidSet = true;
}
////////////////////////////////////////////////////////////////////////////////
boost::uuids::uuid const& Persistable::GetUUID() const
{
    if( !m_uuidSet )
    {
        CreateUUID();
    }
    return m_UUID;
}
////////////////////////////////////////////////////////////////////////////////
std::string const& Persistable::GetUUIDAsString() const
{
    if( !m_uuidSet )
    {
        CreateUUID();
    }
    return m_UUIDString;
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::SetTypeName( std::string name )
{
    m_typename = name;
}
////////////////////////////////////////////////////////////////////////////////
std::string const& Persistable::GetTypeName() const
{
    if( !m_uuidSet )
    {
        CreateUUID();
    }
    return m_typename;
}
////////////////////////////////////////////////////////////////////////////////
bool Persistable::DatumExists( std::string const& datumName ) const
{
    bool result = false;

    DataMap::const_iterator iterator = m_dataMap.find( datumName );
    if( iterator != m_dataMap.end() )
    {
        result = true;
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::AddDatum( const std::string& datumName,
                          boost::any value )
{
    // No-op if datum with this name already exists
    if( DatumExists( datumName ) )
    {
        return;
    }

    DatumPtr datum = DatumPtr( new Datum( value ) );
    AddDatumPtr( datumName, datum );
}
////////////////////////////////////////////////////////////////////////////////
void Persistable::AddDatumPtr( const std::string& datumName,
                          DatumPtr datum )
{
    // No-op if datum with this name already exists
    if( DatumExists( datumName ) )
    {
        return;
    }

    m_dataMap[ datumName ] = datum;

    m_dataList.push_back( datumName );
}

////////////////////////////////////////////////////////////////////////////////
DatumPtr Persistable::GetDatum( std::string const& datumName ) const
{
    DataMap::const_iterator iterator = m_dataMap.find( datumName );
    if( iterator != m_dataMap.end() )
    {
        return (*iterator ).second;
    }
    else
    {
        // Unsure whether it's preferable to throw an exception here or just
        // return an empty DatumPtr. Pro/Con for exception:
        // Pros: *User never has to test whether the returned DatumPtr is empty
        //       *Compound operations on a returned DatumPtr will fail before
        //        the pointer is accessed, allowing for the possiblity of
        //        graceful failure rather than a crash.
        // Cons: *User either has to test for existence of Datum before calling
        //        GetDatum or has to wrap access in try/catch block.
        std::string error( "No datum named " );
        error += datumName + " in Persistable named " + m_typename;
        throw Exception( "Invalid Datum name", error );
    }
}
////////////////////////////////////////////////////////////////////////////////
bool Persistable::SetDatumValue( std::string const& datumName,
                                 boost::any value )
{
    DataMap::const_iterator iterator = m_dataMap.find( datumName );
    if( iterator != m_dataMap.end() )
    {
        return ( *iterator ).second->SetValue( value );
    }
    else
    {
        return false;
    }
}
////////////////////////////////////////////////////////////////////////////////
const std::vector<std::string>& Persistable::GetDataList() const
{
    return m_dataList;
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
