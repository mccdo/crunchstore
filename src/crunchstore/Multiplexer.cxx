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
#include <crunchstore/Multiplexer.h>
#include <crunchstore/Persistable.h>
#include <crunchstore/Exception.h>

#include <iostream>

namespace crunchstore
{

Multiplexer::Multiplexer()
{
    //std::cout << "MUX: " << this << std::endl << std::flush;
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::Save( const Persistable& persistable, Role role, const TransactionKey& transactionKey )
{
    if( role & WORKING_ROLE )
    {
        //std::cout << "Multiplexer::Save -- WORKING_ROLE" << std::endl;
        if( m_workingStore )
        {
            m_workingStore->Save( persistable, role, transactionKey );
        }
    }

    if( role & BACKING_ROLE )
    {
        //std::cout << "Multiplexer::Save -- BACKING_ROLE" << std::endl;
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            (*it)->Save( persistable, role, transactionKey );
            ++it;
        }
    }

    if( role & VERSIONING_ROLE )
    {
        //std::cout << "Multiplexer::Save -- VERSIONING_ROLE" << std::endl;
        if( m_fullVersioningStore )
            m_fullVersioningStore->Save( persistable, role, transactionKey );
    }
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::Load( Persistable& persistable, Role role,
                        const TransactionKey& transactionKey )
{
    switch( role )
    {
    case VERSIONING_ROLE:
    {
        if( m_fullVersioningStore )
            m_fullVersioningStore->Load( persistable, role, transactionKey );
        break;
    }
    case WORKING_ROLE:
    {
        if( m_workingStore )
            m_workingStore->Load( persistable, role, transactionKey );
        break;
    }
    case DEFAULT_ROLE:
    {
        if( m_workingStore )
        {
            const boost::uuids::uuid id = persistable.GetUUID();
            // Find out which store contains this persistable and load from there.
            // If workingStore has it, load from there and move on
            // TODO: Should we be caching locations of IDs on load?
            if( m_workingStore->HasIDForTypename( id, persistable.GetTypeName() ) )
            {
                m_workingStore->Load( persistable, role, transactionKey );
                break;
            }
        }
        // If we didn't find it in the workingStore, fall through to backingStore.
    }
    case BACKING_ROLE:
    {
        std::cout << "Multiplexer::Load: Falling back to the BACKING_ROLE for persistable "
            << persistable.GetTypeName() << std::endl;
        // Find out which store contains this persistable and load from there.
        // TODO: Should we be caching locations of IDs on load?
        const boost::uuids::uuid id = persistable.GetUUID();
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            if( (*it)->HasIDForTypename( id, persistable.GetTypeName() ) )
            {
                (*it)->Load( persistable, role, transactionKey );
                break;
            }
            ++it;
        }
        break;
    }
    default:
    {
        ;// do nothing
    }
    } // switch( role )
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::Remove( Persistable& persistable, Role role,
                          const TransactionKey& transactionKey )
{
    switch( role )
    {
    case VERSIONING_ROLE:
    {
        if( m_fullVersioningStore )
            m_fullVersioningStore->Remove( persistable, role, transactionKey );
        break;
    }
    case WORKING_ROLE:
    {
        if( m_workingStore )
            m_workingStore->Remove( persistable, role, transactionKey );
        break;
    }
    case DEFAULT_ROLE:
    {
        if( m_workingStore )
        {
            const boost::uuids::uuid id = persistable.GetUUID();
            if( m_workingStore->HasIDForTypename( id, persistable.GetTypeName() ) )
            {
                m_workingStore->Remove( persistable, role, transactionKey );
            }
        }
        // DEFAULT_ROLE always falls through to BACKING_ROLE
    }
    case BACKING_ROLE:
    {
        // Find out which backingStore contains this persistable and load from there.
        const boost::uuids::uuid id = persistable.GetUUID();
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            if( (*it)->HasIDForTypename( id, persistable.GetTypeName() ) )
            {
                (*it)->Remove( persistable, role, transactionKey );
                break;
            }
            ++it;
        }
        break;
    }
    default:
    {
        ;// do nothing
    }
    } // switch( role )
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::GetIDsForTypename( const std::string& typeName,
                                     std::vector< std::string >& resultIDs,
                                     Role role )
{
    switch( role )
    {
    case VERSIONING_ROLE:
    {
        if( m_fullVersioningStore )
            m_fullVersioningStore->GetIDsForTypename( typeName, resultIDs );
        break;
    }
    case WORKING_ROLE:
    {
        if( m_workingStore )
            m_workingStore->GetIDsForTypename( typeName, resultIDs );
        break;
    }
    case DEFAULT_ROLE:
    {
        if( m_workingStore )
            m_workingStore->GetIDsForTypename( typeName, resultIDs );
        // DEFAULT_ROLE always falls through to BACKING_ROLE
    }
    case BACKING_ROLE:
    {
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            (*it)->GetIDsForTypename( typeName, resultIDs );
            ++it;
        }
        break;
    }
    default:
    {
        ;// do nothing
    }
    } // switch( role )
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::Search( const std::string& typeName,
                          std::vector< SearchCriterion >& criteria,
                          const std::string& returnField,
                          std::vector< std::string >& results )
{
    // For now send all search requests to working_store. Need to think about
    // whether this method should take a role parameter, and if not, how to
    // search non=working role stores.
    if( m_workingStore )
    {
        m_workingStore->Search( typeName, criteria, returnField, results );
    }
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::ProcessBackgroundTasks()
{
    if( m_workingStore )
        m_workingStore->ProcessBackgroundTasks();
    if( m_fullVersioningStore )
        m_fullVersioningStore->ProcessBackgroundTasks();
    std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
    while( it != m_backingStores.end() )
    {
        (*it)->ProcessBackgroundTasks();
    }
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::AttachStore( DataAbstractionLayerPtr store,
                                         Store::StoreRole role,
                                         Store::SyncMode )
{
    //std::cout << "Multiplexer::AttachStore" << std::endl;
    std::pair< DataAbstractionLayerPtr, Store::StoreRole > storePair( store, role );
    m_stores.push_back( storePair );

    if( role == Store::WORKINGSTORE_ROLE )
    {
        m_workingStore = store;
    }
    else if( role == Store::BACKINGSTORE_ROLE )
    {
        m_backingStores.push_back( store );
    }

    static_cast< Store* >(store.get())->Attach();
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::DetachStore( DataAbstractionLayerPtr store )
{
    StoreListType::iterator it = m_stores.begin();
    while( it != m_stores.end() )
    {
        if( it->first == store  )
        {
            static_cast< Store* >(it->first.get())->Detach();
            m_stores.erase( it );
            return;
        }
        ++it;
    }

    if( store == m_workingStore )
    {
        m_workingStore = DataAbstractionLayerPtr();
    }
    else if( store == m_fullVersioningStore )
    {
        m_fullVersioningStore = DataAbstractionLayerPtr();
    }
    else
    {

    }

    // Do we want to throw if the passed store wasn't found?
    //throw "DetachStore: unable to find store.";
    throw Exception( "Detach Store Error", "Unable to find store" );
}
////////////////////////////////////////////////////////////////////////////////
void Multiplexer::Drop( const std::string& typeName, Role role  )
{
    switch( role )
    {
    case VERSIONING_ROLE:
    {
        if( m_fullVersioningStore )
            m_fullVersioningStore->Drop( typeName, role );
        break;
    }
    case WORKING_ROLE:
    {
        if( m_workingStore )
            m_workingStore->Drop( typeName, role );
        break;
    }
    case DEFAULT_ROLE:
    {
        if( m_workingStore )
            m_workingStore->Drop( typeName, role );
        // DEFAULT_ROLE always falls through to BACKING_ROLE
    }
    case BACKING_ROLE:
    {
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            (*it)->Drop( typeName, role );
            ++it;
        }
        break;
    }
    default:
    {
        ;// do nothing
    }
    } // switch( role )
}
////////////////////////////////////////////////////////////////////////////////
bool Multiplexer::HasIDForTypename( const boost::uuids::uuid& id,
                                    const std::string& typeName,
                                    Role role )
{
    int result = 0;
    switch( role )
    {
    case VERSIONING_ROLE:
    {
        if( m_fullVersioningStore )
            result += m_fullVersioningStore->HasIDForTypename( id, typeName, role );
        break;
    }
    case WORKING_ROLE:
    {
        if( m_workingStore )
            result += m_workingStore->HasIDForTypename( id, typeName, role );
        break;
    }
    case DEFAULT_ROLE:
    {
        if( m_workingStore )
            result += m_workingStore->HasIDForTypename( id, typeName, role );
        // DEFAULT_ROLE always falls through to BACKING_ROLE
    }
    case BACKING_ROLE:
    {
        std::vector< DataAbstractionLayerPtr >::iterator it = m_backingStores.begin();
        while( it != m_backingStores.end() )
        {
            result += (*it)->HasIDForTypename( id, typeName, role );
            ++it;
        }
        break;
    }
    default:
    {
        ;// do nothing
    }
    } // switch( role )
    return static_cast<bool>(result);
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
