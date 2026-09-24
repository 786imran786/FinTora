export type ConnectionStatus = 'CONNECTED' | 'CONNECTING' | 'DISCONNECTED'
export type OrderSide = 'BUY' | 'SELL'
export type OrderType = 'LIMIT' | 'MARKET'
export type NotificationTone = 'success' | 'warning' | 'error' | 'info'
export type OrderStatus = 'OPEN' | 'PARTIALLY_FILLED' | 'FILLED' | 'CANCELLED'

export interface OrderBookLevel {
  price: number
  quantity: number
}

export interface OrderBookState {
  asks: OrderBookLevel[]
  bids: OrderBookLevel[]
  spread: number
  bestAsk: number | null
  bestBid: number | null
}

export interface MarketStats {
  lastPrice: number
  bestBid: number
  bestAsk: number
  spread: number
  volume: number
}

export interface Order {
  id: number
  side: OrderSide
  type: OrderType
  price: number | null
  quantity: number
  remaining: number
  status: OrderStatus
  createdAt: string
}

export interface Trade {
  id: number
  price: number
  quantity: number
  side: OrderSide
  time: string
}

export interface Metrics {
  ordersProcessed: number
  tradesExecuted: number
  ordersPerSecond: number
  activeOrders: number
  totalVolume: number
}

export interface Notification {
  id: number
  tone: NotificationTone
  title: string
  message: string
}

export interface ChartPoint {
  time: string
  price: number
}

export interface SocketMessage extends Record<string, unknown> {
  type: string
  [key: string]: unknown
}

export interface PlaceOrderPayload extends Record<string, unknown> {
  type: 'PLACE_ORDER'
  side: OrderSide
  orderType: OrderType
  price?: number
  quantity: number
}

export interface CancelOrderPayload extends Record<string, unknown> {
  type: 'CANCEL_ORDER'
  orderId: number
}
