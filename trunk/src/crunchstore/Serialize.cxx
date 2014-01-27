#include <crunchstore/Serialize.h>
#include <crunchstore/Exception.h>

#include <Poco/Dynamic/Struct.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Handler.h>
#include <Poco/JSON/ParseHandler.h>

namespace crunchstore
{

Serialize::Serialize()
{
}

std::string Serialize::ToJSON( Persistable& p )
{
    using Poco::Dynamic::Var;
    using Poco::Dynamic::Struct;
    Var v = Struct< std::string >();

    std::vector<std::string>::const_iterator key = p.GetDataList().begin();
    std::vector<std::string>::const_iterator end = p.GetDataList().end();

    while( key != end )
    {
        DatumPtr d = p.GetDatum( *key );
        if( d->IsBool() )
        {
            v[*key] = d->extract< bool >();
        }
        else if( d->IsInt() )
        {
            v[*key] = d->extract< int >();
        }
        else if( d->IsFloat() )
        {
            v[*key] = d->extract< float >();
        }
        else if( d->IsDouble() )
        {
            v[*key] = d->extract< double >();
        }
        else if( d->IsString() )
        {
            v[*key] = d->extract< std::string >();
        }
        else if( d->IsIntVector() )
        {
            v[*key] = d->extract< std::vector< int > >();
        }
        else if( d->IsFloatVector() )
        {
            v[*key] = d->extract< std::vector< float > >();
        }
        else if( d->IsDoubleVector() )
        {
            v[*key] = d->extract< std::vector< double > >();
        }
        else if( d->IsStringVector() )
        {
            v[*key] = d->extract< std::vector< std::string > >();
        }
        else if( d->IsBLOB() )
        {
            v[*key] = d->extract< std::vector< char > >();
        }
        ++key;
    }

    v["uuid"] = p.GetUUIDAsString();

    std::string json("{ \"typename\" : \"");
    json += p.GetTypeName() + "\", \"data\" : "
            + Poco::Dynamic::Var::toString( v ) + " }";
    return json;
}

void Serialize::FromJSON( Persistable& p, const std::string& json )
{
    using Poco::Dynamic::Var;
    using Poco::Dynamic::Struct;
    using Poco::JSON::Parser;
    using Poco::JSON::ParseHandler;
    using Poco::JSON::Object;

    ParseHandler::Ptr handler = new ParseHandler( true );
    Parser parser( handler );
    Var result = parser.parse( json );

    Object::Ptr set;
    if ( result.type() == typeid( Object::Ptr ) )
    {
        set = result.extract<Object::Ptr>();
    }

    // Require a matching typename
    if( !set->has( "typename" ) )
    {
        throw Exception("Serialize::FromJSON error", "JSON contains no typename");
    }
    if( set->getValue<std::string>( "typename" ) != p.GetTypeName() )
    {
        throw Exception("Serialize::FromJSON error", "Typename mismatch");
    }

    Var tmp = set->getObject( "data" );
    Var s = Struct< std::string >();
    s = Var::parse( Var::toString( tmp ) );
    Struct<std::string> v = s.extract< Struct<std::string> >();

    if( !v.contains("uuid") )
    {
        throw Exception("Serialize::FromJSON error", "No uuid in JSON");
    }
    p.SetUUID( v["uuid"].extract<std::string>() );
    std::vector<std::string>::const_iterator key = p.GetDataList().begin();
    std::vector<std::string>::const_iterator end = p.GetDataList().end();
    while( key != end )
    {
        if( !v.contains(*key) )
        {
            continue;
        }

        DatumPtr d = p.GetDatum( *key );
        if( d->IsBool() )
        {
            d->SetValue( v[*key].convert< bool >() );
        }
        else if( d->IsInt() )
        {
            d->SetValue( v[*key].convert< int >() );
        }
        else if( d->IsFloat() )
        {
            d->SetValue( v[*key].convert< float >() );
        }
        else if( d->IsDouble() )
        {
            d->SetValue( v[*key].convert< double >() );
        }
        else if( d->IsString() )
        {
            d->SetValue( v[*key].convert< std::string >() );
        }
        else if( d->IsIntVector() )
        {
            SetVec< int >( d, v[*key] );
        }
        else if( d->IsFloatVector() )
        {
            SetVec< float >( d, v[*key] );
        }
        else if( d->IsDoubleVector() )
        {
            SetVec< double >( d, v[*key] );
        }
        else if( d->IsStringVector() )
        {
            SetVec< std::string >( d, v[*key] );
        }
        else if( d->IsBLOB() )
        {
            SetVec< char >( d, v[*key] );
        }
        ++key;
    }
}

}
