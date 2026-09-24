import { CartesianGrid, Line, LineChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts'
import type { ChartPoint } from '../../types/trading'

interface PriceChartProps {
  data: ChartPoint[]
}

export default function PriceChart({ data }: PriceChartProps) {
  return (
    <section className="panel chart-panel">
      <div className="panel-header">
        <span>PRICE CHART</span>
      </div>

      {data.length === 0 ? (
        <div className="empty-state">Waiting for trade data...</div>
      ) : (
        <div className="chart-wrap">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data} margin={{ top: 8, right: 12, bottom: 4, left: 0 }}>
              <CartesianGrid stroke="#1c2a3d" strokeDasharray="3 3" vertical={false} />
              <XAxis dataKey="time" axisLine={false} tickLine={false} tick={{ fill: '#8493a8', fontSize: 10 }} minTickGap={14} />
              <YAxis
                domain={['dataMin - 1', 'dataMax + 1']}
                axisLine={false}
                tickLine={false}
                tick={{ fill: '#8493a8', fontSize: 10 }}
                tickFormatter={(value) => Number(value).toFixed(2)}
                width={42}
              />
              <Tooltip
                contentStyle={{
                  background: '#101b2e',
                  border: '1px solid #1c2a3d',
                  borderRadius: '4px',
                  color: '#f5f7fa',
                }}
                formatter={(value) => [`${Number(value ?? 0).toFixed(2)}`, 'Price']}
              />
              <Line type="monotone" dataKey="price" stroke="#67e8f9" strokeWidth={2} dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}
    </section>
  )
}
