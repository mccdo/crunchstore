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
#include <crunchstore/Store.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT Multiplexer: public DataAbstractionLayer
{
public:
    Multiplexer();


    // Overrides of DataAbstractionLayer
    virtual void Save( const Persistable& persistable,
                       Role role = DEFAULT_ROLE  );
    virtual void Load( Persistable& persistable, Role role = DEFAULT_ROLE );
    virtual void Remove( Persistable& persistable, Role role );
    virtual void GetIDsForTypename( const std::string& typeName,
                                    std::vector< std::string >& resultIDs,
                                    Role role );
    virtual void Search( const std::string& typeName,
                         std::vector< SearchCriterion >& criteria,
                         const std::string& returnField,
                         std::vector< std::string >& results );

    virtual void Drop( const std::string& typeName, Role role = DEFAULT_ROLE  );

    void ProcessBackgroundTasks();

    // Attaches and detaches stores from the data access flow
    void AttachStore( DataAbstractionLayerPtr store,
                      Store::StoreRole role,
                      Store::SyncMode mode = Store::ASYNCHRONOUS_MODE );
    void DetachStore( DataAbstractionLayerPtr store );

private:
    typedef std::vector< std::pair< DataAbstractionLayerPtr, Store::StoreRole > > StoreListType;
    StoreListType m_stores;

    DataAbstractionLayerPtr m_workingStore;
    std::vector< DataAbstractionLayerPtr > m_backingStores;
    DataAbstractionLayerPtr m_fullVersioningStore;
    //DataAbstractionLayerPtr m_snapshotVersioningStores;
};

} // namespace crunchstore
