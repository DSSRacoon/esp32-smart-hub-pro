'use client';

import { useState, useEffect, type SubmitEvent as ReactSubmitEvent } from 'react';
import Header from '@/components/Header';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";

import Link from 'next/link';

interface Rule {
  id: number;
  nome: string;
  sensor: string;
  gatilho: string;
  valor: number;
  acaoComando: string;
  ativa: boolean;
}

export default function AutomacoesPage() {
  const [regras, setRegras] = useState<Rule[]>([]);
  const [nome, setNome] = useState('');
  const [valor, setValor] = useState('');
  const [comando, setComando] = useState('ac_on');

  // Carrega as automações do banco de dados
  const fetchRegras = async (): Promise<Rule[]> => {
    const res = await fetch('/api/rules');
    if (!res.ok) {
      throw new Error(`Falha ao buscar regras: HTTP ${res.status}`);
    }
    return res.json();
  };

  useEffect(() => {
    let active = true;

    fetchRegras()
      .then((data) => {
        if (active) {
          setRegras(data);
        }
      })
      .catch((err) => {
        if (active) {
          console.error("Erro ao buscar regras:", err);
        }
      });

    return () => {
      active = false;
    };
  }, []);

  // Envia uma nova regra para o PostgreSQL
  const handleSubmit = async (e: ReactSubmitEvent<HTMLFormElement>) => {
    e.preventDefault();
    if (!nome || !valor) return;

    try {
      const res = await fetch('/api/rules', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          nome,
          sensor: 'temp',
          gatilho: 'maior_que',
          valor: parseFloat(valor),
          acaoDevice: 'ac',
          acaoComando: comando,
        }),
      });

      if (res.ok) {
        setNome('');
        setValor('');
        setRegras(await fetchRegras()); // Atualiza a lista na tela
      }
    } catch (err) {
      console.error("Erro ao salvar automação:", err);
    }
  };

  // Liga/Desliga uma automação
  const toggleRegra = async (id: number, estadoAtual: boolean) => {
    try {
      await fetch('/api/rules', {
        method: 'PATCH',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id, ativa: !estadoAtual }),
      });
      setRegras(await fetchRegras());
    } catch (err) {
      console.error("Erro ao alterar status da regra:", err);
    }
  };

  return (
    <div className="min-h-screen bg-[#0f172a] text-white pb-12 font-sans">
      <Header title="GERENCIADOR DE AUTOMAÇÕES" />

      <main className="max-w-4xl mx-auto px-4 mt-8 space-y-8">

        {/* Botão para voltar para a Home */}
      <div className="flex justify-start">
        <Link 
          href="/" 
          className="text-xs font-bold text-slate-400 hover:text-white uppercase tracking-wider flex items-center gap-2 transition-colors"
        >
          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2.5" d="M10 19l-7-7m0 0l7-7m-7 7h18" />
          </svg>
          Voltar para o Painel
        </Link>
      </div>
        
        {/* Formulário de Criação */}
        <Card className="bg-[#1e293b] border-slate-800 text-white shadow-xl">
          <CardHeader>
            <CardTitle className="text-lg font-bold text-slate-200">Novo Gatilho Inteligente</CardTitle>
            <CardDescription className="text-slate-400">Configure regras baseadas em software para o seu hardware</CardDescription>
          </CardHeader>
          <CardContent>
            <form onSubmit={handleSubmit} className="space-y-4">
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <input 
                  type="text" 
                  placeholder="Nome ex: Ligar Ar no Calor"
                  value={nome}
                  onChange={(e) => setNome(e.target.value)}
                  className="bg-[#0f172a] border border-slate-800 rounded-xl px-4 py-2.5 text-sm focus:outline-none focus:border-blue-500"
                />
                <input 
                  type="number" 
                  step="0.1"
                  placeholder="Se Temp for maior que (°C)"
                  value={valor}
                  onChange={(e) => setValor(e.target.value)}
                  className="bg-[#0f172a] border border-slate-800 rounded-xl px-4 py-2.5 text-sm focus:outline-none focus:border-blue-500"
                />
                <select 
                  value={comando}
                  onChange={(e) => setComando(e.target.value)}
                  className="bg-[#0f172a] border border-slate-800 rounded-xl px-4 py-2.5 text-sm focus:outline-none focus:border-blue-500 text-slate-300"
                >
                  <option value="ac_on">Disparar: Ligar Ar</option>
                  <option value="ac_off">Disparar: Desligar Ar</option>
                  <option value="tv_on">Disparar: Ligar TV</option>
                </select>
              </div>
              <Button type="submit" className="w-full bg-emerald-600 hover:bg-emerald-500 font-bold py-2.5 rounded-xl transition-colors">
                Criar Automação Ativa
              </Button>
            </form>
          </CardContent>
        </Card>

        {/* Lista de Automações Criadas */}
        <div className="space-y-4">
          <h2 className="text-sm font-bold text-slate-400 uppercase tracking-wider">Regras Ativas no PostgreSQL</h2>
          
          {regras.length === 0 ? (
            <p className="text-sm text-slate-500 text-center py-4">Nenhum gatilho de automação configurado ainda.</p>
          ) : (
            regras.map((regra) => (
              <div key={regra.id} className="bg-[#1e293b] p-5 rounded-2xl border border-slate-800 flex items-center justify-between shadow-lg">
                <div>
                  <h4 className="font-bold text-slate-200">{regra.nome}</h4>
                  <p className="text-xs text-slate-400 mt-1">
                    Condição: Se <span className="text-red-400 font-semibold">Temperatura</span> &gt; {regra.valor}°C ➔ Ação: <span className="text-blue-400 font-semibold">{regra.acaoComando}</span>
                  </p>
                </div>
                <button
                  onClick={() => toggleRegra(regra.id, regra.ativa)}
                  className={`px-4 py-1.5 rounded-xl font-bold text-xs transition-colors ${
                    regra.ativa ? 'bg-emerald-500/20 text-emerald-400 border border-emerald-500/30' : 'bg-slate-800 text-slate-400 border border-slate-700'
                  }`}
                >
                  {regra.ativa ? 'ATIVADO' : 'PAUSADO'}
                </button>
              </div>
            ))
          )}
        </div>
      </main>
    </div>
  );
}
