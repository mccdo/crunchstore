#include <iostream>
#include <vector>
#include <fstream>

#include <crunchstore/Persistable.h>
#include <crunchstore/Datum.h>
#include <crunchstore/DataManager.h>
#include <crunchstore/SQLiteStore.h>
#include <crunchstore/Serialize.h>

using namespace crunchstore;

/// An example data class
class DC
{
public:


    DC(
        int col1 = 0,
        double col2 = 0.0,
        std::string col3 = "",
        bool col4 = false,
        std::vector< double > col5 = std::vector< double >() )
    {
        m_data.AddDatum( "Col1", col1 );
        m_data.AddDatum( "Col2", col2 );
        m_data.AddDatum( "Col3", col3 );
        m_data.AddDatum( "Col4", col4 );
        m_data.AddDatum( "Col5", col5 );

        m_data.SetTypeName( "DC" );
    }

    // Need some sort of method like this to get at the Persistable
    Persistable& GetData()
    {
        return m_data;
    }

    // Syntactic sugar; user code could call GetData().GetDatum( name ) instead.
    DatumPtr const operator [](
        std::string const& name ) const
    {
        return m_data.GetDatum( name );
    }

private:
    /// Holds the data
    Persistable m_data;
};

////////////////////////////////////////////////////////////////////////////////

int main(int, char**)
{
    using namespace std;
    using namespace crunchstore;

    // Set up a DataManager to test persistence. This uses a NullCache and
    // NullBuffer by default.
    DataManager manager;

    // Add an SQLite store; we could just as easily add a MongoDB store
    // (crunchstore already supports MongoDB), or with minor tweaks to the
    // SQLiteStore code, anything that Poco::Data supports.
    SQLiteStorePtr sqstore( new SQLiteStore );
    sqstore->SetStorePath( "DCTest.db" );
    manager.AttachStore( sqstore, Store::WORKINGSTORE_ROLE );

    DC dc( 333, 1.4142, "A String", true );

    // Read a value out of dc
    cout << dc["Col3"]->extract< std::string >() << endl;
    // OR
    //cout << dc.GetData().GetDatumValue< std::string >("Col3") << endl;
    // OR
    //cout << dc.GetData().GetDatum("Col3")->extract< std::string >() << endl;


    // Save dc to the store held by manager
    manager.Save( dc.GetData() );

    // Change the value of a Datum
    dc["Col1"]->SetValue( 1 );

    // Load from the store, and show that Col1 goes back to original value
    manager.Load( dc.GetData() );
    cout << "Loaded value of Col1: " << dc["Col1"]->extract< int >() << endl;


    // Crunchstore can handle vector variables. They
    // can't be nested, and they have to be vectors, not lists or maps, etc.
    std::vector< double > dv;
    dv.push_back( 1.1 );
    dv.push_back( 2.2 );
    dv.push_back( 3.3 );

    DC dc2( 555, 3.1415, "Another String", false, dv );
    manager.Save( dc2.GetData() );
    dv.clear();
    dc2["Col5"]->SetValue( dv );
    manager.Load( dc2.GetData() );
    dv = dc2["Col5"]->extract< std::vector< double > >();
    for( size_t c = 0; c < dv.size(); ++c )
    {
        cout << dv.at( c ) << endl;
    }

    // Serialiation to JSON is also supported via the
    // crunchstore::Serialize::ToJSON static method:
    cout << Serialize::ToJSON( dc2.GetData() ) << endl;

    std::ifstream fs( "data.json" );
    std::stringstream json;
    json << fs.rdbuf();
    // Likewise for De-serialization:
    Serialize::FromJSON( dc2.GetData(), json.str() );

    cout << "De-serialized:" << endl;
    cout << Serialize::ToJSON( dc2.GetData() ) << endl;

    return 0;
}
