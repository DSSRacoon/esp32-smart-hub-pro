import 'dotenv/config';
import { PrismaClient } from '@prisma/client';
import { PrismaPg } from '@prisma/adapter-pg';

// Configura o adaptador nativo exigido pelo Prisma 7 utilizando a sua URL do .env
const adapter = new PrismaPg({ connectionString: process.env.DATABASE_URL });
const prisma = new PrismaClient({ adapter });

async function main() {
  console.log('🌱 Iniciando a carga de comandos padrões de Infravermelho...');

  const comandosPadroes = [
    { device: 'ac', nome: 'ac_on', codigoHex: '18,32,A1B2C3D4' },
    { device: 'ac', nome: 'ac_off', codigoHex: '18,32,E5F6G7H8' },
    { device: 'ac', nome: 'ac_23', codigoHex: '18,32,C5D6E7F8' },
    { device: 'tv', nome: 'tv_on', codigoHex: '7,32,20DF10EF' },
    { device: 'tv', nome: 'tv_off', codigoHex: '7,32,20DF10EF' },
  ];

  for (const cmd of comandosPadroes) {
    await prisma.comandoIR.upsert({
      where: { nome: cmd.nome },
      update: {}, 
      create: cmd, 
    });
  }

  console.log('✅ Banco de dados populado com sucesso!');
}

main()
  .catch((e) => {
    console.error(e);
    process.exit(1);
  })
  .finally(async () => {
    await prisma.$disconnect();
  });
