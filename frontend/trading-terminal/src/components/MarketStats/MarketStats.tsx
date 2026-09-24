import type { MarketStats as MarketStatsType } from '../../types/trading'

interface MarketStatsProps {
  stats: MarketStatsType
}

const formatNumber = (value: number, digits = 2) =>
  Number.isInteger(value)
    ? value.toLocaleString()
    : value.toLocaleString(undefined, { minimumFractionDigits: digits, maximumFractionDigits: digits })

export default function MarketStats({ stats }: MarketStatsProps) {
  const items = [
    { label: 'LAST', value: formatNumber(stats.lastPrice, 2) },
    { label: 'BEST BID', value: formatNumber(stats.bestBid, 2) },
    { label: 'BEST ASK', value: formatNumber(stats.bestAsk, 2) },
    { label: 'SPREAD', value: formatNumber(stats.spread, 2) },
    { label: 'VOLUME', value: stats.volume.toLocaleString() },
  ]

  return (
    <div className="market-strip">
      {items.map((item) => (
        <div key={item.label} className="market-tile">
          <div className="mini-label">{item.label}</div>
          <div className="mini-value">{item.value}</div>
        </div>
      ))}
    </div>
  )
}
