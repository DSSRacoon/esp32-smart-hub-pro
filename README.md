# ESP32 Smart Hub Pro - Full-Stack Automation Panel

Este projeto é um ecossistema IoT híbrido de alta resiliência que integra um microcontrolador **ESP32** a uma aplicação web moderna de alto desempenho desenvolvida com **Next.js (App Router)** e banco de dados **PostgreSQL** [10.23 PM, 11.50 AM].

A arquitetura foi projetada para mover a carga pesada de processamento de interface, APIs e persistência histórica para o servidor, permitindo que o hardware opere de forma leve e extremamente estável com o framework **ESP RainMaker** [11.50 AM].

## 🚀 Tecnologias Utilizadas

### Web Server (Next.js Full-Stack)
* **Framework:** Next.js 15+ (React, Client/Server Components) [11.50 AM]
* **Estilização:** Tailwind CSS & Shadcn/ui (Design moderno e responsivo) [11.50 AM]
* **Gráficos:** Recharts integrados ao Shadcn [11.50 AM]
* **ORM:** Prisma v7 (Type-Safe Queries com Adaptador de Driver Nativo) [11.16 AM]
* **Banco de Dados:** PostgreSQL (Histórico de sensores e controle de comandos IR) [10.23 PM, 11.16 AM]

### Hardware (Embedded IoT Node)
* **Microcontrolador:** ESP32 Dev Module [11.50 AM]
* **Framework IoT:** ESP RainMaker (Criptografia BLE, provisionamento ágil e suporte nativo à Alexa/Google Home) [11.50 AM]
* **Drivers de Sensores:** Adafruit BMP280 (SPI por Software) & LiquidCrystal_I2C (Pinos 21/22) [12.06 PM, 12.16 PM]
* **Módulo Infravermelho:** IRremoteESP8266 (Controle de Ar Condicionado e Televisão) [10.23 PM]

---

## 📐 Arquitetura do Sistema e Fluxo de Dados

```text
 [ Front-End: React ]  <--->  [ Back-End: Next.js API ]  <--->  [ PostgreSQL Database ]
                                         |
                                         v (HTTP Polling / Active Requests)
                                [ Hardware: ESP32 Hub ]
```

1. **Inversão de Fluxo para Telemetria:** O Next.js toma a iniciativa de consultar o IP do ESP32 de forma cíclica [10.23 PM, 12.07 AM]. Ele captura as leituras brutas de temperatura, pressão e consumo, salvando-as permanentemente no banco de dados e enviando-as de forma reativa para a interface gráfica e o gráfico de histórico [11.49 AM, 11.54 AM].
2. **Resiliência Baseada em Software (Mecanismo de Fallback):** Caso o hardware sofra interrupções elétricas ou de rede, a API de controle do Next.js intercepta o erro através de blocos `try/catch` de proteção, retornando **Status 200 OK** para a interface [11.54 AM]. Isso impede o congelamento do JavaScript no navegador e mantém o painel responsivo [11.54 AM].
3. **Gerenciamento de Comandos IR por Banco de Dados:** Removendo *strings* e buffers dinâmicos pesados da memória estática (IRAM) do chip, os códigos hexadecimais dos controles são armazenados na tabela `ComandoIR` no banco [10.23 PM, 11.50 AM]. Ao acionar o painel, o Next.js busca o código e despacha uma requisição simplificada para o LED transmissor do ESP32 [10.23 PM, 11.52 PM].

---

## 🛠️ Como Iniciar o Projeto Web

### 1. Clonar o repositório e instalar dependências
```bash
git clone https://github.com
cd seu-repositorio
npm install
```

### 2. Configurar Variáveis de Ambiente
Crie um arquivo `.env` na raiz do projeto e configure a URL de conexão do seu PostgreSQL:
```env
DATABASE_URL="postgresql://usuario:senha@localhost:5432/nomedobanco?schema=public"
```

### 3. Sincronizar Estrutura do Banco de Dados
```bash
npx prisma migrate dev --name init_automacao
```

### 4. Executar a Carga Inicial de Dados (Prisma Seed v7)
Popula automaticamente o banco com os registros e comandos infravermelhos iniciais do Ar e da TV [11.16 AM]:
```bash
npx prisma db seed
```

### 5. Rodar o servidor de desenvolvimento
```bash
npm run dev
```
Acesse `http://localhost:3000` no seu navegador para controlar a casa! [11.54 AM]
