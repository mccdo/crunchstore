#pragma once

#include <string>
#include <vector>

#include <Poco/Dynamic/Var.h>

#include <crunchstore/Persistable.h>
#include <crunchstore/Datum.h>

namespace crunchstore
{

using Poco::Dynamic::Var;
class CRUNCHSTORE_EXPORT Serialize
{
public:
    Serialize();

    static std::string ToJSON( Persistable& p );
    static void FromJSON( Persistable& p, const std::string& json );

private:
    template< typename T >
    static void SetVec( DatumPtr d, Var& v )
    {
        std::vector< Var> vvec = v.extract< std::vector< Var > >();
        std::vector< T > dvec;
        for( size_t idx = 0; idx < vvec.size(); ++idx )
        {
            dvec.push_back( vvec.at(idx).convert< T >() );
        }
        d->SetValue( dvec );
    }
};

}
