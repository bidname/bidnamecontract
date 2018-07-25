#include "bidname.hpp"

using namespace std;
using eosio::action;
using eosio::permission_level;
using std::vector;
using eosio::print;
using eosio::name;

//@abi action
void bidname::createorder(const account_name seller,account_name acc, uint64_t price, asset adfee ){
    require_auth(seller);
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    eosio_assert( adfee.symbol == CORE_SYMBOL, "only core token allowed" );
    eosio_assert( adfee.is_valid(), "invalid adfee" );
    eosio_assert( adfee.amount > 1.0000, "must adfee positive quantity" );
    eosio_assert( is_account( acc ), "account don't exists" );

    deleteoldorder(acc);
    
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
        order.status = OPEN;
    });
}

void bidname::deleteoldorder(account_name account){
    auto acc_index = openorders.template get_index<N(acc)>();

    auto acc_itr = acc_index.find(name{account});
    if(acc_itr != acc_index.end()){
        eosio_assert(acc_itr->status == OPEN, "account order is lock" );
        acc_index.erase(acc_itr);
    }
}

bool bidname::ismaintained()
{
        return false;
}

//@abi action
void bidname::cancelorder(uint64_t orderid,account_name acc,account_name seller){
    require_auth(seller);
    auto acc_itr = openorders.find(orderid);
   
    eosio_assert(acc_itr != openorders.end(), "don't find the order");
    eosio_assert(acc_itr->status == OPEN, "order is locking");
    eosio_assert(name{acc_itr->seller} == name{seller}, "order info is wrong");
    eosio_assert(name{acc_itr->acc} == name{acc}, "order info is wrong");
    openorders.erase(acc_itr);
    eosio_assert(acc_itr != openorders.end(), "can't cancel order");
}

//@abi action
void bidname::placeorder(account_name acc,uint64_t orderId,account_name buyer,public_key newpkey){

}

//@abi action
void bidname::accrelease(account_name seller, account_name acc, account_name buyer,uint64_t orderid){

}

//@abi action
void bidname::setadfee(uint64_t orderid, account_name seller, account_name acc){

}

//@abi action
void bidname::setmaintain(bool maintain){
    require_auth(_self);
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
