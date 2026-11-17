'use client';

import { useState } from 'react';

interface DeviceProps {
  title: string;
  type: 'ac' | 'tv';
}

export default function DeviceControl({ title, type }: DeviceProps) {
  const [status, setStatus] = useState<string | null>(null);

  const handleIRAction = async (action: 'learn' | 'send', command: string) => {
    setStatus(action === 'send' ? `Disparando comando ${command}...` : `Modo aprender ativo para ${command}...`);
    
    try {
      // ✅ CORRIGIDO: O fetch agora roda tanto para 'send' quanto para 'learn'
      const response = await fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ 
          device: type, 
          command: command,
          action: action //// 👈 Passa "learn" ou "send" dinamicamente conforme o botão clicado! Envia "learn" ou "send" 
          //dinamicamente para o Back-End
        }),
      });

      if (response.ok) {
        if (action === 'send') {
          setStatus(`Comando ${command} disparado com sucesso!`);
        } else {
          setStatus(`✓ Comando ${command} aprendido e gravado no PostgreSQL!`);
        }
      } else if (response.status === 404) {
        setStatus(`Erro: Comando ${command} não cadastrado no PostgreSQL.`);
      } else if (response.status === 408) {
        setStatus(`⚠️ Tempo esgotado! O ESP32 não detectou nenhum controle remoto.`);
      } else {
        setStatus('Erro ao processar comando no servidor.');
      }

    } catch (error) {
      console.error("Erro na comunicação com a API de controle:", error);
      setStatus('Falha na comunicação com a API.');
    }


    setTimeout(() => setStatus(null), 3000);
  };

  return (
    <div className="bg-[#1e293b] p-6 rounded-2xl shadow-lg border border-slate-800 flex flex-col justify-between space-y-4">
      <div>
        <h3 className="text-lg font-bold text-slate-200">{title}</h3>
        {status && <p className="text-xs text-blue-400 mt-1 font-medium animate-pulse">{status}</p>}
      </div>
      
      {/* Grid inteligente: Se for Ar Condicionado, exibe 3 colunas. Se for TV, exibe 2 colunas */}
      <div className={`grid gap-3 ${type === 'ac' ? 'grid-cols-3' : 'grid-cols-2'}`}>
        
        {/* BOTÕES DE APRENDIZADO (LEVE OPACIDADE) */}
        <button 
          onClick={() => handleIRAction('learn', `${type}_on`)}
          className="bg-slate-700 hover:bg-slate-600 text-[10px] sm:text-xs py-2 px-2 rounded-xl font-semibold transition-colors opacity-80"
        >
          Aprender Ligar
        </button>
        <button 
          onClick={() => handleIRAction('learn', `${type}_off`)}
          className="bg-slate-700 hover:bg-slate-600 text-[10px] sm:text-xs py-2 px-2 rounded-xl font-semibold transition-colors opacity-80"
        >
          Aprender Desligar
        </button>

        {type === 'ac' && (
          <button 
            onClick={() => handleIRAction('learn', 'ac_23')}
            className="bg-slate-700 hover:bg-slate-600 text-[10px] sm:text-xs py-2 px-2 rounded-xl font-semibold transition-colors opacity-80"
          >
            Aprender 23°C
          </button>
        )}

        {/* BOTÕES DE DISPARO REAL (STAIN 200) */}
        <button 
          onClick={() => handleIRAction('send', `${type}_on`)}
          className="bg-blue-600 hover:bg-blue-500 py-2.5 px-4 rounded-xl font-bold transition-colors text-sm"
        >
          Ligar
        </button>
        <button 
          onClick={() => handleIRAction('send', `${type}_off`)}
          className="bg-red-600 hover:bg-red-500 py-2.5 px-4 rounded-xl font-bold transition-colors text-sm"
        >
          Desligar
        </button>

        {type === 'ac' && (
          <button 
            onClick={() => handleIRAction('send', 'ac_23')}
            className="bg-amber-600 hover:bg-amber-500 py-2.5 px-4 rounded-xl font-bold transition-colors text-sm"
          >
            23°C / On-Off
          </button>
        )}
      </div>
    </div>
  );
}
