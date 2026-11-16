'use client';

export default function RelayControl() {
  const relays = [
    { id: 1, name: 'Quarto' },
    { id: 2, name: 'Exaustor' },
    { id: 3, name: 'Ventilador' },
    { id: 4, name: 'Sala' },
    { id: 5, name: 'Bancada' },
    { id: 6, name: 'Jardim F.' },
  ];

  // Apenas uma única função assíncrona para lidar com o clique
  const toggleRelay = async (id: number) => {
    console.log(`Alternando estado do relé ID: ${id}`);
    
    try {
      const response = await fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ device: 'relay', targetId: id, action: 'toggle' }),
      });
      
      if (response.ok) {
        console.log(`Comando para o relé ${id} enviado com sucesso!`);
      } else {
        console.error(`Erro do servidor ao tentar alterar o relé ${id}`);
      }
    } catch (error) {
      console.error("Falha ao comunicar com o servidor de controle", error);
    }
  };

  return (
    <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-6 gap-4">
      {relays.map((relay) => (
        <div key={relay.id} className="bg-[#151f32] p-4 rounded-xl border border-slate-800 text-center flex flex-col justify-between space-y-3">
          <span className="text-sm font-semibold text-slate-300">{relay.name}</span>
          <button 
            onClick={() => toggleRelay(relay.id)}
            className="bg-blue-600 hover:bg-blue-500 active:bg-blue-700 text-xs py-2 px-3 rounded-lg font-bold transition-colors w-full"
          >
            Ligar/Desligar
          </button>
        </div>
      ))}
    </div>
  );
}
