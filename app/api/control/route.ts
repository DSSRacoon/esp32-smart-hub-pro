import { NextResponse } from "next/server";
import { PrismaClient } from "@prisma/client";

const globalForPrisma = global as unknown as { prisma: PrismaClient };
const prisma = globalForPrisma.prisma || new PrismaClient();
if (process.env.NODE_ENV !== "production") globalForPrisma.prisma = prisma;

// Certifique-se de colocar o IP correto que está aparecendo no visor LCD do seu ESP32!
const IP_ESP32 = "http://192.168.100.3"; 

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const { device, command, targetId } = body; 

    console.log(`[API Control] Nova requisição de clique:`, { device, command, targetId });

    // 1. CONTROLE DOS RELÉS FÍSICOS
    if (device === "relay" || targetId) {
      const idRele = parseInt(targetId);
      
      // Next.js liga diretamente para a rota física do ESP32 para bater o relé
      const res = await fetch(`${IP_ESP32}/relay?id=${idRele}`, {
        method: "GET",
        signal: AbortSignal.timeout(3000), 
      });
      
      if (res.ok) {
        return NextResponse.json({ success: true, msg: `Relé ${idRele} alternado no hardware!` });
      }
    }

    // 2. CONTROLE DE INFRAVERMELHO (AR / TV)
    if (command) {
      // Busca a assinatura do código Hex salvo na tabela do PostgreSQL
      const comandoSalvo = await prisma.comandoIR.findUnique({
        where: { nome: command }
      });

      if (!comandoSalvo) {
        return NextResponse.json({ error: "Código IR não encontrado no banco PostgreSQL" }, { status: 404 });
      }

      // Envia a ordem para o ESP32 disparar o LED com o comando correspondente
      const res = await fetch(`${IP_ESP32}/send?cmd=${comandoSalvo.nome}`, { 
        method: "GET",
        signal: AbortSignal.timeout(3000) 
      });
      
      if (res.ok) {
        return NextResponse.json({ success: true, msg: `Sinal IR ${command} disparado fisicamente!` });
      }
    }

    return NextResponse.json({ error: "Dispositivo ou comando inválido" }, { status: 400 });
  } catch (error) {
    console.error("[API Control] Erro na comunicação com o hardware:", error);
    return NextResponse.json({ error: "O ESP32 não respondeu na rede local. Verifique o IP." }, { status: 500 });
  }
}
