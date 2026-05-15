# 需要的 class
1. engine
2. IStrategy(不同策略去繼承)

## Notes
1. 看起來 Shioaji API 下單都是以 Trade 物件為主，而 Trade 則是包含 (Contract, Order, Status)


## Engine

```C++
class Engine {
    /// 交易相關
    /// 系統層級風控
    bool RiskCheck(Trade);
    /// 新單 
    bool SendNewReq(Trade);
    /// 改價格
    bool SendChgPri(Trade, uint64_t);
    /// 改數量（數量為 0 視同刪單，不再多開一個 API）
    bool SendChgQty(Trade, uint64_t);

    /// 接收到資料的時候推送到所有有註冊的策略裡面
    vector<IStrategy> vs_;
    /// 需要紀錄有哪些 request (是否用 vector 需要再思考)
    /// 每一個策略的下單的委託狀態需要統一紀錄
    /// TODO: Request 要怎麼紀錄生命歷程？
    ///       像 f9omstw 一樣用每一筆 Request 有 Order 當作容器
    ///       用 Ordraw 當作此 Request 的 Snapshot？
    unordered_map<ordno, Trade> total_req_;

    /// 帳務相關
    /// 當日已實現損益，若超過的話或許需要停止 Engine
    int64_t total_profit_;
}
```
### 問題
1. tick/bidask是否需要儲存在engine?
    - 應該是依照策略需求？看有沒有策略需要？
2. OrderBook 需要 maintain 嗎？
    - 目前也不太確定，感覺不會有情況需要存著所有商品的 OrderBook 
    - 這可以讓需要的策略自己設計怎麼儲存？

## 策略
```C++
class IStrategy {
    /// 如何處理解析過的 Tick，決定收到新的資料後是否要觸發新刪改單
    /// 裡面流程應該為
    /// 更新自己維護的資料 -> 判斷訊號是否觸發 -> 組成 Request 呼叫 Engine 提供的 API
    bool OnRecvTick(Tick) virtual;
    /// 如何處理解析過的 BidAsk，決定收到新的資料後是否要觸發新刪改單
    bool OnRecvBidAsk(BidAsk) virtual;
    /// 處理回報（委託回報、成交回報）
    bool OnReport(Report) virtual;
    /// 策略風控？ 
    bool StratRiskCehck(Trade) virtual;

    /// 策略自己的 Request 狀態
    /// 似乎有 ordno 這個 unique key，或許應該用 unordered_map 存取？
    unordered_map<ordno, Trade> req_;

    /// 策略自己的當日損益
    int64_t profit_;
 

    /// 以下為自己需要的變數名稱 ...
}
```