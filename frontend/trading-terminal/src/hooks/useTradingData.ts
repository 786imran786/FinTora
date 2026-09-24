import { useCallback, useEffect, useMemo, useState } from 'react'
import type {
  ChartPoint,
  MarketStats,
  Metrics,
  Notification,
  NotificationTone,
  Order,
  OrderBookState,
  SocketMessage,
  Trade,
} from '../types/trading'

const baseOrderBook: OrderBookState = {
  asks: [
    { price: 105.0, quantity: 100 },
    { price: 104.0, quantity: 250 },
    { price: 103.0, quantity: 180 },
  ],
  bids: [
    { price: 102.5, quantity: 300 },
    { price: 101.8, quantity: 450 },
    { price: 100.9, quantity: 600 },
  ],
  spread: 0.7,
  bestAsk: 103,
  bestBid: 102.5,
}

const baseMarketStats: MarketStats = {
  lastPrice: 102.5,
  bestBid: 102.5,
  bestAsk: 103,
  spread: 0.5,
  volume: 12450,
}

const baseMetrics: Metrics = {
  ordersProcessed: 1284521,
  tradesExecuted: 482391,
  ordersPerSecond: 18420,
  activeOrders: 8452,
  totalVolume: 4820000,
}

const baseTrades: Trade[] = [
  { id: 1, price: 102.5, quantity: 100, side: 'BUY', time: '12:31:05' },
  { id: 2, price: 102.4, quantity: 250, side: 'SELL', time: '12:31:02' },
  { id: 3, price: 102.4, quantity: 50, side: 'BUY', time: '12:30:58' },
]

const baseOrders: Order[] = [
  { id: 1001, side: 'BUY', type: 'LIMIT', price: 100.2, quantity: 100, remaining: 60, status: 'OPEN', createdAt: '12:31:05' },
  { id: 1002, side: 'SELL', type: 'LIMIT', price: 105.1, quantity: 200, remaining: 200, status: 'OPEN', createdAt: '12:31:02' },
]

const formatTime = (date: Date) => `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}`

