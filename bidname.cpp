#include "bidname.hpp"

using namespace std;
using eosio::action;
using eosio::permission_level;
using std::vector;

//@abi action
void bidname::createorder(name seller, name acc, uint64_t price, asset adfee ){

}
//@abi action
void bidname::cancelorder(uint64_t orderId,name acc,name seller){

}

//@abi action
void bidname::placeorder(name account,uint64_t orderId,name buyer){

}

//@abi action
void bidname::accrelease(name seller, name acc, name buyer,uint64_t orderid){

}

//@abi action
void bidname::setadfee(uint64_t orderid, name seller, name acc){

}

EOSIO_ABI(bidname, (createorder)(cancelorder)(placeorder)(accrelease)(setadfee))
