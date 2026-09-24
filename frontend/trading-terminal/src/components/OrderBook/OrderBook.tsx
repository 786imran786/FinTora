import type { OrderBookState } from '../../types/trading'

interface OrderBookProps {
  book: OrderBookState
}

export default function OrderBook({ book }: OrderBookProps) {
  const maxDepth = Math.max(...[...book.asks, ...book.bids].map((level) => level.quantity), 1)

  return (
    <section className="panel orderbook-panel">
      <div className="panel-header">
        <span>ORDER BOOK</span>
      </div>

      <div className="book-table">
        <div className="book-header-row">
          <span>PRICE</span>
          <span>QTY</span>
        </div>

        <div className="book-group">
          {book.asks.slice(0, 5).map((level) => {
            const width = `${(level.quantity / maxDepth) * 100}%`
            const isBest = book.bestAsk !== null && Math.abs(level.price - book.bestAsk) < 0.0001
            return (
              <div key={`ask-${level.price}`} className={`book-row ${isBest ? 'best-ask' : ''}`}>
                <div className="depth-fill ask" style={{ width }} />
                <span className="price sell">{level.price.toFixed(2)}</span>
                <span className="quantity">{level.quantity}</span>
              </div>
            )
          })}
        </div>

        <div className="spread-row">
          <span>SPREAD</span>
          <strong>{book.spread.toFixed(2)}</strong>
        </div>

        <div className="book-group">
          {book.bids.slice(0, 5).map((level) => {
            const width = `${(level.quantity / maxDepth) * 100}%`
            const isBest = book.bestBid !== null && Math.abs(level.price - book.bestBid) < 0.0001
            return (
              <div key={`bid-${level.price}`} className={`book-row ${isBest ? 'best-bid' : ''}`}>
                <div className="depth-fill bid" style={{ width }} />
                <span className="price buy">{level.price.toFixed(2)}</span>
                <span className="quantity">{level.quantity}</span>
              </div>
            )
          })}
        </div>
      </div>
    </section>
  )
}
