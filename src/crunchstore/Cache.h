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

#include <crunchstore/ExportConfig.h>

#include <crunchstore/BufferBase.h>

namespace crunchstore
{

class CRUNCHSTORE_EXPORT Cache : public BufferBase
{
public:
    Cache();
    virtual ~Cache();
    virtual void Buffer( std::vector< std::string > ids, BufferPriority priority = NORMAL_PRIORITY ) = 0;

    /// Override of method in DataAbstractionLayer. A cache is always the first
    /// "real" layer a Persistable travels through on the way to a store. Since
    /// we use lazy UUID creation in Persistable, we must ensure that the
    /// Persistable gets a valid UUID before anything else happens. To do that,
    /// we simply need to request the UUID via persistable.GetUUID(). Any class
    /// that derives from Cache and overrides this method must do two things:
    /// 1. Call persistable.GetUUID() before you do anything else
    /// 2. Call m_child->Save( persistable, role ) to ensure that the save
    ///    call propagates up the chain to a buffer and a store.
    virtual void Save( const Persistable& persistable, Role role = DEFAULT_ROLE );
};

} // namespace crunchstore
