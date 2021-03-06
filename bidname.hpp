#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/public_key.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/asset.hpp>
#include <string>

using namespace eosio;

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::key256;
using eosio::name;
using eosio::permission_level;
using eosio::print;
using std::vector;
// using eosio::public_key;

class bidname : public eosio::contract{
  private:
    //模拟systemauthority
    struct key_weight 
    {
      eosio::public_key   key;
      weight_type  weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
    };
    //模拟systemauthority
    struct permission_level_weight
    {
      permission_level  permission ;
      weight_type       weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };
    //模拟systemauthority
    struct wait_weight 
    {
    uint32_t wait_sec;
    weight_type weight;
    };
    //模拟systemauthority
    struct authority   
    {
    uint32_t threshold;
    vector<key_weight> keys;
    vector<permission_level_weight> accounts;
    vector<wait_weight> waits;
    };

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
        double royalty;                         // 手续费比例
        asset reward;                           // 奖励标准
        account_name contractname;              // 代币合约地址
        account_name receiver;                  // 接受手续费账户

        uint64_t primary_key()const { return id; }

        EOSLIB_SERIALIZE( globalset, (id)(maintained)(royalty)(reward)(contractname)(receiver) )
    };

    //@abi table openorders i64
    struct openorder
    {
        uint64_t id;
        account_name seller;
        account_name acc;
        asset adfee;
        account_name buyer;
        eosio::public_key newpkey;
        asset price;
        uint16_t status;
        time createdat;
        time placeat;

        uint64_t primary_key() const { return id; }
        account_name by_seller() const { return seller; }
        account_name by_acc() const { return acc; }
        uint64_t by_adfee() const {return adfee.amount;}

        EOSLIB_SERIALIZE( openorder, (id)(seller)(acc)(adfee)(buyer)(newpkey)(price)(status)(createdat)(placeat) )
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
        uint64_t by_price() const {return price.amount;}

        EOSLIB_SERIALIZE( comporder, (id)(seller)(acc)(buyer)(price)(createdat)(finishedat) )
    };

  private: 
    typedef eosio::multi_index<N(globalsets), globalset> global_set_index;

    typedef eosio::multi_index<N(openorders), openorder,
                                indexed_by<N(seller), const_mem_fun<openorder, account_name, &openorder::by_seller>>,
                                indexed_by<N(acc), const_mem_fun<openorder, account_name, &openorder::by_acc>>,
                                indexed_by<N(adfee), const_mem_fun<openorder, uint64_t, &openorder::by_adfee>>>
        open_orders_index;

    typedef eosio::multi_index<N(comporders), comporder,
                                indexed_by<N(seller), const_mem_fun<comporder, account_name, &comporder::by_seller>>,
                                indexed_by<N(acc), const_mem_fun<comporder, account_name, &comporder::by_acc>>,
                                indexed_by<N(price), const_mem_fun<comporder, uint64_t, &comporder::by_price>>>
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

    void createorder(const account_name seller,account_name acc, asset price, asset adfee  );
    void cancelorder(account_name acc,account_name seller);
    void placeorder(account_name acc,account_name buyer,eosio::public_key newpkey);
    void accrelease(account_name seller, account_name acc, account_name buyer);
    void setadfee(account_name seller, account_name acc, asset adfee); 
    void setglobalcfg(bool maintain, double royalty, asset reward, account_name contractname,account_name receiver);
    void cancelplace(account_name acc,account_name buyer,account_name seller);

  private:
    void setorderstatus(uint64_t orderid);                                                 // 修改订单状态
    bool isorderopen(uint64_t orderid);                                                    // 订单是否开启
    asset getorderprice(uint64_t orderid);                                                 // 获取订单金额
    void isaccountvalid(account_name seller,account_name acc);                             //判断seller和acc是否冲突
    void deleteoldorder(account_name account);                                             // 删除acc重复的open订单                          
    void ordercommission(account_name client, permission_name permission, asset fee);      // 缴纳广告费及佣金
    void reward(account_name buyer, account_name seller);                                  // 给予购买者代币奖励
    bool ismaintained();                                                                   // 是否在维护                            
    asset getreward();                                                                     // 获取代币奖励标准
    double getroyalty();                                                                   // 获取手续费比例
    account_name getcontractname();                                                        // 获取代币账户
    account_name getreceiver();                                                            // 获取团队收益账户
};