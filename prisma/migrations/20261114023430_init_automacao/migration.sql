-- CreateTable
CREATE TABLE "Telemetria" (
    "id" SERIAL NOT NULL,
    "temp" DOUBLE PRECISION NOT NULL,
    "pres" DOUBLE PRECISION NOT NULL,
    "energy" DOUBLE PRECISION NOT NULL,
    "criadoEm" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT "Telemetria_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "ComandoIR" (
    "id" SERIAL NOT NULL,
    "nome" TEXT NOT NULL,
    "codigoHex" TEXT NOT NULL,
    "atualizado" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "ComandoIR_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "ComandoIR_nome_key" ON "ComandoIR"("nome");
