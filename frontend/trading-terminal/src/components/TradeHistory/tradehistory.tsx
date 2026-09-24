	import type { Trade } from '../../types/trading'

	interface TradeHistoryProps {
		trades: Trade[]
	}

	export default function TradeHistory({ trades }: TradeHistoryProps) {
		return (
			<section className="panel table-panel">
				<div className="panel-header compact-header">
					<span>TRADE HISTORY</span>
				</div>

				{trades.length === 0 ? (
					<div className="empty-state">Waiting for trades...</div>
				) : (
					<div className="table-scroll">
						<table className="terminal-table compact-table">
							<thead>
								<tr>
									<th>TIME</th>
									<th>SIDE</th>
									<th>PRICE</th>
									<th>QTY</th>
								</tr>
							</thead>
							<tbody>
								{trades.slice(0, 8).map((trade) => (
									<tr key={`${trade.id}-${trade.time}`}>
										<td>{trade.time}</td>
										<td className={trade.side === 'BUY' ? 'buy-text' : 'sell-text'}>{trade.side}</td>
										<td className="numeric">{trade.price.toFixed(2)}</td>
										<td className="numeric">{trade.quantity}</td>
									</tr>
								))}
							</tbody>
						</table>
					</div>
				)}
			</section>
		)
	}
    