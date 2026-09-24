import { useMemo, useState } from 'react'
import { buildPlaceOrderMessage } from '../../services/websocket'
import type { OrderSide, OrderType } from '../../types/trading'

interface OrderFormProps {
  onSubmit: (payload: Record<string, unknown>) => boolean
}

const sideStyles: Record<OrderSide, string> = {
  BUY: 'buy-active',
  SELL: 'sell-active',
}

export default function OrderForm({ onSubmit }: OrderFormProps) {
  const [side, setSide] = useState<OrderSide>('BUY')
  const [orderType, setOrderType] = useState<OrderType>('LIMIT')
  const [price, setPrice] = useState('102.50')
  const [quantity, setQuantity] = useState('100')
  const [error, setError] = useState('')

  const total = useMemo(() => {
    if (orderType === 'MARKET') return 0
    const parsedPrice = Number(price)
    const parsedQuantity = Number(quantity)
    if (!Number.isFinite(parsedPrice) || parsedPrice <= 0 || !Number.isFinite(parsedQuantity) || parsedQuantity <= 0) {
      return 0
    }
    return parsedPrice * parsedQuantity
  }, [orderType, price, quantity])

  const handleSubmit = (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault()

    const parsedQuantity = Number(quantity)
    const parsedPrice = Number(price)

    if (!Number.isFinite(parsedQuantity) || parsedQuantity <= 0) {
      setError('Quantity must be greater than 0.')
      return
    }

    if (orderType === 'LIMIT' && (!Number.isFinite(parsedPrice) || parsedPrice <= 0)) {
      setError('LIMIT price must be greater than 0.')
      return
    }

    const payload = buildPlaceOrderMessage(side, orderType, parsedQuantity, orderType === 'LIMIT' ? parsedPrice : undefined)
    const sent = onSubmit(payload)

    if (!sent) {
      setError('Unable to send order to the matching engine.')
      return
    }

    setError('')
  }

  const submitClass = side === 'BUY' ? 'submit-buy' : 'submit-sell'

  return (
    <section className="panel order-panel">
      <div className="panel-header compact-header">
        <span>ORDER</span>
      </div>

      <form className="order-form" onSubmit={handleSubmit}>
        <div className="segmented-row">
          {(['BUY', 'SELL'] as OrderSide[]).map((option) => (
            <button
              key={option}
              type="button"
              className={`segment ${side === option ? sideStyles[option] : ''}`}
              onClick={() => setSide(option)}
              aria-pressed={side === option}
            >
              {option}
            </button>
          ))}
        </div>

        <div className="segmented-row small">
          {(['LIMIT', 'MARKET'] as OrderType[]).map((option) => (
            <button
              key={option}
              type="button"
              className={`segment ${orderType === option ? 'active-subtle' : ''}`}
              onClick={() => setOrderType(option)}
              aria-pressed={orderType === option}
            >
              {option}
            </button>
          ))}
        </div>

        {orderType === 'LIMIT' && (
          <label className="field-group">
            <span className="field-label">Price</span>
            <input
              type="number"
              min="0"
              step="0.01"
              value={price}
              onChange={(event) => setPrice(event.target.value)}
              aria-label="Order price"
            />
          </label>
        )}

        <label className="field-group">
          <span className="field-label">Quantity</span>
          <input
            type="number"
            min="0"
            step="1"
            value={quantity}
            onChange={(event) => setQuantity(event.target.value)}
            aria-label="Order quantity"
          />
        </label>

        {orderType === 'LIMIT' && (
          <div className="total-box">
            <span>Total</span>
            <strong>{total.toLocaleString(undefined, { minimumFractionDigits: 2, maximumFractionDigits: 2 })}</strong>
          </div>
        )}

        {error && <div className="error-box">{error}</div>}

        <button type="submit" className={`submit-button ${submitClass}`}>
          PLACE {side} ORDER
        </button>
      </form>
    </section>
  )
}
