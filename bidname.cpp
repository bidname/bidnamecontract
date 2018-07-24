#include "bidname.hpp"

using namespace std;
using eosio::action;
using eosio::permission_level;
using std::vector;

//@abi action
void bidname::createorder(account_name seller, account_name acc, uint64_t price, asset adfee ){
    require_auth(seller);
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    eosio_assert( adfee.symbol == CORE_SYMBOL, "only core token allowed" );
    eosio_assert( adfee.is_valid(), "invalid adfee" );
    eosio_assert( adfee.amount > 1.0000, "must adfee positive quantity" );
    eosio_assert( is_account( acc ), "account don't exists" );
    action act(
        permission_level{seller, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(seller, _self, adfee, std::string("")));
    act.send();

    openorders.emplace(_self, [&](auto &order) {
        order.id = openorders.available_primary_key();
        order.seller = seller;
        order.acc = acc;
        order.price = price;
        order.adfee = adfee.amount;
        order.createdat = now();
    });
}

bool bidname::ismaintained()
{
        return false;
}

//@abi action
void bidname::cancelorder(uint64_t orderId,account_name acc,account_name seller){

}

//@abi action
void bidname::placeorder(account_name account,uint64_t orderId,account_name buyer){

}

//@abi action
void bidname::accrelease(account_name seller, account_name acc, account_name buyer,uint64_t orderid){

}

//@abi action
void bidname::setadfee(uint64_t orderid, account_name seller, account_name acc){

}

//@abi action
void bidname::setmaintain(bool maintain){
    globalsets.emplace(_self, [&](auto &globalset) {
        globalset.id = globalsets.available_primary_key();
        globalset.maintained = maintain;
    });
}

//@abi action
void bidname::setroyalty(double royalty){

}

//@abi action
void bidname::setreward(uint64_t reward){

}

EOSIO_ABI(bidname, (createorder)(cancelorder)(placeorder)(accrelease)(setadfee)(setmaintain)(setroyalty)(setreward))
