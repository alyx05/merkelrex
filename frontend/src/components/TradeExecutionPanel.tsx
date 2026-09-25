import { useState, useEffect } from 'react'
import {
  placeOrder,
  fetchWallet,
  formatBalance,
  DEFAULT_USER,
} from '../lib/api'

export function TradeExecutionPanel({
  pair,
  onOrderPlaced,
}: {
  pair: string
  onOrderPlaced: () => void
}) {
  const [type, setType] = useState<'bid' | 'ask'>('bid')
  const [price, setPrice] = useState('')
  const [amount, setAmount] = useState('')
  const [wallet, setWallet] = useState<Record<string, number>>({})
  const [message, setMessage] = useState<string | null>(null)
  const [error, setError] = useState<string | null>(null)
  const [submitting, setSubmitting] = useState(false)

  const loadWallet = async () => {
    try {
      const data = await fetchWallet(DEFAULT_USER)
      setWallet(data.balances)
    } catch {
      /* silent */
    }
  }

  useEffect(() => {
    loadWallet()
  }, [])

  const baseCurrency = pair.split('/')[0]
  const quoteCurrency = pair.split('/')[1]

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setMessage(null)
    setError(null)
    setSubmitting(true)
    try {
      const result = await placeOrder({
        user_id: DEFAULT_USER,
        product: pair,
        type,
        price: parseFloat(price),
        amount: parseFloat(amount),
      })
      setMessage(`✓ ${result.status}: ${type.toUpperCase()} ${amount} ${baseCurrency} @ ${price} ${quoteCurrency}`)
      setPrice('')
      setAmount('')
      loadWallet()
      onOrderPlaced()
    } catch (err) {
      setError((err as Error).message)
    } finally {
      setSubmitting(false)
    }
  }

  return (
    <div className="bg-slate-900 rounded-xl border border-slate-800 p-4">
      <h2 className="text-slate-200 font-semibold mb-4">Trade Execution</h2>
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Order form */}
        <form onSubmit={handleSubmit} className="space-y-4">
          <div className="flex gap-2">
            <button
              type="button"
              onClick={() => setType('bid')}
              className={`flex-1 py-2 rounded-lg font-medium transition-colors ${
                type === 'bid' ? 'bg-emerald-500 text-white' : 'bg-slate-800 text-slate-400 hover:bg-slate-700'
              }`}
            >
              Buy / Bid
            </button>
            <button
              type="button"
              onClick={() => setType('ask')}
              className={`flex-1 py-2 rounded-lg font-medium transition-colors ${
                type === 'ask' ? 'bg-red-500 text-white' : 'bg-slate-800 text-slate-400 hover:bg-slate-700'
              }`}
            >
              Sell / Ask
            </button>
          </div>

          <div>
            <label className="text-xs text-slate-500">Pair</label>
            <div className="mt-1 text-slate-200 font-mono">{pair}</div>
          </div>

          <div>
            <label className="text-xs text-slate-500">Price ({quoteCurrency})</label>
            <input
              type="number"
              step="any"
              value={price}
              onChange={(e) => setPrice(e.target.value)}
              required
              className="w-full mt-1 bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-slate-200 focus:outline-none focus:border-slate-500"
            />
          </div>

          <div>
            <label className="text-xs text-slate-500">Amount ({baseCurrency})</label>
            <input
              type="number"
              step="any"
              value={amount}
              onChange={(e) => setAmount(e.target.value)}
              required
              className="w-full mt-1 bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-slate-200 focus:outline-none focus:border-slate-500"
            />
          </div>

          <button
            type="submit"
            disabled={submitting}
            className={`w-full py-2.5 rounded-lg font-semibold disabled:opacity-50 transition-colors ${
              type === 'bid'
                ? 'bg-emerald-500 hover:bg-emerald-600 text-white'
                : 'bg-red-500 hover:bg-red-600 text-white'
            }`}
          >
            {submitting ? 'Placing…' : `Place ${type === 'bid' ? 'Buy' : 'Sell'} Order`}
          </button>

          {message && <p className="text-emerald-400 text-sm">{message}</p>}
          {error && <p className="text-red-400 text-sm">{error}</p>}
        </form>

        {/* Wallet */}
        <div>
          <h3 className="text-slate-300 font-medium mb-3">Wallet Balances</h3>
          <div className="space-y-2">
            {Object.entries(wallet).length === 0 && (
              <p className="text-slate-500 text-sm">No balances found</p>
            )}
            {Object.entries(wallet).map(([currency, balance]) => (
              <div
                key={currency}
                className="flex justify-between items-center bg-slate-800 rounded-lg px-3 py-2"
              >
                <span className="text-slate-300 font-medium">{currency}</span>
                <span className="text-slate-400 font-mono">{formatBalance(balance)}</span>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  )
}
