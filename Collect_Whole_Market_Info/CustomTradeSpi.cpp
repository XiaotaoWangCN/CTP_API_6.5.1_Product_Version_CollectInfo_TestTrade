﻿#include <iostream>
#include <time.h>
#include <thread>
#include <chrono>
#include<cstring>
#include "GBK2UTF8.h"
#include<vector>
#include "CustomTradeSpi.h"
#include "CTP_API/ThostFtdcMdApi.h"
using namespace std;


std::vector<std::string> CustomTradeSpi::instruments;

// ---- 全局参数声明 ---- //
extern TThostFtdcBrokerIDType gBrokerID;                      // 经纪商代码
extern TThostFtdcUserIDType gInvesterID;                  // 账户名
extern TThostFtdcPasswordType gInvesterPassword;              // 密码
extern TThostFtdcAppIDType gInvesterAppID ;
extern TThostFtdcAuthCodeType gInvesterAuthCode ;

extern CThostFtdcTraderApi *g_pTradeUserApi;                  // 交易指针

extern char gTradeFrontAddr[];                                // 前置地址
extern TThostFtdcInstrumentIDType g_pTradeInstrumentID;       // 合约代码
extern TThostFtdcDirectionType gTradeDirection;               // 方向
extern TThostFtdcPriceType gLimitPrice;                       // 价格


extern CThostFtdcMdApi *g_pMdUserApi;
// 会话参数
TThostFtdcFrontIDType	trade_front_id;	//前置编号
TThostFtdcSessionIDType	session_id;	//会话编号
TThostFtdcOrderRefType	order_ref;	//报单引用
time_t lOrderTime;
time_t lOrderOkTime;


void CustomTradeSpi::OnFrontConnected()
{
    std::cout << "=====Trade==建立网络连接成功=====" << std::endl;
    print_time();
    CThostFtdcReqAuthenticateField authfield;
    memset(&authfield, 0, sizeof(authfield));
    strcpy(authfield.BrokerID, gBrokerID);
    strcpy(authfield.UserID, gInvesterID);
    strcpy(authfield.AppID, gInvesterAppID);
    strcpy(authfield.AuthCode, gInvesterAuthCode);
    int rt = g_pTradeUserApi->ReqAuthenticate(&authfield, 0);
}

void CustomTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====Trade==认证成功=====" << std::endl;
        print_time();
        loginFlag = true;
        std::cout<<"User Product Info: "<<gbk2utf8(pRspAuthenticateField->UserProductInfo)<<std::endl;
        std::cout<<"UserID: "<<gbk2utf8(pRspAuthenticateField->UserID)<<std::endl;
        std::cout<<"BrokerID: "<<gbk2utf8(pRspAuthenticateField->BrokerID)<<std::endl;
        std::cout<<"AppID: "<<gbk2utf8(pRspAuthenticateField->AppID)<<std::endl;
        //std::cout<<"AppType: "<<gbk2utf8(pRspAuthenticateField->AppType)<<std::endl;
        // 开始登录
        reqUserLogin();
    }
}
#if 1
void CustomTradeSpi::OnRspUserLogin(
        CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====Trade==登录成功=====" << std::endl;
        print_time();
        loginFlag = true;
        std::cout << "TradingDay:"<< gbk2utf8(pRspUserLogin->TradingDay) << std::endl;
        std::cout << "LoginTime:"<< gbk2utf8(pRspUserLogin->LoginTime) << std::endl;
        std::cout <<  "BrokerID:"<<gbk2utf8(pRspUserLogin->BrokerID) << std::endl;
        std::cout <<   "UserID:"<<gbk2utf8(pRspUserLogin->UserID) << std::endl;

/*
        std::cout <<  gbk2utf8(pRspUserLogin->TradingDay) << std::endl;
        std::cout <<  gbk2utf8(pRspUserLogin->LoginTime) << std::endl;
        std::cout <<  gbk2utf8(pRspUserLogin->BrokerID) << std::endl;
        std::cout <<  gbk2utf8(pRspUserLogin->UserID) << std::endl;
*/
        // 保存会话参数
        trade_front_id = pRspUserLogin->FrontID;
        session_id = pRspUserLogin->SessionID;
        strcpy(order_ref, pRspUserLogin->MaxOrderRef);

        // 结算结果确认
        reqSettlementInfoConfirm();
    }

}

void CustomTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

    isErrorRspInfo(pRspInfo);

}

void CustomTradeSpi::OnFrontDisconnected(int nReason)
{

    std::cerr << "=====Trade==网络连接断开=====" << std::endl;
    print_time();
    std::cerr << "错误码： " << nReason << std::endl;
}

void CustomTradeSpi::OnHeartBeatWarning(int nTimeLapse)
{

    std::cerr << "=====Trade==网络心跳超时=====" << std::endl;
    print_time();
    std::cerr << "距上次连接时间： " << nTimeLapse << std::endl;
}

void CustomTradeSpi::OnRspUserLogout(
        CThostFtdcUserLogoutField *pUserLogout,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        loginFlag = false; // 登出
        std::cout << "=====Trade==登出成功=====" << std::endl;
        print_time();
        std::cout << "经纪商： " << pUserLogout->BrokerID << std::endl;
        std::cout << "帐户名： " << pUserLogout->UserID << std::endl;
    }
}

void CustomTradeSpi::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{
    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====Trade==结算确认=====" << std::endl;
        print_time();
        std::cout <<pSettlementInfoConfirm->ConfirmDate << std::endl;
        std::cout <<  pSettlementInfoConfirm->ConfirmTime << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        // 查询资金
        reqQueryTradingAccount();
    }

}

void CustomTradeSpi::OnRspQryInstrument(
        CThostFtdcInstrumentField *pInstrument,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    //if (!isErrorRspInfo(pRspInfo))
    {
        instruments.push_back(gbk2utf8(pInstrument->InstrumentID));
        //instruments.push_back(pInstrument->InstrumentID);
        //std::cout << "交易所代码： " << pInstrument->ExchangeID << std::endl;
        //std::cout << "合约代码： " << pInstrument->InstrumentID << std::endl;
        //std::cout << "执行价： " << pInstrument->StrikePrice << std::endl;
        //std::cout << "到期日： " << pInstrument->EndDelivDate << std::endl;
        //std::cout << "当前状态： " << pInstrument->IsTrading << std::endl;
        if(bIsLast== true)
        {
            int size=instruments.size();
            std::cout<<"instruments.size="<<size<<std::endl;
            std::cout<<"===开始订阅行情==="<<std::endl;
            print_time();
            char* g_pInstrumentID[1];
            int rt=-1<<30;
            for (int i=0;i<size;++i)
            {
                //std::cout<<"i="<<i<<" "<<instruments[i]<<std::endl;
                g_pInstrumentID[0]= (char*)(instruments[i].c_str());
                rt = g_pMdUserApi->SubscribeMarketData(g_pInstrumentID, 1);
            }
            cout << "CustoTradeSpi.cpp g_pMdUserApi="<<g_pMdUserApi << endl;
            std::cout<<"行情指针订阅行情返回值="<<rt<<std::endl;
        }
    }
}
void CustomTradeSpi::OnRspQryTradingAccount(
        CThostFtdcTradingAccountField *pTradingAccount,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====查询资金成功=====" << std::endl;
        print_time();
        std::cout << "账号： " << pTradingAccount->AccountID << std::endl;
        std::cout << "可用： " << pTradingAccount->Available << std::endl;
        std::cout << "可取： " << pTradingAccount->WithdrawQuota << std::endl;
        std::cout << "当前: " << pTradingAccount->CurrMargin << std::endl;
        std::cout << "盈亏： " << pTradingAccount->CloseProfit << std::endl;
        // 请求查询持仓
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
        reqQueryInvestorPosition();
    }

}

void CustomTradeSpi::OnRspQryInvestorPosition(
        CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====查询持仓=====" << std::endl;
        print_time();
        if (pInvestorPosition)
        {
            std::cout << "合约代码： " << pInvestorPosition->InstrumentID << std::endl;
            std::cout << "价格： " << pInvestorPosition->OpenAmount << std::endl;
            std::cout << "数量： " << pInvestorPosition->OpenVolume << std::endl;
            std::cout << "方向： " << pInvestorPosition->PosiDirection << std::endl;
            std::cout << "占用保证金：" << pInvestorPosition->UseMargin << std::endl;
        }
        else
            std::cout << "----->该合约未持仓" << std::endl;
        if(bIsLast==true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            // 请求查询合约
            reqQueryInstrument();
            //reqQueryInvestorPositionDetail();
        }

    }

}


