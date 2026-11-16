import "dotenv/config";
import { defineConfig, env } from "prisma/config";

export default defineConfig({
  schema: "prisma/schema.prisma",
  
  // ================= ADICIONE ESTE BLOCO AQUI =================
  migrations: {
    path: "prisma/migrations",
    seed: "npx tsx prisma/seed.ts", // Instrui o Prisma 7 a rodar o seu arquivo de sementes
  },
  // ============================================================

  datasource: {
    url: env("DATABASE_URL"),
  },
});
