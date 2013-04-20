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
#include <crunchstore/StorePtr.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT Store : public DataAbstractionLayer
{
public:
    // These are binary flags: DEFAULTSTORE_ROLE is intended to be a combination
    // of WORKINGSTORE_ROLE and BACKINGSTORE_ROLE
    enum StoreRole { WORKINGSTORE_ROLE = 1, BACKINGSTORE_ROLE = 2,
                     DEFAULTSTORE_ROLE = 3, VERSIONINGSTORE_ROLE = 4 };

    enum SyncMode { SYNCHRONOUS_MODE, ASYNCHRONOUS_MODE };

    // Read/write access mode flags.
    enum AccessMode { READ_MODE = 1, WRITE_MODE = 2 };

    Store();

    virtual ~Store();

    virtual void SetStoreRole( StoreRole role );
    virtual void SetSyncMode( SyncMode mode );
    virtual void SetAccessMode( AccessMode mode );

    virtual void Attach();
    virtual void Detach();

    /**
     * Every type of store has some concept of a path or location. For local
     * file-based stores @c path is literally the path to a file. For
     * server-based stores, @c is some sort of network connection string.
     */
    virtual void SetStorePath( const std::string& path );

    /**
     * Checks whether table with @c typeName exists in the current database.
     * @param typeName Name of table to check for
     * @return @c true if table exists, @c false otherwise.
     */
    virtual bool HasTypeName( const std::string& typeName );

    /// Override from DataAbstractionLayer. This method must not be overridden
    /// by derived classes, which should instead override SaveImpl.
    void Save( const Persistable& persistable, Role role = DEFAULT_ROLE,
               const TransactionKey& transactionKey = TransactionKey()  );

    /// Override from DataAbstractionLayer. This method must not be overridden
    /// by derived classes, which should instead override LoadImpl.
    void Load( Persistable& persistable, Role role = DEFAULT_ROLE,
               const TransactionKey& transactionKey = TransactionKey() );

    /// Begins a bulk transaction with the store. Returns a transaction key that
    /// must be passed along with any Load, Save, or Remove operation that
    /// should be part of this bulk transaction.
    //virtual TransactionKey BeginTransaction();

    /// End the bulk transaction associated with @c transactionKey.
    virtual void EndTransaction( TransactionKey& transactionKey );


protected:
    /// Derived classes should override this to do their actual save operation
    virtual void SaveImpl( const Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           const TransactionKey& transactionKey = TransactionKey()  );

    /// Derived classes should override this to do their actual load operation
    virtual void LoadImpl( Persistable& persistable,
                           Role role = DEFAULT_ROLE,
                           const TransactionKey& transactionKey = TransactionKey() );

private:
    StoreRole m_storeRole;
    SyncMode m_storeMode;
    AccessMode m_accessMode;
};

} // namespace crunchstore