void CustomTradeSpi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
                                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====查询持仓明细成功=====" << std::endl;
        print_time();
        if (pInvestorPositionDetail)
        {
            std::cout << "合约代码： " << pInvestorPositionDetail->InstrumentID << std::endl;
            std::cout << "价格： " << pInvestorPositionDetail->OpenPrice << std::endl;
            std::cout << "交易所代码： " << pInvestorPositionDetail->ExchangeID << std::endl;
            std::cout << "昨结算价：" << pInvestorPositionDetail->LastSettlementPrice << std::endl;
            std::cout << "开仓日期：" << pInvestorPositionDetail->OpenDate << std::endl;
            std::cout << "保证金：" << pInvestorPositionDetail->Margin << std::endl;
            std::cout << "交易所保证金：" << pInvestorPositionDetail->ExchMargin << std::endl;
            std::cout << "逐日盯市：" << pInvestorPositionDetail->PositionProfitByDate << std::endl;
            std::cout << "逐笔对冲：" << pInvestorPositionDetail->PositionProfitByTrade << std::endl;
            std::cout << "方向：" << pInvestorPositionDetail->Direction << std::endl;
        }
        else
            std::cout << "----->该合约未持仓" << std::endl;
        if(bIsLast==true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            // 请求查询合约
            //reqQueryInstrument();
        }
        if(bIsLast== false)
        {
            std::cout<<"False"<<std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            // 请求查询合约
            //reqQueryInstrument();
        }
    }

}


void CustomTradeSpi::OnRspOrderInsert(
        CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====报单录入成功=====" << std::endl;
        std::cout << "合约代码： " << pInputOrder->InstrumentID << std::endl;
        std::cout << "价格： " << pInputOrder->LimitPrice << std::endl;
        std::cout << "数量： " << pInputOrder->VolumeTotalOriginal << std::endl;
        std::cout << "方向： " << pInputOrder->Direction << std::endl;
    }

}

void CustomTradeSpi::OnRspOrderAction(
        CThostFtdcInputOrderActionField *pInputOrderAction,
        CThostFtdcRspInfoField *pRspInfo,
        int nRequestID,
        bool bIsLast)
{

    if (!isErrorRspInfo(pRspInfo))
    {
        std::cout << "=====报单操作成功=====" << std::endl;
        std::cout << "合约代码： " << pInputOrderAction->InstrumentID << std::endl;
        std::cout << "操作标志： " << pInputOrderAction->ActionFlag;
    }
}

void CustomTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    char str[10];
    //std::cout<<"HERE"<<std::endl;
    sprintf(str, "%d", pOrder->OrderSubmitStatus);
    int orderState = atoi(str) - 48;	//报单状态0=已经提交，3=已经接受

    std::cout << "=====收到报单应答=====" << std::endl;

    if (isMyOrder(pOrder))
    {
        if (isTradingOrder(pOrder))
        {
            std::cout << "--->>> 等待中！" << std::endl;
            
        }
        else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
            std::cout << "--->>> 撤单成功！" << std::endl;
    }
}

void CustomTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    std::cout << "=====报单成功成交=====" << std::endl;
    std::cout << "时间： " << pTrade->TradeTime << std::endl;
    std::cout << "代码： " << pTrade->InstrumentID << std::endl;
    std::cout << "价格： " << pTrade->Price << std::endl;
    std::cout << "数量： " << pTrade->Volume << std::endl;
    std::cout << "方向： " << pTrade->Direction << std::endl;
}

bool CustomTradeSpi::isErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    bool bResult = pRspInfo && (pRspInfo->ErrorID != 0);
    if (bResult)
        std::cerr << "返回错误--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << gbk2utf8(pRspInfo->ErrorMsg) << std::endl;
    return bResult;
}

