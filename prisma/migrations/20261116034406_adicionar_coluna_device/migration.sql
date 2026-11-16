/*
  Warnings:

  - You are about to drop the column `atualizado` on the `ComandoIR` table. All the data in the column will be lost.
  - Added the required column `device` to the `ComandoIR` table without a default value. This is not possible if the table is not empty.

*/
-- AlterTable
ALTER TABLE "ComandoIR" DROP COLUMN "atualizado",
ADD COLUMN     "device" TEXT NOT NULL;

-- CreateTable
CREATE TABLE "Automacao" (
    "id" SERIAL NOT NULL,
    "nome" TEXT NOT NULL,
    "sensor" TEXT NOT NULL,
    "gatilho" TEXT NOT NULL,
    "valor" DOUBLE PRECISION NOT NULL,
    "acaoDevice" TEXT NOT NULL,
    "acaoComando" TEXT NOT NULL,
    "ativa" BOOLEAN NOT NULL DEFAULT true,

    CONSTRAINT "Automacao_pkey" PRIMARY KEY ("id")
);
