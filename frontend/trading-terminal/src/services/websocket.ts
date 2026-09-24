import type { CancelOrderPayload, OrderSide, OrderType, PlaceOrderPayload } from '../types/trading'

export function buildPlaceOrderMessage(
  side: OrderSide,
  orderType: OrderType,
  quantity: number,
  price?: number,
): PlaceOrderPayload {
  if (orderType === 'LIMIT') {
    return {
      type: 'PLACE_ORDER',
      side,
      orderType,
      price: Number(price ?? 0),
      quantity: Number(quantity),
    }
  }

  return {
    type: 'PLACE_ORDER',
    side,
    orderType,
    quantity: Number(quantity),
  }
}

export function buildCancelOrderMessage(orderId: number): CancelOrderPayload {
  return {
    type: 'CANCEL_ORDER',
    orderId,
  }
}

export function safeParseJson(raw: string): Record<string, unknown> | null {
  try {
    const parsed = JSON.parse(raw)
    return parsed && typeof parsed === 'object' ? (parsed as Record<string, unknown>) : null
  } catch {
    return null
  }
}
