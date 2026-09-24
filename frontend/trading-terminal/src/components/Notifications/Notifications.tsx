import type { Notification } from '../../types/trading'

interface NotificationsProps {
  notifications: Notification[]
}

export default function Notifications({ notifications }: NotificationsProps) {
  return (
    <div className="notifications" aria-live="polite">
      {notifications.map((notification) => (
        <div className={`notification ${notification.tone}`} key={notification.id}>
          <strong>{notification.title}</strong>
          <span>{notification.message}</span>
        </div>
      ))}
    </div>
  )
}
