import type { Order } from '../../types/trading'

interface OpenOrdersProps {
  orders: Order[]
  onCancel: (orderId: number) => void
}

export default function OpenOrders({ orders, onCancel }: OpenOrdersProps) {
  return (
    <section className="panel table-panel">
      <div className="panel-header compact-header">
        <span>OPEN ORDERS</span>
      </div>

      {orders.length === 0 ? (
        <div className="empty-state">No open orders</div>
      ) : (
        <div className="table-scroll">
          <table className="terminal-table compact-table">
            <thead>
              <tr>
                <th>ID</th>
                <th>SIDE</th>
                <th>TYPE</th>
                <th>PRICE</th>
                <th>QTY</th>
                <th>REMAINING</th>
                <th>STATUS</th>
                <th>ACTION</th>
              </tr>
            </thead>
            <tbody>
              {orders.map((order) => (
                <tr key={order.id}>
                  <td>{order.id}</td>
                  <td className={order.side === 'BUY' ? 'buy-text' : 'sell-text'}>{order.side}</td>
                  <td>{order.type}</td>
                  <td className="numeric">{order.price !== null ? order.price.toFixed(2) : '--'}</td>
                  <td className="numeric">{order.quantity}</td>
                  <td className="numeric">{order.remaining}</td>
                  <td>{order.status}</td>
                  <td>
                    <button type="button" className="cancel-button" onClick={() => onCancel(order.id)}>
                      CANCEL
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </section>
  )
}
