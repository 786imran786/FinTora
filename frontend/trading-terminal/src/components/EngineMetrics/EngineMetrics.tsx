import type { Metrics } from '../../types/trading'

interface EngineMetricsProps {
  metrics: Metrics
}

export default function EngineMetrics({ metrics }: EngineMetricsProps) {
  const items = [
    ['ORDERS PROCESSED', metrics.ordersProcessed.toLocaleString()],
    ['TRADES EXECUTED', metrics.tradesExecuted.toLocaleString()],
    ['ORDERS / SEC', metrics.ordersPerSecond.toLocaleString()],
    ['ACTIVE ORDERS', metrics.activeOrders.toLocaleString()],
    ['TOTAL VOLUME', metrics.totalVolume.toLocaleString()],
  ]

  return (
    <section className="metrics-strip" aria-label="Engine metrics">
      {items.map(([label, value]) => (
        <div className="metric-item" key={label}>
          <span>{label}</span>
          <strong>{value}</strong>
        </div>
      ))}
    </section>
  )
}
