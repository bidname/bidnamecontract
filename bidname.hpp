#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::key256;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using std::vector;

class bidname : public eosio::contract{
  private:


    typedef enum order_status
    {
        LOCKING = 0,        // 锁定状态，不允许下单
        OPEN,            // 正在进行
    } order_status;

    //@abi table globalsets i64
    struct globalset
    {
        uint64_t id = 0;                        // 主键，使用available_primary_key生成
        bool maintained = false;                // 是否处于系统维护状态
        double royalty;
        uint64_t reward;

        uint64_t primary_key()const { return id; }

        EOSLIB_SERIALIZE( globalset, (id)(maintained)(royalty)(reward) )
    };

    //@abi table openorders i64
    struct openorder
    {
        uint64_t id;
        account_name seller;
        account_name acc;
        double adfee;
        account_name buyer;
        public_key newpkey;
        uint64_t price;
        uint16_t status;
        time createdat;

        uint64_t primary_key() const { return id; }
        account_name by_seller() const { return seller; }
        account_name by_acc() const { return acc; }
        double by_adfee() const {return adfee;}

        EOSLIB_SERIALIZE( openorder, (id)(seller)(acc)(adfee)(buyer)(newpkey)(price)(status)(createdat) )
    };

    //@abi table comporders i64
    struct comporder
    {
        uint64_t id;
        account_name seller;
        account_name acc;
        account_name buyer;
        asset price;
        time createdat;
        time finishedat;

        uint64_t primary_key() const { return id; }
        account_name by_seller() const { return seller; }
        account_name by_acc() const { return acc; }
        asset by_price() const {return price;}

        EOSLIB_SERIALIZE( comporder, (id)(seller)(acc)(buyer)(price)(createdat)(finishedat) )
    };

  private: 
    typedef eosio::multi_index<N(globalsets), globalset> global_set_index;

    typedef eosio::multi_index<N(openorders), openorder,
                                indexed_by<N(seller), const_mem_fun<openorder, account_name, &openorder::by_seller>>,
                                indexed_by<N(acc), const_mem_fun<openorder, account_name, &openorder::by_acc>>,
                                indexed_by<N(adfee), const_mem_fun<openorder, double, &openorder::by_adfee>>>
        open_orders_index;

    typedef eosio::multi_index<N(comporders), comporder,
                                indexed_by<N(seller), const_mem_fun<comporder, account_name, &comporder::by_seller>>,
                                indexed_by<N(acc), const_mem_fun<comporder, account_name, &comporder::by_acc>>,
                                indexed_by<N(price), const_mem_fun<comporder, asset, &comporder::by_price>>>
        comp_orders_index;

    global_set_index globalsets;
    open_orders_index openorders;
    comp_orders_index comporders;

  public:
    bidname(account_name self)
        : eosio::contract(self),
          globalsets(_self, _self),
          openorders(_self, _self),
          comporders(_self, _self)
    {
    }

    void createorder(const account_name seller,account_name acc, uint64_t price, asset adfee  );
    void cancelorder(uint64_t orderid,account_name acc,account_name seller);
    void placeorder(account_name acc,uint64_t orderid,account_name buyer,public_key newpkey);
    void accrelease(account_name seller, account_name acc, account_name buyer,uint64_t orderid);
    void setadfee(uint64_t orderid, account_name seller, account_name acc); 
    void setmaintain(bool maintain);
    void setroyalty(double royalty);  
    void setreward(uint64_t reward);

  private:
    void setorderstatus(uint64_t orderid);                                                                                 // 修改订单状态
    bool isorderopen(uint64_t orderid);                                                                                     // 订单是否开启
    asset getorderprice(uint64_t orderid);                                                                                 // 获取订单金额
    void deleteoldorder(account_name account);                                                                                         // 该账户是否已存在open的单                          
    void ordercommission(account_name client, asset fee);                                                              // 缴纳广告费及佣金
    void reward(account_name buyer, account_name seller);                                                                                // 给予购买者代币奖励
    void canceloldorder(uint64_t orderid);
    bool ismaintained();   
    uint64_t getreward();
    double getroyalty();

};