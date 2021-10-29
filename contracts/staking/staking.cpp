#include <koinos/system/system_calls.hpp>
#include <koinos/token.hpp>

#include <koinos/buffer.hpp>
#include <koinos/common.h>

#include <staking.h>

using namespace koinos;
using namespace koinos::contracts;
using namespace std::string_literals;

namespace constants {

constexpr std::size_t max_address_size = 25;

// 0x005b1e61d37259b9c2d99bf417f592e0b77725165d2488be45
const std::string koin_contract = "\x00\x5b\x1e\x61\xd3\x72\x59\xb9\xc2\xd9\x9b\xf4\x17\xf5\x92\xe0\xb7\x77\x25\x16\x5d\x24\x88\xbe\x45"s;
const auto contract_id          = system::get_contract_id();

} // constants

namespace state {

system::object_space contract_space()
{
   system::object_space obj_space;
   auto contract_id = constants::contract_id;
   obj_space.mutable_zone().set( reinterpret_cast< const uint8_t* >( contract_id.data() ), contract_id.size() );
   obj_space.set_id( 0 );
   return obj_space;
}

} // state

enum entries : uint32_t
{
   stake_entry      = 1,
   withdraw_entry   = 2,
   balance_of_entry = 3,
};

using stake_arguments      = staking::stake_arguments< constants::max_address_size >;
using stake_result         = staking::stake_result;
using withdraw_arguments   = stake_arguments;
using withdraw_result      = stake_result;
using balance_of_arguments = staking::balance_of_arguments< constants::max_address_size >;
using balance_of_result    = staking::balance_of_result;
using balance_object       = staking::balance_object;

stake_result stake( const stake_arguments& args )
{
   stake_result res;
   res.set_value( false );

   std::string account( reinterpret_cast< const char* >( args.get_account().get_const() ), args.get_account().get_length() );

   auto koin_token = koinos::token( constants::koin_contract );

   if ( !koin_token.transfer( account, constants::contract_id, args.value() ) )
   {
      system::print( "Transfer KOIN from 'account' failed\n" );
      return res;
   }

   auto contract_space = state::contract_space();
   balance_object bal;
   system::get_object( contract_space, account, bal );

   bal.set_value( bal.value() + args.value() );

   system::put_object( contract_space, account, bal );

   res.set_value( true );

   return res;
}

withdraw_result withdraw( const withdraw_arguments& args )
{
   withdraw_result res;
   res.set_value( false );

   std::string account( reinterpret_cast< const char* >( args.get_account().get_const() ), args.get_account().get_length() );

   auto contract_space = state::contract_space();
   balance_object bal;
   system::get_object( contract_space, account, bal );

   if ( bal.value() < args.value() )
   {
      system::print( "'account' has insufficient balance\n" );
      return res;
   }

   auto koin_token = koinos::token( constants::koin_contract );
   if ( !koin_token.transfer( constants::contract_id, account, args.value() ) )
   {
      system::print( "Contract had insufficient funds for withdraw ¯\\_(ツ)_/¯\n" );
      return res;
   }

   bal.set_value( bal.value() - args.value() );
   system::put_object( contract_space, account, bal );

   res.set_value( true );
   return res;
}

balance_of_result balance_of( const balance_of_arguments& args )
{
   balance_of_result res;

   std::string account( reinterpret_cast< const char* >( args.get_account().get_const() ), args.get_account().get_length() );

   balance_object bal;
   system::get_object( state::contract_space(), account, bal );

   res.set_value( bal.value() );
   return res;
}


int main()
{
   auto entry_point = system::get_entry_point();
   auto args = system::get_contract_arguments();

   std::array< uint8_t, 32 > retbuf;

   koinos::read_buffer rdbuf( (uint8_t*)args.c_str(), args.size() );
   koinos::write_buffer buffer( retbuf.data(), retbuf.size() );

   switch( entry_point )
   {
      case entries::stake_entry:
      {
         stake_arguments args;
         args.deserialize( rdbuf );

         auto res = stake( args );
         res.serialize( buffer );
         break;
      }
      case entries::withdraw_entry:
      {
         withdraw_arguments args;
         args.deserialize( rdbuf );

         auto res = withdraw( args );
         res.serialize( buffer );
         break;
      }
      case entries::balance_of_entry:
      {
         balance_of_arguments args;
         args.deserialize( rdbuf );

         auto res = balance_of( args );
         res.serialize( buffer );
         break;
      }
      default:
         system::print( "unknown entry point\n" );
         system::exit_contract( 1 );
   }

   std::string retval( reinterpret_cast< const char* >( buffer.data() ), buffer.get_size() );
   system::set_contract_result_bytes( retval );

   system::exit_contract( 0 );
   return 0;
}
