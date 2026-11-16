export default function Header({ title }: { title: string }) {
  return (
    <header className="bg-[#1e293b] py-6 text-center font-black text-xl tracking-wider text-transparent bg-clip-text bg-gradient-to-r from-blue-400 to-emerald-400 shadow-md border-b border-slate-800">
      {title}
    </header>
  );
}
