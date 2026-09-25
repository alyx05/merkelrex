import { useEffect, useRef, useState } from 'react'
import { createChart, ColorType, IChartApi, ISeriesApi } from 'lightweight-charts'
import { fetchOHLC } from '../lib/api'

export function TradingChart({ pair, refreshKey }: { pair: string; refreshKey: number }) {
  const containerRef = useRef<HTMLDivElement>(null)
  const chartRef = useRef<IChartApi | null>(null)
  const seriesRef = useRef<ISeriesApi<'Candlestick'> | null>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  // Create chart once
  useEffect(() => {
    if (!containerRef.current) return

    const chart = createChart(containerRef.current, {
      layout: {
        background: { type: ColorType.Solid, color: '#0f172a' },
        textColor: '#94a3b8',
        fontFamily: 'system-ui, sans-serif',
      },
      grid: {
        vertLines: { color: '#1e293b' },
        horzLines: { color: '#1e293b' },
      },
      crosshair: { mode: 1 },
      rightPriceScale: { borderColor: '#1e293b' },
      timeScale: { borderColor: '#1e293b', timeVisible: false },
      width: containerRef.current.clientWidth,
      height: 380,
    })
    chartRef.current = chart

    const series = chart.addCandlestickSeries({
      upColor: '#22c55e',
      downColor: '#ef4444',
      borderUpColor: '#22c55e',
      borderDownColor: '#ef4444',
      wickUpColor: '#22c55e',
      wickDownColor: '#ef4444',
    })
    seriesRef.current = series

    const ro = new ResizeObserver((entries) => {
      if (chartRef.current) {
        chartRef.current.applyOptions({ width: entries[0].contentRect.width })
      }
    })
    ro.observe(containerRef.current)

    return () => {
      ro.disconnect()
      chart.remove()
      chartRef.current = null
      seriesRef.current = null
    }
  }, [])

  // Fetch data when pair or refreshKey changes
  useEffect(() => {
    if (!seriesRef.current || !chartRef.current) return
    setLoading(true)
    setError(null)

    fetchOHLC(pair)
      .then((data) => {
        // Use asks side for the candlestick series, convert date -> ISO string
        const candles = data.asks.map((p) => ({
          time: p.date.replace(/\//g, '-') as any,
          open: p.open,
          high: p.high,
          low: p.low,
          close: p.close,
        }))
        seriesRef.current!.setData(candles)
        chartRef.current!.timeScale().fitContent()
        setLoading(false)
      })
      .catch((err) => {
        setError(err.message)
        setLoading(false)
      })
  }, [pair, refreshKey])

  return (
    <div className="bg-slate-900 rounded-xl border border-slate-800 p-4">
      <div className="flex items-center justify-between mb-3">
        <h2 className="text-slate-200 font-semibold">Price Chart — {pair}</h2>
        {loading && <span className="text-slate-500 text-sm">Loading…</span>}
      </div>
      <div ref={containerRef} className="w-full" />
      {error && <p className="text-red-400 text-sm mt-2">{error}</p>}
    </div>
  )
}
