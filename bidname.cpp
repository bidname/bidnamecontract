#include "bidname.hpp"

using namespace std;

using eosio::action;
using eosio::permission_level;
using std::vector;
using eosio::print;
using eosio::name;

//@abi action
void bidname::createorder(const account_name seller,account_name acc, asset price, asset adfee ){
    require_auth2(acc,N(owner));
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    eosio_assert( price.symbol == CORE_SYMBOL, "only core token allowed" );
    eosio_assert( price.is_valid(), "invalid price" );
    eosio_assert( price.amount > 1, "must price positive quantity" );
    eosio_assert( adfee.symbol == CORE_SYMBOL, "only core token allowed" );
    eosio_assert( adfee.is_valid(), "invalid adfee" );
    eosio_assert( adfee.amount > 1, "must adfee positive quantity" );
    eosio_assert( is_account( acc ), "account don't exists" );
    eosio_assert( acc != seller, "don't sell yourself" );

    isaccountvalid(seller,acc);
    deleteoldorder(acc);
    
    ordercommission(acc,N(owner),adfee);

    openorders.emplace(acc, [&](auto &order) {
        order.id = openorders.available_primary_key();
        order.seller = seller;
        order.acc = acc;
        order.price = price;
        order.adfee = adfee;
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
    auto global_itr = globalsets.begin();
    if ( global_itr == globalsets.end() ) 
    {
        return true;
    }
    return global_itr->maintained;
}

//@abi action
void bidname::cancelorder(uint64_t orderid,account_name acc,account_name seller){
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    require_auth2(acc,N(owner));
    auto acc_itr = openorders.find(orderid);
   
    eosio_assert(acc_itr != openorders.end(), "don't find the order");
    // eosio_assert(acc_itr->status == OPEN, "order is locking");
    eosio_assert(name{acc_itr->seller} == name{seller}, "order info is wrong");
    eosio_assert(name{acc_itr->acc} == name{acc}, "order info is wrong");
    if(acc_itr->status == LOCKING && acc_itr->buyer != 0){
        action transferact(
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
            std::make_tuple(_self, acc_itr->buyer, acc_itr->price, std::string("")));
        transferact.send();
    }
    openorders.erase(acc_itr);
    eosio_assert(acc_itr != openorders.end(), "can't cancel order");
}

//@abi action
void bidname::placeorder(account_name acc,uint64_t orderid,account_name buyer,eosio::public_key newpkey){
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    require_auth2(buyer,N(active));
    auto order_itr = openorders.find(orderid);
    eosio_assert(order_itr != openorders.end(), "don't find the order");
    eosio_assert(order_itr->status == OPEN, "order is locking");
    eosio_assert(name{order_itr->acc} == name{acc}, "order info is wrong");

    action transferact(
        permission_level{buyer, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(buyer, _self, order_itr->price, std::string("")));
    transferact.send();
    openorders.modify(order_itr,buyer, [&]( auto& order ) {
        order.buyer = buyer;
        order.newpkey = newpkey;
        order.status = LOCKING;
        order.placeat = now();
      });
}

//@abi action
void bidname::accrelease(account_name seller, account_name acc, account_name buyer,uint64_t orderid){
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    require_auth2(acc,N(owner));

    auto order_itr = openorders.find(orderid);
    eosio_assert(order_itr != openorders.end(), "don't find the order");
    eosio_assert(order_itr->status == LOCKING, "order is not locking");
    eosio_assert(name{order_itr->acc} == name{acc}, "order info is wrong");
    eosio_assert(name{order_itr->seller} == name{seller}, "order info is wrong");
    eosio_assert(name{order_itr->buyer} == name{buyer}, "order info is wrong");
    double royalty = getroyalty();
    asset poundage = asset(order_itr->price.amount*royalty,order_itr->price.symbol);
    
    action transferact(
        permission_level{_self, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(_self, seller, order_itr->price - poundage, std::string("")));
    transferact.send();
    ordercommission(_self,N(active),poundage);

    bidname::key_weight keyweight = { .key = order_itr->newpkey,  .weight = 1 };
    bidname::authority ownerkey  = { .threshold = 1, .keys = { keyweight }, .accounts = {}, .waits = {} };
    print("i m here i right2");

    action updateauthact(
        permission_level{acc, N(owner)},
        N(eosio), N(updateauth),
        std::make_tuple(acc, N(owner), N(),ownerkey));
    updateauthact.send();
    
    comporders.emplace(_self, [&](auto &order) {
        order.id = openorders.available_primary_key();
        order.seller = seller;
        order.acc = acc;
        order.price = order_itr->price;
        order.buyer = buyer;
        order.createdat = order_itr->createdat;
        order.finishedat = now();
    });

    openorders.erase(order_itr);
    // reward(seller,buyer);
    
}

//@abi action
void bidname::setadfee(uint64_t orderid, account_name seller, account_name acc, asset adfee){
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    require_auth2(acc,N(owner));
    auto order_itr = openorders.find(orderid);
    eosio_assert(order_itr != openorders.end(), "don't find the order");
    eosio_assert(order_itr->status == OPEN, "order is locking");
    eosio_assert(name{order_itr->seller} == name{seller}, "order info is wrong");
    eosio_assert(name{order_itr->acc} == name{acc}, "order info is wrong");
    eosio_assert(adfee.amount > 0, "adfee is wrong");
    eosio_assert(adfee.symbol == CORE_SYMBOL, "only core token allowed" );
    ordercommission(acc, N(owner), adfee);
    openorders.modify(order_itr,acc,[&]( auto& order){
        order.adfee += adfee;
    });
}

//@abi action
void bidname::cancelplace(account_name acc,uint64_t orderid,account_name buyer,account_name seller){
    eosio_assert(ismaintained() == false, "The game is under maintenance");
    require_auth2(buyer,N(active));
    auto order_itr = openorders.find(orderid);
    eosio_assert(order_itr != openorders.end(), "don't find the order");
    eosio_assert(order_itr->status == LOCKING, "order is unlocking");
    eosio_assert(name{order_itr->seller} == name{seller}, "order info is wrong");
    eosio_assert(name{order_itr->acc} == name{acc}, "order info is wrong");
    eosio_assert(name{order_itr->buyer} == name{buyer}, "order info is wrong");
    double royalty = getroyalty();
    if((now()-order_itr->placeat)>86400){
        action transferact(
            permission_level{_self, N(active)},
            N(eosio.token), N(transfer),
            std::make_tuple(_self, buyer, order_itr->price, std::string("")));
        transferact.send();
    }else{
    asset returnprice = asset(order_itr->price.amount*royalty,order_itr->price.symbol);
    action transferact(
        permission_level{_self, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(_self, buyer, order_itr->price - returnprice, std::string("")));
    transferact.send();

    ordercommission(_self,N(active),returnprice);
    }
    
    openorders.modify(order_itr,buyer,[&]( auto& order){
        order.buyer = account_name();
        order.newpkey = eosio::public_key();
        order.status = OPEN;
        order.placeat = 0;
    });
}

//@abi action
void bidname::setglobalcfg(bool maintain, double royalty, asset reward, account_name contractname,account_name receiver){
    require_auth(_self);
    eosio_assert(is_account(contractname) , "account is invalid");
    eosio_assert(is_account(receiver) , "account is invalid");

    eosio_assert(royalty>0 , "royalty have to bigger than zero");
    eosio_assert(reward.amount>0 , "reward have to bigger than zero");
   
    auto global_itr = globalsets.begin();
    if(global_itr == globalsets.end()){
        globalsets.emplace(_self, [&](auto &globalset) {
        globalset.id = globalsets.available_primary_key();
        globalset.maintained = maintain;
        globalset.royalty = royalty;
        globalset.reward = reward;
        globalset.contractname = contractname;
        globalset.receiver = receiver;
        });
    }else{
        globalsets.modify(global_itr,_self, [&]( auto& globalset ) {
        globalset.maintained = maintain;
        globalset.royalty = royalty;
        globalset.reward = reward;
        globalset.contractname = contractname;
        globalset.receiver = receiver;

        });
    }
}

void bidname::isaccountvalid(account_name seller,account_name acc){
    auto acc_index = openorders.template get_index<N(acc)>();
    auto seller_itr = acc_index.find(seller);
    eosio_assert(seller_itr==acc_index.end(),"seller was traded in order table .");
    auto seller_index = openorders.template get_index<N(seller)>();
    auto acc_itr = seller_index.find(acc);
    eosio_assert(acc_itr==seller_index.end(),"account was a seller in order table.");
}

void bidname::ordercommission(account_name client,permission_name permission, asset fee){
    account_name receiver = getreceiver();
    action transferact(
        permission_level{client, permission},
        N(eosio.token), N(transfer),
        std::make_tuple(client, receiver, fee, std::string("")));
    transferact.send();
}

double bidname::getroyalty(){
    auto global_itr = globalsets.begin();
    if(global_itr == globalsets.end()){
        return 0.1;
    }else{
        return global_itr->royalty;
    }
} 

asset bidname::getreward(){
    auto global_itr = globalsets.begin();
    if(global_itr == globalsets.end()){
        return asset(800,N(BID));
    }else{
        return global_itr->reward;
    }
} 

account_name bidname::getcontractname(){
     auto global_itr = globalsets.begin();
    if(global_itr == globalsets.end()){
        return account_name();
    }else{
        return global_itr->contractname;
    }
}

account_name bidname::getreceiver(){
     auto global_itr = globalsets.begin();
    if(global_itr == globalsets.end()){
        return _self;
    }else{
        return global_itr->receiver;
    }
}

void bidname::reward(account_name buyer, account_name seller){
    asset curreward = getreward();
    account_name curcontract= getcontractname();
    action transferb(
        permission_level{_self, N(active)},
        curcontract, N(transfer),
        std::make_tuple(_self, seller, curreward, std::string("")));
    transferb.send();
    action transfers(
        permission_level{_self, N(active)},
        curcontract, N(transfer),
        std::make_tuple(_self, buyer, curreward, std::string("")));
    transfers.send();
}

EOSIO_ABI(bidname, (createorder)(cancelorder)(placeorder)(accrelease)(setadfee)(cancelplace)(setglobalcfg))
