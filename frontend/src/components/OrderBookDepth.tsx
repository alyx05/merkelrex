import { useEffect, useState } from 'react'
import {
  fetchOrderBook,
  formatPrice,
  formatAmount,
  type OrderLevel,
} from '../lib/api'

export function OrderBookDepth({ pair }: { pair: string }) {
  const [bids, setBids] = useState<OrderLevel[]>([])
  const [asks, setAsks] = useState<OrderLevel[]>([])
  const [connected, setConnected] = useState(false)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    let ws: WebSocket | null = null

    const fetchData = async () => {
      try {
        const data = await fetchOrderBook(pair)
        setBids(data.bids)
        setAsks(data.asks)
        setError(null)
      } catch (err) {
        setError((err as Error).message)
      }
    }

    fetchData()

    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:'
    ws = new WebSocket(`${protocol}//${location.host}/ws/market-feed`)
    ws.onopen = () => {
      setConnected(true)
      fetchData() // re-sync on (re)connect in case we missed updates
    }
    ws.onclose = () => setConnected(false)
    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data)
        if (msg.type === 'orderbook' && msg.pair === pair) {
          setBids(msg.bids)
          setAsks(msg.asks)
          setError(null)
        }
      } catch {
        // ignore non-JSON or malformed messages
      }
    }

    return () => ws?.close()
  }, [pair])

  const maxAmount = Math.max(...bids.map((b) => b.amount), ...asks.map((a) => a.amount), 1)
  const sortedAsks = [...asks].sort((a, b) => b.price - a.price)
  const sortedBids = [...bids].sort((a, b) => b.price - a.price)
  const bestBid = sortedBids[0]?.price ?? 0
  const bestAsk = sortedAsks[sortedAsks.length - 1]?.price ?? 0
  const spread = bestAsk > 0 && bestBid > 0 ? bestAsk - bestBid : 0

  return (
    <div className="bg-slate-900 rounded-xl border border-slate-800 p-4 flex flex-col">
      <div className="flex items-center justify-between mb-3">
        <h2 className="text-slate-200 font-semibold">Order Book</h2>
        <span
          className={`text-xs px-2 py-1 rounded-full ${
            connected ? 'bg-emerald-500/20 text-emerald-400' : 'bg-slate-700 text-slate-400'
          }`}
        >
          {connected ? '● Live' : '○ Offline'}
        </span>
      </div>

      <div className="grid grid-cols-2 text-xs text-slate-500 mb-1 px-2">
        <span>Price</span>
        <span className="text-right">Amount</span>
      </div>

      <div className="flex-1 overflow-y-auto max-h-[380px]">
        {sortedAsks.map((level, i) => (
          <DepthRow key={`ask-${i}`} level={level} maxAmount={maxAmount} side="ask" />
        ))}

        <div className="py-2 px-2 text-center text-sm border-y border-slate-800 my-1">
          <span className="text-slate-500">Spread </span>
          <span className="text-slate-300 font-mono">{formatPrice(spread)}</span>
        </div>

        {sortedBids.map((level, i) => (
          <DepthRow key={`bid-${i}`} level={level} maxAmount={maxAmount} side="bid" />
        ))}
      </div>

      {error && <p className="text-red-400 text-sm mt-2">{error}</p>}
      {bids.length === 0 && asks.length === 0 && !error && (
        <p className="text-slate-500 text-sm text-center py-4">No data</p>
      )}
    </div>
  )
}

function DepthRow({
  level,
  maxAmount,
  side,
}: {
  level: OrderLevel
  maxAmount: number
  side: 'bid' | 'ask'
}) {
  const widthPercent = Math.min((level.amount / maxAmount) * 100, 100)
  const barColor = side === 'bid' ? 'bg-emerald-500/15' : 'bg-red-500/15'
  const textColor = side === 'bid' ? 'text-emerald-400' : 'text-red-400'

  return (
    <div className="relative grid grid-cols-2 px-2 py-1 text-sm font-mono">
      <div
        className={`absolute inset-y-0 right-0 ${barColor} rounded-sm`}
        style={{ width: `${widthPercent}%` }}
      />
      <span className={`relative ${textColor}`}>{formatPrice(level.price)}</span>
      <span className="relative text-right text-slate-300">{formatAmount(level.amount)}</span>
    </div>
  )
}
