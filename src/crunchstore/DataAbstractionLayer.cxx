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
#include <crunchstore/DataAbstractionLayer.h>
#include <crunchstore/SearchCriterion.h>

namespace crunchstore
{
////////////////////////////////////////////////////////////////////////////////
DataAbstractionLayer::DataAbstractionLayer()
{
}
////////////////////////////////////////////////////////////////////////////////
DataAbstractionLayer::~DataAbstractionLayer()
{

}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::Save( const Persistable& persistable, Role role  )
{
    if( m_child )
        m_child->Save( persistable, role );
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::Load( Persistable& persistable, Role role )
{
    if( m_child )
        m_child->Load( persistable, role );
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::Remove( Persistable& persistable,
                                   Role role )
{
    if( m_child )
        m_child->Remove( persistable, role );
}
////////////////////////////////////////////////////////////////////////////////
bool DataAbstractionLayer::HasIDForTypename( const boost::uuids::uuid& id, const std::string& typeName )
{
    return m_child->HasIDForTypename( id, typeName );
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::GetIDsForTypename( const std::string& typeName, std::vector< std::string >& resultIDs )
{
    if( m_child )
        m_child->GetIDsForTypename( typeName, resultIDs );
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::Search( const std::string& typeName,
                                   std::vector< SearchCriterion >& criteria,
                                   const std::string& returnField,
                                   std::vector< std::string >& results )
{
    if( m_child )
        m_child->Search( typeName, criteria, returnField, results );
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::ProcessBackgroundTasks()
{
    // Do nothing in base case.
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::SetChild( DataAbstractionLayerPtr child )
{
    m_child = child;
}
////////////////////////////////////////////////////////////////////////////////
void DataAbstractionLayer::Drop( const std::string& typeName, Role role )
{
    if( m_child )
        m_child->Drop( typeName, role );
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore

