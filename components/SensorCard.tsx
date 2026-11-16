interface SensorCardProps {
  title: string;
  value: string;
  icon: React.ReactNode;
  color: string;
  subtitle?: string;
}

export default function SensorCard({ title, value, icon, color, subtitle }: SensorCardProps) {
  return (
    <div className="bg-[#1e293b] p-5 rounded-2xl shadow-lg border border-slate-800 hover:-translate-y-1 transition-all duration-300 group">
      <div className="flex flex-col items-center text-center">
        <svg className={`w-10 h-10 ${color} mb-2 transition-transform duration-300 group-hover:scale-110`} viewBox="0 0 24 24" fill="currentColor">
          {icon}
        </svg>
        <h3 className="text-xs font-bold text-slate-400 uppercase tracking-wider">{title}</h3>
        <h1 className="text-2xl sm:text-3xl font-black mt-2 tracking-tight">{value}</h1>
        {subtitle && <p className="text-xs text-slate-500 mt-1 font-medium">{subtitle}</p>}
      </div>
    </div>
  );
}
