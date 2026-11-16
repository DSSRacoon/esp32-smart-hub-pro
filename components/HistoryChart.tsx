"use client";

import { Area, AreaChart, CartesianGrid, XAxis, YAxis } from "recharts";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import {
  ChartConfig,
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
} from "@/components/ui/chart";

// Dados fictícios que depois virão do seu banco PostgreSQL
const chartData = [
  { hora: "10:00", temp: 22, energia: 120 },
  { hora: "11:00", temp: 23.5, energia: 340 },
  { hora: "12:00", temp: 25, energia: 410 },
  { hora: "13:00", temp: 24.8, energia: 150 },
  { hora: "14:00", temp: 26, energia: 180 },
  { hora: "15:00", temp: 23, energia: 500 },
];

// Configuração de cores e labels seguindo o padrão Shadcn
const chartConfig = {
  temp: {
    label: "Temperatura (°C)",
    color: "hsl(var(--chart-1))", // Puxa a cor vermelha/laranja padrão do seu tema
  },
  energia: {
    label: "Consumo (W)",
    color: "hsl(var(--chart-2))", // Puxa a cor azul/verde
  },
} satisfies ChartConfig;

export default function HistoryChart() {
  return (
    <Card className="bg-[#1e293b] border-slate-800 text-white shadow-xl">
      <CardHeader>
        <CardTitle className="text-lg font-bold tracking-tight text-slate-200">Histórico de Telemetria</CardTitle>
        <CardDescription className="text-slate-400">Leituras consolidadas do banco de dados nas últimas horas</CardDescription>
      </CardHeader>
      <CardContent>
        <ChartContainer config={chartConfig} className="h-[300px] w-full">
          <AreaChart
            accessibilityLayer
            data={chartData}
            margin={{ left: -20, right: 12, top: 10, bottom: 0 }}
          >
            <CartesianGrid vertical={false} stroke="#334155" />
            <XAxis
              dataKey="hora"
              tickLine={false}
              axisLine={false}
              tickMargin={8}
              stroke="#94a3b8"
            />
            <YAxis 
              tickLine={false}
              axisLine={false}
              stroke="#94a3b8"
            />
            <ChartTooltip
              cursor={false}
              content={<ChartTooltipContent indicator="dot" />}
            />
            {/* Linha de Temperatura com Gradiente Preenchido */}
            <Area
              dataKey="temp"
              type="monotone"
              fill="rgba(239, 68, 68, 0.1)"
              stroke="#ef4444"
              stackId="a"
            />
            {/* Linha de Energia com Gradiente Preenchido */}
            <Area
              dataKey="energia"
              type="monotone"
              fill="rgba(59, 130, 246, 0.1)"
              stroke="#3b82f6"
              stackId="b"
            />
          </AreaChart>
        </ChartContainer>
      </CardContent>
    </Card>
  );
}
