using System.Runtime.InteropServices;

namespace BeepEngineTest
{
    internal class Program
    {
        static void Main(string[] args)
        {
            bool b = StartBeepEngine();

            Console.WriteLine($"Beep engine started. Result: {b}");

            if (!IsBeepEngineRunning())
            {
                Console.WriteLine("Beep engine is not running.");
                return;
            }

            while(true)
            {
                Console.Write("Enter frequency: ");
                string? s = Console.ReadLine();
                if (s != null && float.TryParse(s, out float freq))
                {
                    BeepEngineBeep(freq, 0.5f);
                }
                else break;
            }

            StopBeepEngine();
            Console.WriteLine("Beep engine stopped.");
        }

        [DllImport("Sunlighter.BeepEngine.dll")]
        public static extern bool StartBeepEngine();

        [DllImport("Sunlighter.BeepEngine.dll")]
        public static extern void StopBeepEngine();

        [DllImport("Sunlighter.BeepEngine.dll")]
        public static extern void BeepEngineBeep(float frequency, float duration);

        [DllImport("Sunlighter.BeepEngine.dll")]
        public static extern bool IsBeepEngineRunning();
    }
}
