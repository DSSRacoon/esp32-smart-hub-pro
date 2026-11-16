"use client";

import { useState, useEffect } from "react";
import Link from 'next/link';
import Header from "@/components/Header";
import SensorCard from "@/components/SensorCard";
import RelayControl from "@/components/RelayControl";
import DeviceControl from "@/components/DeviceControl";
import HistoryChart from "@/components/HistoryChart";

// Tipagem rigorosa para as chaves recebidas do banco PostgreSQL
interface SensorData {
  temp: number;
  pres: number;
  energy: number;
  prediction: number;
  previsao?: string;
}

// Tipagem estruturada para a timeline do gráfico do Shadcn
interface ChartPoint {
  hora: string;
  temp: number;
  energia: number;
}

export default function DashboardPage() {
  // Estados necessários declarados corretamente para evitar erros de compilação
  const [cards, setCards] = useState<SensorData>({
    temp: 0,
    pres: 0,
    energy: 0,
    prediction: 0,
    previsao: "Sincronizando...",
  });
  const [chartData, setChartData] = useState<ChartPoint[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(false);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch('/api/sensor');
        if (!response.ok) throw new Error('Erro na requisição');
        
        const data = await response.json();
        
        // Sincroniza os estados com os dados reais mapeados do Back-End
        setCards(data.cards); 
        setChartData(data.grafico);
        setError(false);
      } catch (err) {
        console.error("Erro ao sincronizar com o banco PostgreSQL:", err);
        setError(true);
      } finally {
        setLoading(false);
      }
    };

    fetchData();
    const interval = setInterval(fetchData, 5000); // Polling em tempo real a cada 5 segundos
    return () => clearInterval(interval);
  }, []);

  // Tela de transição limpa para impedir que o React tente ler dados nulos no primeiro frame
  if (loading) {
    return (
      <div className="min-h-screen bg-[#0f172a] text-white flex flex-col items-center justify-center space-y-4">
        <div className="w-12 h-12 border-4 border-t-blue-500 border-slate-700 rounded-full animate-spin"></div>
        <p className="text-slate-400 font-medium tracking-wide">Carregando painel de automação...</p>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-[#0f172a] text-white pb-12 font-sans selection:bg-blue-500">
      <Header title="ESP32 SMART HUB PRO" />

      {/* Barra de Navegação Superior */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 mt-4 flex justify-end">
        <Link 
          href="/automacoes" 
          className="bg-slate-800 hover:bg-slate-700 text-slate-200 border border-slate-700 rounded-xl px-4 py-2 text-xs font-bold tracking-wide uppercase shadow-lg transition-all duration-300 flex items-center gap-2 hover:-translate-y-0.5"
        >
          <svg className="w-4 h-4 text-purple-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2.5" d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4" />
          </svg>
          Gerenciar Automações
        </Link>
      </div>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 mt-4 space-y-8">
        
        {/* Banner de Fallback caso a API do Next perca a comunicação com o PostgreSQL */}
        {error && (
          <div className="bg-red-950/50 border border-red-800 text-red-200 p-4 rounded-xl text-sm font-medium animate-pulse text-center">
            ⚠️ Alerta: Falha na conexão com o banco PostgreSQL. Exibindo dados locais de segurança.
          </div>
        )}
        
        {/* Seção 1: Grid de Sensores (Chaves de estado e formatação fixadas) */}
        <section className="grid grid-cols-2 md:grid-cols-4 gap-4">
          <SensorCard 
            title="Temperatura" 
            value={cards?.temp ? `${cards.temp.toFixed(1)}°C` : '--°C'} 
            color="text-red-500"
            icon={<path d="M15,13V5A3,3 0 0,0 12,2A3,3 0 0,0 9,5V13A5,5 0 1,0 15,13M12,4A1,1 0 0,1 13,5V8H11V5A1,1 0 0,1 12,4Z"/>}
          />
          <SensorCard 
            title="Pressão ATM" 
            value={cards?.pres ? `${cards.pres.toFixed(1)} hPa` : '-- hPa'} 
            color="text-blue-500"
            icon={<path d="M12,20A6,6 0 0,1 6,14C6,10 12,3.25 12,3.25C12,3.25 18,10 18,14A6,6 0 0,1 12,20Z"/>}
            subtitle={cards?.previsao ? cards.previsao : "Sincronizando..."}
          />
          <SensorCard 
            title="Energia" 
            value={cards?.energy ? `${cards.energy.toFixed(0)}W` : '--W'} 
            color="text-amber-400"
            icon={<path d="M7,2V13H10V22L17,10H13L17,2H7Z"/>}
          />
          <SensorCard 
            title="Previsão" 
            value={cards?.prediction ? `${cards.prediction.toFixed(1)}°C` : '--°C'} 
            color="text-purple-500"
            icon={<path d="M12,2L4.5,20.29L5.21,21L12,18L18.79,21L19.5,20.29L12,2Z"/>}
          />
        </section>

        {/* Seção 2: Controles Infravermelho (Ar e TV) */}
        <section className="grid grid-cols-1 md:grid-cols-2 gap-6">
          <DeviceControl type="ac" title="Ar Condicionado" />
          <DeviceControl type="tv" title="Televisão" />
        </section>

        {/* Seção 3: Grid de Relés */}
        <section className="bg-[#1e293b] p-6 rounded-2xl shadow-xl border border-slate-800">
          <h2 className="text-xl font-bold mb-4 text-slate-400 tracking-wide uppercase text-sm">Controle de Ambientes (Relés)</h2>
          <RelayControl />
        </section>

        {/* Seção 4: Gráfico dinâmico do Shadcn/ui passando a propriedade com dados reais */}
        <HistoryChart data={chartData} />

      </main>
    </div>
  );
}
