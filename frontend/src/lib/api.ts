const API_BASE = '/api/v1'

export const PAIRS = ['BTC/USDT', 'ETH/USDT', 'ETH/BTC', 'DOGE/USDT', 'DOGE/BTC']
export const DEFAULT_USER = '6239683678'

export interface OHLCPoint {
  date: string
  open: number
  high: number
  low: number
  close: number
}

export interface OHLCResponse {
  pair: string
  asks: OHLCPoint[]
  bids: OHLCPoint[]
}

export interface OrderLevel {
  price: number
  amount: number
}

export interface OrderBookResponse {
  pair: string
  bids: OrderLevel[]
  asks: OrderLevel[]
}

export interface WalletResponse {
  user_id: string
  balances: Record<string, number>
}

export interface OrderResponse {
  status: string
  product: string
  type: string
  price: number
  amount: number
}

export async function fetchOHLC(pair: string): Promise<OHLCResponse> {
  const res = await fetch(`${API_BASE}/analytics/ohlc?pair=${encodeURIComponent(pair)}`)
  if (!res.ok) throw new Error(`OHLC fetch failed (${res.status})`)
  return res.json()
}

export async function fetchOrderBook(pair: string): Promise<OrderBookResponse> {
  const res = await fetch(`${API_BASE}/orderbook?pair=${encodeURIComponent(pair)}`)
  if (!res.ok) throw new Error(`Order book fetch failed (${res.status})`)
  return res.json()
}

export async function fetchWallet(userId: string): Promise<WalletResponse> {
  const res = await fetch(`${API_BASE}/wallet?user_id=${encodeURIComponent(userId)}`)
  if (!res.ok) throw new Error(`Wallet fetch failed (${res.status})`)
  return res.json()
}

export async function placeOrder(order: {
  user_id: string
  product: string
  type: string
  price: number
  amount: number
}): Promise<OrderResponse> {
  const res = await fetch(`${API_BASE}/orders`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(order),
  })
  const text = await res.text()
  try {
    const data = JSON.parse(text)
    if (!res.ok) throw new Error(data.message ?? text)
    return data
  } catch (e) {
    if (e instanceof SyntaxError) throw new Error(text)
    throw e
  }
}

/** Format a price with adaptive precision based on magnitude. */
export function formatPrice(n: number): string {
  if (n >= 100) return n.toFixed(2)
  if (n >= 1) return n.toFixed(4)
  if (n >= 0.01) return n.toFixed(6)
  return n.toFixed(8)
}

/** Format an amount with adaptive precision. */
export function formatAmount(n: number): string {
  if (n >= 1000) return n.toFixed(2)
  if (n >= 1) return n.toFixed(4)
  return n.toFixed(6)
}

/** Format a wallet balance, trimming unnecessary trailing zeros. */
export function formatBalance(n: number): string {
  return n.toLocaleString('en-US', { maximumFractionDigits: 8 })
}
