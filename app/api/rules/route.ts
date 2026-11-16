import { NextResponse } from "next/server";
import { PrismaClient } from "@prisma/client";

const prisma = new PrismaClient();

// GET: Lista todas as automações para a tela React
export async function GET() {
  try {
    const regras = await prisma.automacao.findMany({
      orderBy: { id: "desc" },
    });
    return NextResponse.json(regras);
  } catch (error) {
    return NextResponse.json({ error: "Erro ao buscar regras" }, { status: 500 });
  }
}

// POST: Cria um novo gatilho enviado pelo formulário
export async function POST(request: Request) {
  try {
    const body = await request.json();
    const { nome, sensor, gatilho, valor, acaoDevice, acaoComando } = body;

    const novaRegra = await prisma.automacao.create({
      data: {
        nome,
        sensor,
        gatilho,
        valor: parseFloat(valor),
        acaoDevice,
        acaoComando,
      },
    });

    return NextResponse.json(novaRegra, { status: 201 });
  } catch (error) {
    return NextResponse.json({ error: "Erro ao criar automação" }, { status: 400 });
  }
}

// PATCH: Ativa ou pausa uma automação existente
export async function PATCH(request: Request) {
  try {
    const body = await request.json();
    const { id, Wood, ativa } = body;

    const regraAtualizada = await prisma.automacao.update({
      where: { id: parseInt(id) },
      data: { ativa },
    });

    return NextResponse.json(regraAtualizada);
  } catch (error) {
    return NextResponse.json({ error: "Erro ao atualizar status da regra" }, { status: 400 });
  }
}