export function useTradingData(messages: SocketMessage[]) {
  const [orderBook, setOrderBook] = useState<OrderBookState>(baseOrderBook)
  const [marketStats, setMarketStats] = useState<MarketStats>(baseMarketStats)
  const [openOrders, setOpenOrders] = useState<Order[]>(baseOrders)
  const [trades, setTrades] = useState<Trade[]>(baseTrades)
  const [metrics, setMetrics] = useState<Metrics>(baseMetrics)
  const [notifications, setNotifications] = useState<Notification[]>([])

  const addNotification = useCallback((tone: NotificationTone, title: string, message: string) => {
    setNotifications((prev) => [{ id: Date.now() + Math.random(), tone, title, message }, ...prev].slice(0, 6))
  }, [])

  useEffect(() => {
    if (!messages.length) return

    const latest = messages[messages.length - 1]
    if (!latest || typeof latest.type !== 'string') return

    switch (latest.type) {
      case 'ORDER_BOOK_UPDATE': {
        const asks = Array.isArray(latest.asks)
          ? (latest.asks as Array<{ price?: number; quantity?: number }>).map((level) => ({
              price: Number(level.price ?? 0),
              quantity: Number(level.quantity ?? 0),
            }))
          : baseOrderBook.asks

        const bids = Array.isArray(latest.bids)
          ? (latest.bids as Array<{ price?: number; quantity?: number }>).map((level) => ({
              price: Number(level.price ?? 0),
              quantity: Number(level.quantity ?? 0),
            }))
          : baseOrderBook.bids

        const sortedAsks = [...asks].sort((a, b) => a.price - b.price)
        const sortedBids = [...bids].sort((a, b) => b.price - a.price)
        const bestAsk = sortedAsks[0]?.price ?? null
        const bestBid = sortedBids[0]?.price ?? null
        const spread = bestAsk !== null && bestBid !== null ? Number((bestAsk - bestBid).toFixed(2)) : 0

        setOrderBook({
          asks: sortedAsks,
          bids: sortedBids,
          spread,
          bestAsk,
          bestBid,
        })

        setMarketStats((prev) => ({
          ...prev,
          bestAsk: bestAsk ?? prev.bestAsk,
          bestBid: bestBid ?? prev.bestBid,
          spread: spread || prev.spread,
        }))
        break
      }

      case 'TRADE': {
        const trade: Trade = {
          id: Number(latest.id ?? Date.now()),
          price: Number(latest.price ?? 0),
          quantity: Number(latest.quantity ?? 0),
          side: latest.side === 'SELL' ? 'SELL' : 'BUY',
          time: typeof latest.time === 'string' ? latest.time : formatTime(new Date()),
        }

        setTrades((prev) => [trade, ...prev].slice(0, 10))
        setMarketStats((prev) => ({ ...prev, lastPrice: trade.price, volume: prev.volume + trade.quantity }))
        addNotification('success', 'Trade executed', `${trade.quantity} @ ${trade.price.toFixed(2)}`)
        break
      }

      case 'METRICS': {
        setMetrics({
          ordersProcessed: Number(latest.ordersProcessed ?? 0),
          tradesExecuted: Number(latest.tradesExecuted ?? 0),
          ordersPerSecond: Number(latest.ordersPerSecond ?? 0),
          activeOrders: Number(latest.activeOrders ?? 0),
          totalVolume: Number(latest.totalVolume ?? 0),
        })
        break
      }

      case 'ORDER_ACCEPTED': {
        const order: Order = {
          id: Number(latest.orderId ?? 0),
          side: latest.side === 'SELL' ? 'SELL' : 'BUY',
          type: latest.orderType === 'MARKET' ? 'MARKET' : 'LIMIT',
          price: latest.price !== undefined ? Number(latest.price) : null,
          quantity: Number(latest.quantity ?? 0),
          remaining: Number(latest.quantity ?? 0),
          status: 'OPEN',
          createdAt: formatTime(new Date()),
        }

        setOpenOrders((prev) => [order, ...prev.filter((item) => item.id !== order.id)].slice(0, 12))
        addNotification('success', 'Order accepted', `Order #${order.id}`)
        break
      }

      case 'ORDER_STATUS': {
        const orderId = Number(latest.orderId ?? 0)
        const remaining = Number(latest.remaining ?? 0)
        setOpenOrders((prev) =>
          prev.map((item) =>
            item.id === orderId ? { ...item, remaining, status: remaining <= 0 ? 'FILLED' : 'OPEN' } : item,
          ),
        )
        break
      }

      case 'ORDER_CANCELLED': {
        const orderId = Number(latest.orderId ?? 0)
        setOpenOrders((prev) => prev.filter((item) => item.id !== orderId))
        addNotification('warning', 'Order cancelled', `Order #${orderId}`)
        break
      }

      case 'NOTIFICATION': {
        addNotification(
          (latest.tone as NotificationTone) ?? 'info',
          String(latest.title ?? 'Market update'),
          String(latest.message ?? ''),
        )
        break
      }

      case 'ERROR': {
        addNotification('error', 'Backend error', String(latest.message ?? 'Unable to place order'))
        break
      }

      default:
        break
    }
  }, [addNotification, messages])

  const chartData = useMemo<ChartPoint[]>(() => {
    const baseChart = [
      { time: '12:00', price: 100.2 },
      { time: '12:01', price: 100.5 },
      { time: '12:02', price: 100.3 },
      { time: '12:03', price: 101.0 },
      { time: '12:04', price: 101.4 },
      { time: '12:05', price: 102.1 },
    ]

    return [...baseChart, ...trades.map((trade) => ({ time: trade.time, price: trade.price }))]
      .slice(-12)
      .map((point) => ({ ...point, time: String(point.time), price: Number(point.price) }))
  }, [trades])

  return {
    orderBook,
    marketStats,
    openOrders,
    trades,
    metrics,
    notifications,
    chartData,
  }
}