void CustomTradeSpi::reqUserLogin()
{
    CThostFtdcReqUserLoginField loginReq;
    memset(&loginReq, 0, sizeof(loginReq));
    strcpy(loginReq.BrokerID, gBrokerID);
    strcpy(loginReq.UserID, gInvesterID);
    strcpy(loginReq.Password, gInvesterPassword);
    static int requestID = 0; // 请求编号
    std::cout<<loginReq.BrokerID<<std::endl;
    std::cout<<loginReq.UserID<<std::endl;
    std::cout<<loginReq.Password<<std::endl;
    int rt = g_pTradeUserApi->ReqUserLogin(&loginReq, requestID);
    if (!rt)
        std::cout << ">>>>>>发送登录请求成功" << std::endl;
    else
        std::cerr << "--->>>发送登录请求失败" << std::endl;
}

void CustomTradeSpi::reqUserLogout()
{
    CThostFtdcUserLogoutField logoutReq;
    memset(&logoutReq, 0, sizeof(logoutReq));
    strcpy(logoutReq.BrokerID, gBrokerID);
    strcpy(logoutReq.UserID, gInvesterID);
    static int requestID = 0; // 请求编号
    int rt = g_pTradeUserApi->ReqUserLogout(&logoutReq, requestID);
    if (!rt)
        std::cout << ">>>>>>发送登出请求成功" << std::endl;
    else
        std::cerr << "--->>>发送登出请求失败" << std::endl;
}


void CustomTradeSpi::reqSettlementInfoConfirm()
{

    CThostFtdcSettlementInfoConfirmField settlementConfirmReq;
    memset(&settlementConfirmReq, 0, sizeof(settlementConfirmReq));
    strcpy(settlementConfirmReq.BrokerID, gBrokerID);
    strcpy(settlementConfirmReq.InvestorID, gInvesterID);
    static int requestID = 0; // 请求编号
    std::cout << "===结算确认===" << std::endl;
    print_time();
    int rt = g_pTradeUserApi->ReqSettlementInfoConfirm(&settlementConfirmReq, requestID);
    if (!rt)
        std::cout << ">>>>>>结算确认成功" << std::endl;
    else
        std::cerr << "--->>>结算确认失败" << std::endl;
/*
    CThostFtdcQryInstrumentField req;
    memset(&req, 0, sizeof(req));
    int a=g_pTradeUserApi->ReqQryInstrument(&req,0);
    std::cout<<"a="<<a<<std::endl;

*/

}

void CustomTradeSpi::reqQueryInstrument()
{

    CThostFtdcQryInstrumentField instrumentReq;
    memset(&instrumentReq, 0, sizeof(instrumentReq));
    //strcpy(instrumentReq.InstrumentID, g_pTradeInstrumentID);
    static int requestID = 0; // 请求编号
    std::cout << "===开始查询全部合约===" << std::endl;
    print_time();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 有时候需要停顿一会才能查询成功
    int rt = g_pTradeUserApi->ReqQryInstrument(&instrumentReq, requestID);
    if (!rt)
        std::cout << ">>>>>>查询全部合约成功" << std::endl;
    else
        std::cerr << "--->>>查询全部合约失败" << std::endl;
}

void CustomTradeSpi::reqQueryTradingAccount()
{
    CThostFtdcQryTradingAccountField tradingAccountReq;
    memset(&tradingAccountReq, 0, sizeof(tradingAccountReq));
    strcpy(tradingAccountReq.BrokerID, gBrokerID);
    strcpy(tradingAccountReq.InvestorID, gInvesterID);
    static int requestID = 0; // 请求编号
    //std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
    std::cout << "===开始资金查询请求===" << std::endl;
    print_time();
    int rt = g_pTradeUserApi->ReqQryTradingAccount(&tradingAccountReq, ++requestID);
    //std::cout<<"rt="<<rt<<std::endl;
    if (!rt)
        std::cout << ">>>>>>资金查询请求成功" << std::endl;
    else
        std::cerr << "--->>>资金查询请求失败" << std::endl;
}

