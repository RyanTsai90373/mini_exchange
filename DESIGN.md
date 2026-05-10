# TickEngine 設計

## 整體架構（兩個程式互傳訊息）

```
┌─ Python 程式 ───────────────┐              ┌─ C++ 程式 ────────────────┐
│                             │              │                            │
│  收永豐 API 行情             │              │  收行情訊息                 │
│        ↓                    │   ZMQ        │        ↓                   │
│  包成 JSON                   │ ───────────→ │  解 JSON 變成 C++ 物件     │
│        ↓                    │              │        ↓                   │
│  送出去                      │              │  丟進主 loop 處理           │
│                             │              │                            │
│  收 C++ 來的下單請求          │   ZMQ        │  策略產生下單請求           │
│        ↑                    │ ←─────────── │        ↓                   │
│  解 JSON                     │              │  風控檢查                   │
│        ↓                    │              │        ↓                   │
│  呼叫永豐 API 送單            │              │  通過就包成 JSON 送出       │
│        ↓                    │              │                            │
│  收成交回報                   │              │  收成交回報訊息             │
│        ↓                    │   ZMQ        │        ↓                   │
│  包成 JSON                   │ ───────────→ │  解 JSON                    │
│        ↓                    │              │        ↓                   │
│  送出去                      │              │  更新部位、損益              │
│                             │              │                            │
└─────────────────────────────┘              └────────────────────────────┘
```

重點：
- C++ 不會直接呼叫永豐 API（永豐 SDK 是 Python 的）
- 兩邊互傳的東西全部是 JSON
- 中間靠 ZMQ 把 JSON 從一個程式送到另一個程式

---

## C++ 程式內部結構

```
                    ZMQ 收訊息 thread
                          ↓
                    放進內部 queue
                          ↓
                    主 loop thread（一個 thread 順序跑下面所有事）
                          ↓
                    從 queue 拿事件出來
                          ↓
              ┌──────────┴──────────┐
              ↓                     ↓
        是行情？               是成交回報？
              ↓                     ↓
        通知所有策略             更新部位 / 損益
              ↓
        策略吐出下單請求
              ↓
        風控 check
              ↓
        通過就放進「要送出 queue」
              ↓
                    ZMQ 送訊息 thread
                          ↓
                    送回 Python
```

主 loop 那個 thread 順序處理所有事，不開 thread pool 給策略並行。
（理由：策略計算很快，加 thread 反而更慢更難 debug）

---

## 主要要做的東西

### Python 那邊
- 接永豐 API（行情 / 成交回報 / 送單）
- 收到東西包成 JSON 用 ZMQ 送出去
- 收到 C++ 的下單請求，呼叫永豐 API

### C++ 那邊
- ZMQ 收 thread
- ZMQ 送 thread
- 主 loop（處理事件、跑策略、做風控）
- 部位簿（記現在持有什麼）
- 損益計算

### 介面要定義的
```
IStrategy（策略長怎樣）
  - on_tick(tick)   → 收到行情要做什麼
  - on_fill(fill)   → 收到成交回報要做什麼

IRiskCheck（風控規則）
  - check(order)    → 這張單可以送嗎？
```

之後想換策略 / 換風控 → 寫一個新 class 實作介面就好，engine 不用改。

策略要自己的欄位 / 自己的計算邏輯 → 寫在自己的 class 裡，不要去動 engine。

---

## 開發順序

1. **先讓 ZMQ 通起來**
   - Python 寫死一筆假行情，ZMQ 送出
   - C++ 收到、印出來
   - 通了才繼續

2. **定義 JSON 訊息格式**
   - 行情、下單請求、成交回報各長什麼樣
   - 每筆訊息都要有 `msg_type` 跟 `seq_num`
     （現在用不到沒關係，之後升級不用改 protocol）

3. **架起 C++ 主 loop**
   - 收 ZMQ → 進 queue → 主 loop 處理
   - 還沒策略沒風控也沒關係
   - 先 print 驗證 pipeline 通

4. **寫一個假策略**
   - 實作 IStrategy
   - on_tick 裡面 print 就好
   - 掛到 engine 上、確認行情有傳到

5. **寫一個假風控**
   - 實作 IRiskCheck
   - check 永遠 return true
   - 確認下單流程跑得通

6. **接真實永豐 API**

7. **把 quant research 時期的策略搬進來**

---

## 注意事項

### 1. 策略不要繼承 Engine
- 策略是「掛在 engine 上的東西」，不是「engine 的子類」
- 每個策略寫成 IStrategy 的實作

### 2. 主 loop thread 不要加 mutex / lock
- 一個 thread 順序跑所有事，不會搶資源
- 看到自己想加 `std::mutex` 在策略邏輯裡，停下來重想

### 3. C++ 不會直接呼叫永豐 API
- 永豐 SDK 在 Python
- C++ 想下單 = 送下單請求 JSON 給 Python，由 Python 呼叫永豐

### 4. JSON 訊息一定要有 `msg_type` 跟 `seq_num`
- 現在不用做什麼事
- 之後想加新訊息類型 / 重送機制不用改 protocol

### 5. 風控放 C++，但只做基本的
- 單筆 size 上限
- 部位上限
- 一天最大虧損
- 不要做太複雜的東西

### 6. V1 採 fail-fast
- JSON parse 失敗：log + 跳過該筆
- ZMQ 連不上：直接結束程式
- 策略 throw exception：log + 結束程式
- 不要寫一堆 try-catch 假裝沒事

---

## V1 不做的事（清單）

之後升級再做，現在做了只是浪費時間：

- [ ] 斷線後自動重連
- [ ] 訊息重送 / 補單機制
- [ ] 部位對帳（broker vs engine）
- [ ] 多帳戶 / 多市場
- [ ] 策略並行（thread pool）
- [ ] Binary 序列化（先用 JSON）
- [ ] Shared memory（先用 ZMQ）
- [ ] 即時 dashboard
- [ ] Backtest 模式（interface 已預留，之後再實作）

---

## 跟 ADR-0001 的關係

ADR-0001（2026-05-09 frozen）已被這份設計 supersede。
主要差別：
- ADR-0001：single-process + pybind11 + Python 主導
- 這份：two-process + ZMQ + JSON + C++ 主導 engine 邏輯

之後要寫 ADR-0002 正式記錄這個變更原因（真實策略需求 → 永豐 Python API → 自然導向 IPC 架構）。
