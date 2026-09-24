import type { ConnectionStatus } from '../../types/trading'

interface HeaderProps {
  connectionStatus: ConnectionStatus
  pair: string
}

const statusDot: Record<ConnectionStatus, string> = {
  CONNECTED: '#22c55e',
  CONNECTING: '#f59e0b',
  DISCONNECTED: '#ef4444',
}

export default function Header({ connectionStatus, pair }: HeaderProps) {
  const statusText = {
    CONNECTED: 'CONNECTED',
    CONNECTING: 'CONNECTING',
    DISCONNECTED: 'DISCONNECTED',
  }[connectionStatus]

  return (
    <header className="header-shell">
      <div className="brand-block">
        <div className="brand-name">FINTORA</div>
        <div className="brand-subtitle">REAL-TIME TRADING TERMINAL</div>
      </div>

      <div className="header-pair">{pair}</div>

      <div className="connection-pill" aria-live="polite">
        <span className="status-dot" style={{ backgroundColor: statusDot[connectionStatus] }} />
        <span>{statusText}</span>
      </div>
    </header>
  )
}
