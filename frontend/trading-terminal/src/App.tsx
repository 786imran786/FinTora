import './App.css'
import Header from './components/Header/Header'
import MarketStats from './components/MarketStats/MarketStats'
import OrderBook from './components/OrderBook/OrderBook'
import OrderForm from './components/OrderForm/orderform'
import PriceChart from './components/PriceChart/pricechart'
import OpenOrders from './components/OpenOrders/openorders'
import TradeHistory from './components/TradeHistory/tradehistory'
import EngineMetrics from './components/EngineMetrics/EngineMetrics'
import Notifications from './components/Notifications/Notifications'
import { useWebSocket } from './hooks/useWebSocket'
import { useTradingData } from './hooks/useTradingData'
import { buildCancelOrderMessage } from './services/websocket'

function App() {
  const { connectionStatus, messages, sendMessage } = useWebSocket()
  const { orderBook, marketStats, openOrders, trades, metrics, notifications, chartData } = useTradingData(messages)

  const handleOrderSubmit = (payload: Record<string, unknown>) => sendMessage(payload)

  const handleCancelOrder = (orderId: number) => {
    const confirmed = window.confirm(`Cancel order #${orderId}?`)
    if (!confirmed) return
    sendMessage(buildCancelOrderMessage(orderId))
  }

  return (
    <div className="app-shell">
      <Notifications notifications={notifications} />

      <div className="terminal">
        <Header connectionStatus={connectionStatus} pair="BTC/USDT" />
        <MarketStats stats={marketStats} />

        <div className="workspace-grid">
          <div className="main-column">
            <PriceChart data={chartData} />

            <div className="lower-grid">
              <OpenOrders orders={openOrders} onCancel={handleCancelOrder} />
              <TradeHistory trades={trades} />
            </div>
          </div>

          <div className="side-column">
            <OrderBook book={orderBook} />
            <OrderForm onSubmit={handleOrderSubmit} />
          </div>
        </div>

        <EngineMetrics metrics={metrics} />
      </div>
    </div>
  )
}

export default App
