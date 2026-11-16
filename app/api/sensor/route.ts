import { NextResponse } from "next/server";
import prisma from "@/lib/prisma";

const IP_DO_ESP32 = "http://192.168.100.3"; // O IP real do seu ESP32 na sua rede local

export async function GET() {
  try {
    // O Next.js toma a iniciativa: ele liga para o ESP32 e busca a leitura atual!
    const response = await fetch(`${IP_DO_ESP32}/sensor`, { signal: AbortSignal.timeout(3000) });
    const dadosESP = await response.json();

    // Salva no banco PostgreSQL em segundo plano
    await prisma.telemetria.create({
      data: {
        temp: parseFloat(dadosESP.temp),
        pres: parseFloat(dadosESP.pres),
        energy: parseFloat(dadosESP.energy),
      },
    });

    // Lógica para rodar dentro da sua função GET() logo após salvar no banco PostgreSQL [11.49 AM]
async function processarAutomacoes(tempAtual: number) {
  try {
    // Busca todas as automações ativas no banco de dados
    const regras = await prisma.automacao.findMany({ where: { ativa: true } });

    for (const regra of regras) {
      if (regra.sensor === "temp") {
        // Regra: Ligar se a temperatura for maior que o limite (ex: > 25°C)
        if (regra.gatilho === "maior_que" && tempAtual > regra.valor) {
          console.log(`[AUTOMAÇÃO] ${regra.nome} disparada!`);
          
          // O próprio Back-End simula o clique enviando para a rota de controle
          await fetch("http://localhost:3000/api/control", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ device: regra.acaoDevice, command: regra.acaoComando })
          });
        }
      }
    }
  } catch (err) {
    console.error("Erro no motor de automação:", err);
  }
}


    // Pega o histórico do banco para desenhar o gráfico do Shadcn/ui
    const historico = await prisma.telemetria.findMany({
      take: 10,
      orderBy: { criadoEm: "asc" },
    });

    return NextResponse.json({
      cards: dadosESP,
      grafico: historico.map((h) => ({
        hora: h.criadoEm.toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" }),
        temp: h.temp,
        energia: h.energy,
      })),
    });
  } catch (error) {
    // Se o ESP32 estiver offline, ele busca a última leitura salva no banco para não quebrar a tela
    const ultimoRegistro = await prisma.telemetria.findFirst({ orderBy: { criadoEm: "desc" } });
    return NextResponse.json({ cards: ultimoRegistro, grafico: [] });
  }
}
