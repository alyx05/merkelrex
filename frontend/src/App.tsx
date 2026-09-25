import { useState } from 'react'
import { TradingChart } from './components/TradingChart'
import { OrderBookDepth } from './components/OrderBookDepth'
import { TradeExecutionPanel } from './components/TradeExecutionPanel'
import { PAIRS } from './lib/api'

export default function App() {
  const [pair, setPair] = useState('BTC/USDT')
  const [refreshKey, setRefreshKey] = useState(0)

  return (
    <div className="min-h-screen bg-slate-950 text-slate-200">
      <header className="border-b border-slate-800 px-4 md:px-6 py-4 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 bg-emerald-500 rounded-lg flex items-center justify-center font-bold text-white">
            M
          </div>
          <h1 className="text-lg font-bold">Merkelrex Dashboard</h1>
        </div>
        <div className="flex items-center gap-3">
          <label className="text-sm text-slate-500 hidden sm:inline">Pair:</label>
          <select
            value={pair}
            onChange={(e) => setPair(e.target.value)}
            className="bg-slate-800 border border-slate-700 rounded-lg px-3 py-1.5 text-slate-200 focus:outline-none focus:border-slate-500"
          >
            {PAIRS.map((p) => (
              <option key={p} value={p}>
                {p}
              </option>
            ))}
          </select>
        </div>
      </header>

      <main className="p-4 md:p-6 space-y-6">
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <div className="lg:col-span-2">
            <TradingChart pair={pair} refreshKey={refreshKey} />
          </div>
          <div>
            <OrderBookDepth pair={pair} />
          </div>
        </div>
        <div>
          <TradeExecutionPanel
            pair={pair}
            onOrderPlaced={() => setRefreshKey((k) => k + 1)}
          />
        </div>
      </main>
    </div>
  )
}
