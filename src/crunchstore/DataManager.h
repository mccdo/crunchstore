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

#include <crunchstore/DataAbstractionLayer.h>
#include <crunchstore/Persistable.h>
#include <crunchstore/Cache.h>
#include <crunchstore/BufferBase.h>
#include <crunchstore/Store.h>
#include <crunchstore/Multiplexer.h>
#include <crunchstore/SearchCriterion.h>
#include <crunchstore/DataManagerPtr.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT DataManager: public DataAbstractionLayer
{
public:
    DataManager();

    virtual ~DataManager();

    // Overrides of DataAbstractionLayer
    void ProcessBackgroundTasks();
    // I think these two can be subsumed by passing role = VERSION_ROLE into
    // save and load.
    //void VersioningSave( const Persistable& persistable );
    //void VersioningLoad( Persistable& persistable );

    // Management functions common to this class and Multiplexer.
    // The methods in this class are forwarding functions to those in
    // Multiplexer.
    void AttachStore( DataAbstractionLayerPtr store,
                      Store::StoreRole role,
                      Store::SyncMode mode = Store::ASYNCHRONOUS_MODE );
    void DetachStore( DataAbstractionLayerPtr store );

    // Management fucntions unique to this class
    void SetCache( DataAbstractionLayerPtr cache );
    void SetBuffer( DataAbstractionLayerPtr buffer );

    // Shared among this class, Cache, and BufferBase
    void Buffer( std::vector< std::string > ids,
                 BufferBase::BufferPriority priority = BufferBase::NORMAL_PRIORITY );

private:
    DataAbstractionLayerPtr m_dataMultiplexer;
    DataAbstractionLayerPtr m_buffer;
};

} // namespace crunchstore
