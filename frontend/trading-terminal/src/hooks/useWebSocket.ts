import { useCallback, useEffect, useRef, useState } from 'react'
import { safeParseJson } from '../services/websocket'
import type { ConnectionStatus, SocketMessage } from '../types/trading'

const isMockMode = () => import.meta.env.VITE_USE_MOCK_WS === 'true'

const formatTimestamp = (date: Date) =>
  `${String(date.getHours()).padStart(2, '0')}:${String(date.getMinutes()).padStart(2, '0')}:${String(date.getSeconds()).padStart(2, '0')}`

const makeMockOrderBook = () => ({
  type: 'ORDER_BOOK_UPDATE',
  asks: [
    { price: 105.0, quantity: 100 },
    { price: 104.0, quantity: 250 },
    { price: 103.0, quantity: 180 },
    { price: 102.5, quantity: 220 },
  ],
  bids: [
    { price: 102.5, quantity: 300 },
    { price: 101.8, quantity: 450 },
    { price: 100.9, quantity: 600 },
    { price: 100.1, quantity: 710 },
  ],
})

const makeMockMetrics = (step: number) => ({
  type: 'METRICS',
  ordersProcessed: 1284521 + step * 17,
  tradesExecuted: 482391 + step * 5,
  ordersPerSecond: 18420 + step,
  activeOrders: 8452 + (step % 10),
  totalVolume: 4820000 + step * 220,
})

const makeMockTrade = (step: number): SocketMessage => ({
  type: 'TRADE',
  id: Date.now() + step,
  price: Number((101.4 + ((step * 1.17) % 3.2)).toFixed(2)),
  quantity: 60 + (step % 5) * 20,
  side: step % 2 === 0 ? 'BUY' : 'SELL',
  time: formatTimestamp(new Date()),
})

const makeMockResponses = (payload: Record<string, unknown>): SocketMessage[] => {
  if (payload.type === 'PLACE_ORDER') {
    const orderId = Math.floor(Date.now() % 9000) + 1000
    return [
      {
        type: 'ORDER_ACCEPTED',
        orderId,
        side: payload.side,
        orderType: payload.orderType,
        quantity: payload.quantity,
        price: payload.price,
        status: 'OPEN',
      },
      {
        type: 'ORDER_STATUS',
        orderId,
        status: 'OPEN',
        remaining: Number(payload.quantity ?? 0),
      },
      makeMockOrderBook(),
      {
        type: 'NOTIFICATION',
        tone: 'success',
        title: 'Order accepted',
        message: `Order #${orderId}`,
      },
    ]
  }

  if (payload.type === 'CANCEL_ORDER') {
    const orderId = Number(payload.orderId ?? 0)
    return [
      {
        type: 'ORDER_CANCELLED',
        orderId,
        status: 'CANCELLED',
      },
      makeMockOrderBook(),
      {
        type: 'NOTIFICATION',
        tone: 'warning',
        title: 'Order cancelled',
        message: `Order #${orderId}`,
      },
    ]
  }

  return []
}

export function useWebSocket() {
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>(isMockMode() ? 'CONNECTED' : 'CONNECTING')
  const [messages, setMessages] = useState<SocketMessage[]>([])
  const socketRef = useRef<WebSocket | null>(null)
  const mockStepRef = useRef(0)

  useEffect(() => {
    if (isMockMode()) {
      setConnectionStatus('CONNECTED')

      const intervalId = window.setInterval(() => {
        mockStepRef.current += 1
        setMessages((prev) => [
          ...prev,
          makeMockTrade(mockStepRef.current),
          makeMockMetrics(mockStepRef.current),
          makeMockOrderBook(),
        ].slice(-30))
      }, 4000)

      return () => window.clearInterval(intervalId)
    }

    const wsUrl = import.meta.env.VITE_WS_URL ?? 'ws://localhost:8080'
    const socket = new WebSocket(wsUrl)
    socketRef.current = socket
    setConnectionStatus('CONNECTING')

    socket.onopen = () => {
      setConnectionStatus('CONNECTED')
      // Request initial metrics immediately on connect
      socket.send(JSON.stringify({ type: 'GET_METRICS' }))
    }
    socket.onclose = () => setConnectionStatus('DISCONNECTED')
    socket.onerror = () => setConnectionStatus('DISCONNECTED')
    socket.onmessage = (event) => {
      const parsed = safeParseJson(event.data)
      if (!parsed) return

      const message = parsed as SocketMessage
      if (message.type) {
        setMessages((prev) => [...prev, message])
      }
    }

    // Poll metrics every 5 seconds
    const metricsInterval = window.setInterval(() => {
      if (socket.readyState === WebSocket.OPEN) {
        socket.send(JSON.stringify({ type: 'GET_METRICS' }))
      }
    }, 5000)

    return () => {
      window.clearInterval(metricsInterval)
      socket.close()
      socketRef.current = null
    }
  }, [])

  const connect = useCallback(() => {
    if (isMockMode()) {
      setConnectionStatus('CONNECTED')
      return
    }

    const wsUrl = import.meta.env.VITE_WS_URL ?? 'ws://localhost:8080'
    const socket = new WebSocket(wsUrl)
    socketRef.current = socket
    setConnectionStatus('CONNECTING')

    socket.onopen = () => {
      setConnectionStatus('CONNECTED')
      socket.send(JSON.stringify({ type: 'GET_METRICS' }))
    }
    socket.onclose = () => setConnectionStatus('DISCONNECTED')
    socket.onerror = () => setConnectionStatus('DISCONNECTED')
    socket.onmessage = (event) => {
      const parsed = safeParseJson(event.data)
      if (!parsed) return

      const message = parsed as SocketMessage
      if (message.type) {
        setMessages((prev) => [...prev, message])
      }
    }
  }, [])

  const sendMessage = useCallback((payload: Record<string, unknown>) => {
    if (isMockMode()) {
      const responses = makeMockResponses(payload)
      setMessages((prev) => [...prev, ...responses])
      return true
    }

    if (!socketRef.current || socketRef.current.readyState !== WebSocket.OPEN) {
      setConnectionStatus('DISCONNECTED')
      return false
    }

    try {
      socketRef.current.send(JSON.stringify(payload))
      return true
    } catch {
      return false
    }
  }, [])

  return { connectionStatus, messages, connect, sendMessage }
}
