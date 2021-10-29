#include <koinos/system/system_calls.hpp>

#include <koinos/buffer.hpp>
#include <koinos/common.h>

#include <calc.h>

using namespace koinos;
using namespace koinos::contracts;

enum entries : uint32_t
{
   add_entry = 1,
   sub_entry = 2,
   mul_entry = 3,
   div_entry = 4
};

class calculator
{
public:
   calc::add_result add( int64_t x, int64_t y ) noexcept;
   calc::sub_result sub( int64_t x, int64_t y ) noexcept;
   calc::mul_result mul( int64_t x, int64_t y ) noexcept;
   calc::div_result div( int64_t x, int64_t y ) noexcept;
};

calc::add_result calculator::add( int64_t x, int64_t y ) noexcept
{
   calc::add_result res;
   res.set_value( x + y );
   return res;
}

calc::sub_result calculator::sub( int64_t x, int64_t y ) noexcept
{
   calc::sub_result res;
   res.set_value( x - y );
   return res;
}

calc::mul_result calculator::mul( int64_t x, int64_t y ) noexcept
{
   calc::mul_result res;
   res.set_value( x * y );
   return res;
}

calc::div_result calculator::div( int64_t x, int64_t y ) noexcept
{
   calc::div_result res;

   if ( y == 0 )
   {
      system::print( "cannot divide by zero" );
      system::exit_contract( 1 );
   }

   res.set_value( x / y );
   return res;
}

int main()
{
   auto entry_point = system::get_entry_point();
   auto args = system::get_contract_arguments();

   std::array< uint8_t, 32 > retbuf;

   koinos::read_buffer rdbuf( (uint8_t*)args.c_str(), args.size() );
   koinos::write_buffer buffer( retbuf.data(), retbuf.size() );

   calculator c;

   switch( entry_point )
   {
      case entries::add_entry:
      {
         calc::add_arguments args;
         args.deserialize( rdbuf );

         auto res = c.add( args.x(), args.y() );
         res.serialize( buffer );
         break;
      }
      case entries::sub_entry:
      {
         calc::sub_arguments args;
         args.deserialize( rdbuf );

         auto res = c.sub( args.x(), args.y() );
         res.serialize( buffer );
         break;
      }
      case entries::mul_entry:
      {
         calc::mul_arguments args;
         args.deserialize( rdbuf );

         auto res = c.mul( args.x(), args.y() );
         res.serialize( buffer );
         break;
      }
      case entries::div_entry:
      {
         calc::div_arguments args;
         args.deserialize( rdbuf );

         auto res = c.div( args.x(), args.y() );
         res.serialize( buffer );
         break;
      }
      default:
         system::exit_contract( 1 );
   }

   std::string retval( reinterpret_cast< const char* >( buffer.data() ), buffer.get_size() );
   system::set_contract_result_bytes( retval );

   system::exit_contract( 0 );
   return 0;
}

