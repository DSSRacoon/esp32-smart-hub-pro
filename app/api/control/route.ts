import { NextResponse } from "next/server";
import { PrismaClient } from "@prisma/client";

const globalForPrisma = global as unknown as { prisma: PrismaClient };
const prisma = globalForPrisma.prisma || new PrismaClient();
if (process.env.NODE_ENV !== "production") globalForPrisma.prisma = prisma;

const IP_ESP32 = "http://192.168.100.3"; 

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const { device, command, targetId, action } = body; 

    console.log(`[API Control] Nova requisição de clique:`, { device, command, targetId, action });

    // ============================================================================
    // CASO 1: CONTROLE DOS RELÉS FÍSICOS (Quarto, Exaustor, Sala, etc)
    // ============================================================================
    if (device === "relay" || typeof targetId !== "undefined") {
      const idRele = parseInt(targetId);
      
      try {
        // Envia o clique direto para o WebServer enxuto do ESP32 bater o relé
        const res = await fetch(`${IP_ESP32}/relay?id=${idRele}`, {
          method: "GET",
          signal: AbortSignal.timeout(2000), 
        });
        
        if (res.ok) {
          return NextResponse.json({ success: true, msg: `Relé ${idRele} alternado no hardware!` }, { status: 200 });
        }
      } catch (err) {
        console.warn(`[API Control] ESP32 não respondeu em /relay?id=${idRele}. Executando fallback seguro.`);
      }

      // Retorno de Sucesso Garantido para a interface React não travar os botões
      return NextResponse.json({ success: true, msg: `Comando enviado para o relé ${idRele}` }, { status: 200 });
    }

    // ============================================================================
    // CASO 2: CONTROLE MODO APRENDER INFRAVERMELHO (Clonar controle físico)
    // ============================================================================
    if (action === "learn") {
      console.log(`[API Control] Modo Aprender iniciado para: ${command}`);

      const res = await fetch(`${IP_ESP32}/learn`, { 
        method: "GET",
        signal: AbortSignal.timeout(12000) 
      });

      if (!res.ok) {
        return NextResponse.json({ error: "Tempo limite esgotado no hardware" }, { status: 408 });
      }

      const codigoHexCapturado = await res.text();

      // Grava ou Atualiza diretamente no seu PostgreSQL via Prisma
      const comandoAtualizado = await prisma.comandoIR.upsert({
        where: { nome: command },
        update: { codigoHex: codigoHexCapturado },
        create: {
          device: device,
          nome: command,
          codigoHex: codigoHexCapturado
        }
      });

      return NextResponse.json({ success: true, msg: `Comando ${command} salvo no banco!`, data: comandoAtualizado }, { status: 200 });
    }

    // ============================================================================
    // CASO 3: CONTROLE MODO DISPARAR INFRAVERMELHO (Ligar/Desligar Ar e TV)
    // ============================================================================
    if (command) {
      const comandoSalvo = await prisma.comandoIR.findUnique({
        where: { nome: command }
      });

      if (!comandoSalvo) {
        return NextResponse.json({ error: "Comando não cadastrado no banco PostgreSQL" }, { status: 404 });
      }

      try {
        await fetch(`${IP_ESP32}/send?code=${encodeURIComponent(comandoSalvo.codigoHex)}`, { 
          method: "GET",
          signal: AbortSignal.timeout(3000) 
        });
      } catch (e) {
        console.warn("[API Control] Falha ao enviar sinal IR para o ESP32.");
      }
      
      return NextResponse.json({ success: true, msg: `Sinal IR ${command} processado!` }, { status: 200 });
    }

    return NextResponse.json({ error: "Dispositivo ou comando inválido" }, { status: 400 });
  } catch (error) {
    console.error("[API Control] Erro crítico:", error);
    return NextResponse.json({ error: "Erro interno no servidor de controle" }, { status: 500 });
  }
}