void CustomTradeSpi::reqQueryInvestorPosition()
{
    CThostFtdcQryInvestorPositionField postionReq;
    memset(&postionReq, 0, sizeof(postionReq));
    strcpy(postionReq.BrokerID, gBrokerID);
    strcpy(postionReq.InvestorID, gInvesterID);
    //strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
    static int requestID = 0; // 请求编号
    std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
    std::cout << "===开始持仓查询===" << std::endl;
    print_time();
    int rt = g_pTradeUserApi->ReqQryInvestorPosition(&postionReq, requestID);
    if (!rt)
        std::cout << ">>>>>>持仓查询成功" << std::endl;
    else
        std::cerr << "--->>>持仓查询失败" << std::endl;
}
void CustomTradeSpi::reqQueryInvestorPositionDetail()
{
    CThostFtdcQryInvestorPositionDetailField postionReq;
    memset(&postionReq, 0, sizeof(postionReq));
    strcpy(postionReq.BrokerID, gBrokerID);
    strcpy(postionReq.InvestorID, gInvesterID);
    //strcpy(postionReq.InstrumentID, g_pTradeInstrumentID);
    static int requestID = 0; // 请求编号
    std::this_thread::sleep_for(std::chrono::milliseconds(700)); // 有时候需要停顿一会才能查询成功
    std::cout << "===开始持仓明细查询===" << std::endl;
    print_time();
    int rt = g_pTradeUserApi->ReqQryInvestorPositionDetail(&postionReq, requestID);
    if (!rt)
        std::cout << ">>>>>>持仓查询成功" << std::endl;
    else
        std::cerr << "--->>>持仓查询失败" << std::endl;
}
void CustomTradeSpi::reqOrderInsert()
{
    CThostFtdcInputOrderField orderInsertReq;
    memset(&orderInsertReq, 0, sizeof(orderInsertReq));
    ///经纪公司代码
    strcpy(orderInsertReq.BrokerID, gBrokerID);
    ///投资者代码
    strcpy(orderInsertReq.InvestorID, gInvesterID);
    ///合约代码
    strcpy(orderInsertReq.InstrumentID, g_pTradeInstrumentID);
    ///报单引用
    strcpy(orderInsertReq.OrderRef, order_ref);
    ///报单价格条件: 限价
    orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向:
    orderInsertReq.Direction = gTradeDirection;
    ///组合开平标志: 开仓
    orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    orderInsertReq.LimitPrice = gLimitPrice;
    ///数量：1
    orderInsertReq.VolumeTotalOriginal = 1;
    ///有效期类型: 当日有效
    orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
    ///成交量类型: 任何数量
    orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    orderInsertReq.MinVolume = 1;
    ///触发条件: 立即
    orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///强平原因: 非强平
    orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    orderInsertReq.IsAutoSuspend = 0;
    ///用户强评标志: 否
    orderInsertReq.UserForceClose = 0;

    static int requestID = 0; // 请求编号
    int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
    if (!rt)
        std::cout << ">>>>>>报单录入成功" << std::endl;
    else
        std::cerr << "--->>>报单录入失败" << std::endl;
}

void CustomTradeSpi::reqOrderInsert(
        TThostFtdcInstrumentIDType instrumentID,
        TThostFtdcPriceType price,
        TThostFtdcVolumeType volume,
        TThostFtdcDirectionType direction)
{
    CThostFtdcInputOrderField orderInsertReq;
    memset(&orderInsertReq, 0, sizeof(orderInsertReq));
    ///经纪公司代码
    strcpy(orderInsertReq.BrokerID, gBrokerID);
    ///用户代码
    strcpy(orderInsertReq.InvestorID, gInvesterID);
    ///合约代码
    strcpy(orderInsertReq.InstrumentID, instrumentID);
    ///报单引用
    strcpy(orderInsertReq.OrderRef, order_ref);
    ///报单价格条件: 限价
    orderInsertReq.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向:
    orderInsertReq.Direction = direction;
    ///组合开平标志: 开仓
    orderInsertReq.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    orderInsertReq.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    orderInsertReq.LimitPrice = price;
    ///数量：1
    orderInsertReq.VolumeTotalOriginal = volume;
    ///有效期类型: 当日有效
    orderInsertReq.TimeCondition = THOST_FTDC_TC_GFD;
    ///成交量类型: 任何数量
    orderInsertReq.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    orderInsertReq.MinVolume = 1;
    ///触发条件: 立即
    orderInsertReq.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///强平原因: 非强平
    orderInsertReq.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    orderInsertReq.IsAutoSuspend = 0;
    ///用户强评标志: 否
    orderInsertReq.UserForceClose = 0;

    static int requestID = 0; // 请求编号
    int rt = g_pTradeUserApi->ReqOrderInsert(&orderInsertReq, ++requestID);
    if (!rt)
        std::cout << ">>>>>>发送报单录入请求成功" << std::endl;
    else
        std::cerr << "--->>>发送报单录入请求失败" << std::endl;
}

