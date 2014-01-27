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
#include <crunchstore/DataManager.h>
#include <crunchstore/NullCache.h>
#include <crunchstore/NullBuffer.h>

namespace crunchstore
{
////////////////////////////////////////////////////////////////////////////////
DataManager::DataManager()
{
    m_dataMultiplexer = DataAbstractionLayerPtr( new Multiplexer );
    SetCache( DataAbstractionLayerPtr( new NullCache ) );
    SetBuffer( DataAbstractionLayerPtr( new NullBuffer ) );
}
////////////////////////////////////////////////////////////////////////////////
DataManager::~DataManager()
{

}
////////////////////////////////////////////////////////////////////////////////
void DataManager::ProcessBackgroundTasks()
{

}
////////////////////////////////////////////////////////////////////////////////
void DataManager::AttachStore( DataAbstractionLayerPtr store,
                               Store::StoreRole role,
                               Store::SyncMode mode )
{
    static_cast< Multiplexer* >( m_dataMultiplexer.get() )->
            AttachStore( store, role, mode );
}
////////////////////////////////////////////////////////////////////////////////
void DataManager::DetachStore( DataAbstractionLayerPtr store )
{
    static_cast< Multiplexer* >( m_dataMultiplexer.get() )->
            DetachStore( store );
}
////////////////////////////////////////////////////////////////////////////////
void DataManager::SetCache( DataAbstractionLayerPtr cache )
{
    // If we already have a cache and buffer set, re-parent the buffer
    if( m_child && m_buffer )
    {
        cache->SetChild( m_buffer );
    }
    SetChild( cache );
}
////////////////////////////////////////////////////////////////////////////////
void DataManager::SetBuffer( DataAbstractionLayerPtr buffer )
{
    // m_child is the cache
    m_child->SetChild( buffer );
    m_buffer = buffer;
    m_buffer->SetChild( m_dataMultiplexer );
}
////////////////////////////////////////////////////////////////////////////////
void DataManager::Buffer( std::vector< std::string > ids,
                          BufferBase::BufferPriority priority )
{
    (static_cast< Cache* >(m_child.get()))->Buffer( ids, priority );
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
