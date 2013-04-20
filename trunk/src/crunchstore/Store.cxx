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
#include <crunchstore/Store.h>

namespace crunchstore
{
////////////////////////////////////////////////////////////////////////////////
Store::Store()
{
}
////////////////////////////////////////////////////////////////////////////////
Store::~Store()
{
}
////////////////////////////////////////////////////////////////////////////////
void Store::SetStoreRole( StoreRole )
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::SetSyncMode( SyncMode )
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::SetAccessMode( AccessMode )
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::Attach()
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::Detach()
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::Save( const Persistable& persistable, Role role,
                  const TransactionKey& transactionKey  )
{
    SaveImpl( persistable, role, transactionKey );
}
////////////////////////////////////////////////////////////////////////////////
void Store::Load( Persistable& persistable, Role role,
                  const TransactionKey& transactionKey )
{
    LoadImpl( persistable, role, transactionKey );
}
////////////////////////////////////////////////////////////////////////////////
//TransactionKey Store::BeginTransaction()
//{
//    return TransactionKey();
//}
////////////////////////////////////////////////////////////////////////////////
void Store::EndTransaction( TransactionKey& )
{
}
////////////////////////////////////////////////////////////////////////////////
void Store::SaveImpl( const Persistable&, Role, const TransactionKey&  )
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::LoadImpl( Persistable&, Role, const TransactionKey&  )
{

}
////////////////////////////////////////////////////////////////////////////////
void Store::SetStorePath( const std::string&  )
{

}
////////////////////////////////////////////////////////////////////////////////
bool Store::HasTypeName( const std::string&  )
{
    return false;
}
////////////////////////////////////////////////////////////////////////////////
} // namespace crunchstore