void CustomTradeSpi::reqOrderAction(CThostFtdcOrderField *pOrder)
{
    static bool orderActionSentFlag = false; // 是否发送了报单
    if (orderActionSentFlag)
        return;

    CThostFtdcInputOrderActionField orderActionReq;
    memset(&orderActionReq, 0, sizeof(orderActionReq));
    ///经纪公司代码
    strcpy(orderActionReq.BrokerID, pOrder->BrokerID);
    ///投资者代码
    strcpy(orderActionReq.InvestorID, pOrder->InvestorID);
    ///报单操作引用
    //	TThostFtdcOrderActionRefType	OrderActionRef;
    ///报单引用
    strcpy(orderActionReq.OrderRef, pOrder->OrderRef);
    ///请求编号
    //	TThostFtdcRequestIDType	RequestID;
    ///前置编号
    orderActionReq.FrontID = trade_front_id;
    ///会话编号
    orderActionReq.SessionID = session_id;
    ///交易所代码
    //	TThostFtdcExchangeIDType	ExchangeID;
    ///报单编号
    //	TThostFtdcOrderSysIDType	OrderSysID;
    ///操作标志
    orderActionReq.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
    //	TThostFtdcPriceType	LimitPrice;
    ///数量变化
    //	TThostFtdcVolumeType	VolumeChange;
    ///用户代码
    //	TThostFtdcUserIDType	UserID;
    ///合约代码
    strcpy(orderActionReq.InstrumentID, pOrder->InstrumentID);
    static int requestID = 0; // 请求编号
    int rt = g_pTradeUserApi->ReqOrderAction(&orderActionReq, ++requestID);
    if (!rt)
        std::cout << ">>>>>>发送报单操作请求成功" << std::endl;
    else
        std::cerr << "--->>>发送报单操作请求失败" << std::endl;
    orderActionSentFlag = true;
}

bool CustomTradeSpi::isMyOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->FrontID == trade_front_id) &&
            (pOrder->SessionID == session_id) &&
            (strcmp(pOrder->OrderRef, order_ref) == 0));
}

bool CustomTradeSpi::isTradingOrder(CThostFtdcOrderField *pOrder)
{
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

void CustomTradeSpi::print_time()
{
    time_t tt;
    time( &tt );
    tt = tt +8*3600;  // transform the time zone
    tm* t= gmtime( &tt );
    //std::cout<<t->tm_hour<<"\t"<<t->tm_min<<"\t"<<t->tm_sec<<std::endl;
    int client_year=t->tm_year+1900;
    int client_mon=t->tm_mon+1;
    int client_day=t->tm_mday;
    int client_hour= t->tm_hour;;
    int client_min= t->tm_min;
    int client_sec=t->tm_sec;
    std::string test="";
    test=test+std::to_string(client_year)+"-";
    test=client_mon<10?test+"0"+std::to_string(client_mon)+"-":test+std::to_string(client_mon)+"-";
    test=client_day<10?test+"0"+std::to_string(client_day)+" ":test+std::to_string(client_day)+" ";
    test=client_hour<10?test+"0"+std::to_string(client_hour)+":":test+std::to_string(client_hour)+":";
    test=client_min<10?test+"0"+std::to_string(client_min)+":":test+std::to_string(client_min)+":";
    test=client_sec<10?test+"0"+std::to_string(client_sec):test+std::to_string(client_sec);
    //std::cout<<"Now Time "<<client_year<<"-"<<client_mon<<"-"<<client_day<<" "<<client_hour<<":"<<client_min<<":"<<client_sec<<std::endl;
    std::cout<<test<<std::endl;
}
#endif
