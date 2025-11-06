using Sunlighter.OptionLib;
using System.Collections.Immutable;
using System.Drawing;
using System.Numerics;

namespace FFTLogicTest
{
    internal class Program
    {
        static void Main(string[] args)
        {
            int size = 8;

            SymbolicValue[] inputs = FFT.MakeInputs(size);
            SymbolicValue[] outputs = FFT.MakeZeroArray(size);
            SymbolicValue[] outputs2 = FFT.MakeZeroArray(size);
            SymbolicValue[] outputs3 = FFT.MakeZeroArray(size);

            void WriteOutputs(SymbolicValue[] outputs)
            {
                foreach (int i in Enumerable.Range(0, size))
                {
                    Console.WriteLine($"\e[0;1;31moutputs[{i}]\e[0m = {outputs[i]}");
                }
            }

            Console.WriteLine("\e[0;1;37m***** Iterative DFT *****\e[0m");
            FFT.DFT_P2(inputs.CreateValueSequence(), outputs.CreateValueSequence(), false);
            WriteOutputs(outputs);

            Console.WriteLine("\e[0;1;37m***** Recursive DFT *****\e[0m");
            FFT.DFT_Split(inputs.CreateValueSequence(), outputs2.CreateValueSequence());
            WriteOutputs(outputs2);

            Console.WriteLine("\e[0;1;37m***** Slow DFT *****\e[0m");
            FFT.DFT(inputs.CreateValueSequence(), outputs3.CreateValueSequence());
            WriteOutputs(outputs3);
        }
    }

    public sealed class BigRational
    {
        private readonly BigInteger numerator;
        private readonly BigInteger denominator;

        private BigRational(BigInteger numerator, BigInteger denominator)
        {
            this.numerator = numerator;
            this.denominator = denominator;
        }

        public BigInteger Numerator => numerator;
        public BigInteger Denominator => denominator;

        public static BigRational Create(BigInteger numerator, BigInteger denominator)
        {
            BigInteger n1 = numerator;
            BigInteger d1 = denominator;

            if (d1 == 0) throw new DivideByZeroException();
            if (d1 < 0)
            {
                n1 = -n1;
                d1 = -d1;
            }

            BigInteger gcd = BigInteger.GreatestCommonDivisor(BigInteger.Abs(n1), d1);

            return new BigRational(n1 / gcd, d1 / gcd);
        }

        public static BigRational operator +(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return Create(n1 * d2 + n2 * d1, d1 * d2);
        }

        public static BigRational operator -(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return Create(n1 * d2 - n2 * d1, d1 * d2);
        }

        public static BigRational operator *(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return Create(n1 * n2, d1 * d2);
        }

        public static BigRational operator /(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return Create(n1 * d2, d1 * n2);
        }

        public static BigRational operator -(BigRational a)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            return Create(-n1, d1);
        }

        public static BigRational Floor(BigRational b)
        {
            BigInteger n1 = b.Numerator;
            BigInteger d1 = b.Denominator;
            BigInteger q = n1 / d1;
            if (n1 >= 0 || n1 % d1 == 0) return Create(q, 1);
            return Create(q - 1, 1);
        }

        public static bool operator ==(BigRational a, BigRational b)
        {
            return a.Numerator == b.Numerator && a.Denominator == b.Denominator;
        }

        public static bool operator !=(BigRational a, BigRational b)
        {
            return a.Numerator != b.Numerator || a.Denominator != b.Denominator;
        }

        public override bool Equals(object? obj)
        {
            if (obj is BigRational)
            {
                BigRational br = (BigRational)obj;
                return this == br;
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return $"{numerator}/{denominator}".GetHashCode();
        }

        public override string ToString()
        {
            if (denominator == BigInteger.One)
            {
                return numerator.ToString();
            }
            else
            {
                return $"{numerator}/{denominator}";
            }
        }

        public static BigRational Zero => new BigRational(0, 1);

        public static BigRational One => new BigRational(1, 1);

        public static BigRational MinusOne => new BigRational(-1, 1);

        public static bool operator <(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return n1 * d2 < n2 * d1;
        }

        public static bool operator >(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return n1 * d2 > n2 * d1;
        }

        public static bool operator <=(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return n1 * d2 <= n2 * d1;
        }

        public static bool operator >=(BigRational a, BigRational b)
        {
            BigInteger n1 = a.Numerator;
            BigInteger d1 = a.Denominator;
            BigInteger n2 = b.Numerator;
            BigInteger d2 = b.Denominator;
            return n1 * d2 >= n2 * d1;
        }
    }

    public abstract class SymbolicValue
    {
        private static Option<SymbolicTerm> TryAddCoefficients(SymbolicTerm ta, SymbolicTerm tb)
        {
            if (Utility.EqualOptions<int>(ta.InputIndex, tb.InputIndex, (a, b) => (a == b)) && ta.RootOfUnity == tb.RootOfUnity)
            {
                return Option<SymbolicTerm>.Some(new SymbolicTerm(ta.Coefficient + tb.Coefficient, ta.InputIndex, ta.RootOfUnity));
            }
            else
            {
                return Option<SymbolicTerm>.None;
            }
        }

        private static ImmutableList<SymbolicTerm> AddCoefficient(ImmutableList<SymbolicTerm> termList, SymbolicTerm newTerm)
        {
            ImmutableList<SymbolicTerm> resultTerms = [];
            bool used = false;
            foreach (SymbolicTerm termListItem in termList)
            {
                if (used)
                {
                    resultTerms = resultTerms.Add(termListItem);
                }
                else
                {
                    Option<SymbolicTerm> tr = TryAddCoefficients(termListItem, newTerm);
                    if (tr.HasValue)
                    {
                        if (tr.Value.Coefficient != BigRational.Zero)
                        {
                            resultTerms = resultTerms.Add(tr.Value);
                        }
                        used = true;
                    }
                    else
                    {
                        resultTerms = resultTerms.Add(termListItem);
                    }
                }
            }
            if (!used)
            {
                resultTerms = resultTerms.Add(newTerm);
            }
            return resultTerms;
        }

        private static ImmutableList<SymbolicTerm> AddCoefficients(ImmutableList<SymbolicTerm> termListA, ImmutableList<SymbolicTerm> termListB)
        {
            ImmutableList<SymbolicTerm> resultTerms = termListA;
            foreach (SymbolicTerm termListItem in termListB)
            {
                resultTerms = AddCoefficient(resultTerms, termListItem);
            }
            return resultTerms;
        }

        public static SymbolicValue operator + (SymbolicValue a, SymbolicValue b)
        {
            if (a is SymbolicTerm ta && b is SymbolicTerm tb)
            {
                Option<SymbolicTerm> tr = TryAddCoefficients(ta, tb);
                if (tr.HasValue)
                {
                    return tr.Value;
                }
                else
                {
                    return new SymbolicSum([ta, tb]);
                }
            }
            else if (a is SymbolicTerm ta2 && b is SymbolicSum sb)
            {
                return new SymbolicSum(AddCoefficient(sb.Terms, ta2));   
            }
            else if (a is SymbolicSum sa && b is SymbolicTerm tb2)
            {
                return new SymbolicSum(AddCoefficient(sa.Terms, tb2));
            }
            else if (a is SymbolicSum sa2 && b is SymbolicSum sb2)
            {
                return new SymbolicSum(AddCoefficients(sa2.Terms, sb2.Terms));
            }
            else
            {
                throw new ArgumentException("Invalid arguments to operator +");
            }
        }

        public static SymbolicValue operator -(SymbolicValue a)
        {
            if (a is SymbolicTerm ta)
            {
                return -ta;
            }
            else if (a is SymbolicSum sa)
            {
                ImmutableList<SymbolicTerm> newTerms = [];
                foreach (SymbolicTerm term in sa.Terms)
                {
                    newTerms = newTerms.Add(-term);
                }
                return new SymbolicSum(newTerms);
            }
            else
            {
                throw new ArgumentException("Invalid argument to operator -");
            }
        }

        public static SymbolicValue operator -(SymbolicValue a, SymbolicValue b)
        {
            return a + (-b);
        }

        private static SymbolicTerm MultiplyTerms(SymbolicTerm a, SymbolicTerm b)
        {
            BigRational newCoefficient = a.Coefficient * b.Coefficient;

            if (a.InputIndex.HasValue && b.InputIndex.HasValue)
            {
                throw new ArgumentException("Cannot multiply two terms with input indices");
            }

            Option<int> newInputIndex = a.InputIndex.HasValue ? a.InputIndex : b.InputIndex;

            BigRational newRootOfUnity = a.RootOfUnity + b.RootOfUnity;
            newRootOfUnity = newRootOfUnity - BigRational.Floor(newRootOfUnity);

            return new SymbolicTerm(newCoefficient, newInputIndex, newRootOfUnity);
        }

        private static ImmutableList<SymbolicTerm> MultiplyTerms(ImmutableList<SymbolicTerm> a, SymbolicTerm b)
        {
            ImmutableList<SymbolicTerm> newTerms = [];
            foreach (SymbolicTerm term in a)
            {
                newTerms = newTerms.Add(MultiplyTerms(term, b));
            }
            return newTerms;
        }

        private static ImmutableList<SymbolicTerm> MultiplyTerms(ImmutableList<SymbolicTerm> a, ImmutableList<SymbolicTerm> b)
        {
            ImmutableList<SymbolicTerm> results = [];
            foreach(SymbolicTerm bItem in b)
            {
                results = AddCoefficients(results, MultiplyTerms(a, bItem));
            }
            return results;
        }

        public static SymbolicValue operator *(SymbolicValue a, SymbolicValue b)
        {
            if (a is SymbolicTerm ta && b is SymbolicTerm tb)
            {
                return MultiplyTerms(ta, tb);
            }
            else if (a is SymbolicTerm ta2 && b is SymbolicSum sb)
            {
                return new SymbolicSum(MultiplyTerms(sb.Terms, ta2));
            }
            else if (a is SymbolicSum sa && b is SymbolicTerm tb2)
            {
                return new SymbolicSum(MultiplyTerms(sa.Terms, tb2));
            }
            else if (a is SymbolicSum sa2 && b is SymbolicSum sb2)
            {
                return new SymbolicSum(MultiplyTerms(sa2.Terms, sb2.Terms));
            }
            else
            {
                throw new ArgumentException("Invalid arguments to operator *");
            }
        }

        private static readonly SymbolicValue zero = new SymbolicSum([]);

        public static SymbolicValue Zero => zero;

        private static readonly SymbolicValue one = new SymbolicTerm(BigRational.One, Option<int>.None, BigRational.Zero);

        public static SymbolicValue One => one;

        public static SymbolicValue RootOfUnity(BigRational b) => new SymbolicTerm(BigRational.One, Option<int>.None, b);

        public static SymbolicValue Constant(BigRational b) => new SymbolicTerm(b, Option<int>.None, BigRational.Zero);
    }

    public sealed class SymbolicTerm : SymbolicValue
    {
        private readonly BigRational coefficient;
        private readonly Option<int> inputIndex;
        private readonly BigRational rootOfUnity;

        public SymbolicTerm
        (
            BigRational coefficient,
            Option<int> inputIndex,
            BigRational rootOfUnity
        )
        {
            this.coefficient = coefficient;
            this.inputIndex = inputIndex;
            this.rootOfUnity = rootOfUnity;
        }

        public BigRational Coefficient => coefficient;

        public Option<int> InputIndex => inputIndex;

        public new BigRational RootOfUnity => rootOfUnity;

        public static SymbolicTerm operator -(SymbolicTerm s)
        {
            BigRational rootOfUnity2 = s.RootOfUnity + BigRational.Create(1, 2);
            rootOfUnity2 = rootOfUnity2 - BigRational.Floor(rootOfUnity2);
            return new SymbolicTerm(s.Coefficient, s.InputIndex, rootOfUnity2);
        }

        public override string ToString()
        {
            if (coefficient == BigRational.Zero) return "0";

            if (coefficient == BigRational.One && !inputIndex.HasValue && rootOfUnity == BigRational.Zero) return "1";

            List<string> items = new List<string>();
            if (coefficient != BigRational.One)
            {
                items.Add(coefficient.ToString());
            }
            if (inputIndex.HasValue)
            {
                items.Add($"x[{inputIndex.Value}]");
            }
            if (rootOfUnity != BigRational.Zero)
            {
                items.Add($"exp(2*pi*j*{rootOfUnity})");
            }
            return string.Join("*", items);
        }
    }

    public sealed class SymbolicSum : SymbolicValue
    {
        private readonly ImmutableList<SymbolicTerm> terms;

        public SymbolicSum(ImmutableList<SymbolicTerm> terms)
        {
            int compareInts(int a, int b)
            {
                if (a < b) return -1;
                if (a > b) return 1;
                return 0;
            }

            this.terms = terms.OrderByComparison(Utility.MakeSelectComparison<SymbolicTerm, Option<int>>(st => st.InputIndex, Utility.MakeOptionComparison<int>(compareInts))).ToImmutableList();
        }

        public ImmutableList<SymbolicTerm> Terms => terms;

        public override string ToString()
        {
            return "(" + string.Join(" + ", terms) + ")";
        }
    }

    public static class Utility
    {
        public static bool EqualOptions<T>(Option<T> a, Option<T> b, Func<T, T, bool> equal)
        {
            if (a.HasValue)
            {
                if (b.HasValue)
                {
                    return equal(a.Value, b.Value);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return !b.HasValue;
            }
        }

        public static IEnumerable<T> OrderByComparison<T>(this IEnumerable<T> items, Comparison<T> compareFunc)
        {
            List<T> list = items.ToList();
            list.Sort((a, b) => compareFunc(a, b));
            return list;
        }

        public static Comparison<Option<T>> MakeOptionComparison<T>(Comparison<T> itemComparison)
        {
            return (a, b) =>
            {
                if (a.HasValue)
                {
                    if (b.HasValue)
                    {
                        return itemComparison(a.Value, b.Value);
                    }
                    else
                    {
                        return 1;
                    }
                }
                else
                {
                    if (b.HasValue)
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }
            };
        }

        public static Comparison<T> MakeSelectComparison<T, U>(Func<T, U> select, Comparison<U> comparison)
        {
            return (a, b) => comparison(select(a), select(b));
        }
    }

    public interface IValueSequence<T>
    {
        int Size { get; }
        T this[int index] { get;  set; }
    }

    public sealed class ValueSequence<T> : IValueSequence<T>
    {
        private readonly int size;
        private readonly Func<int, T> get;
        private readonly Action<int, T> set;

        public ValueSequence(int size, Func<int, T> get, Action<int, T> set)
        {
            this.size = size;
            this.get = get;
            this.set = set;
        }

        public int Size => size;

        public T this[int index]
        {
            get => get(index);
            set => set(index, value);
        }
    }

    public interface IValueRect<T>
    {
        int Width { get; }
        int Height { get; }
        T this[int x, int y] { get;  set; }
    }

    public sealed class ValueRect<T> : IValueRect<T>
    {
        private readonly int width;
        private readonly int height;
        private readonly Func<int, int, T> get;
        private readonly Action<int, int, T> set;

        public ValueRect(int width, int height, Func<int, int, T> get, Action<int, int, T> set)
        {
            this.width = width;
            this.height = height;
            this.get = get;
            this.set = set;
        }

        public int Width => width;

        public int Height => height;

        public T this[int x, int y]
        {
            get => get(x, y);
            set => set(x, y, value);
        }
    }

    public sealed class RectangleData
    {
        private readonly int offset;
        private readonly int xStride;
        private readonly int width;
        private readonly int yStride;
        private readonly int height;
        
        public RectangleData(int offset, int xStride, int width, int yStride, int height)
        {
            this.offset = offset;
            this.xStride = xStride;
            this.width = width;
            this.yStride = yStride;
            this.height = height;
        }

        public int Offset => offset;
        public int XStride => xStride;
        public int Width => width;
        public int YStride => yStride;
        public int Height => height;

        public int Area => height * width;

        public static RectangleData CreateSimpleRowMajor(int width, int height)
        {
            return new RectangleData(0, 1, width, width, height);
        }

        public static RectangleData CreateSimpleColumnMajor(int width, int height)
        {
            return new RectangleData(0, height, width, 1, height);
        }

        public IValueSequence<T> CreateHorizontalSequence<T>(IValueSequence<T> underlying, int row)
        {
            int offset2 = offset + row * yStride;
            return new ValueSequence<T>(width, i => underlying[offset2 + i * xStride], (i, v) => underlying[offset2 + i * xStride] = v);
        }

        public IValueSequence<T> CreateVerticalSequence<T>(IValueSequence<T> underlying, int column)
        {
            int offset2 = offset + column * xStride;
            return new ValueSequence<T>(height, i => underlying[offset2 + i * yStride], (i, v) => underlying[offset2 + i * yStride] = v);
        }

        public IValueSequence<T> CreateRowMajorSequence<T>(IValueSequence<T> underlying)
        {
            return new ValueSequence<T>
            (
                Area,
                i =>
                {
                    int x = i % width;
                    int y = i / width;
                    return underlying[offset + x * xStride + y * yStride];
                },
                (i, v) =>
                {
                    int x = i % width;
                    int y = i / width;
                    underlying[offset + x * xStride + y * yStride] = v;
                }
            );
        }

        public IValueSequence<T> CreateColumnMajorSequence<T>(IValueSequence<T> underlying)
        {
            return new ValueSequence<T>
            (
                Area,
                i =>
                {
                    int x = i / height;
                    int y = i % height;
                    return underlying[offset + x * xStride + y * yStride];
                },
                (i, v) =>
                {
                    int x = i / height;
                    int y = i % height;
                    underlying[offset + x * xStride + y * yStride] = v;
                }
            );
        }

        public IValueRect<T> CreateRect<T>(IValueSequence<T> underlying)
        {
            return new ValueRect<T>
            (
                width,
                height,
                (x, y) => underlying[offset + x * xStride + y * yStride],
                (x, y, v) => underlying[offset + x * xStride + y * yStride] = v
            );
        }

        public override string ToString()
        {
            return $"[rect, offset = {offset}, width = {width}, height = {height}, xStride = {xStride}, yStride = {yStride}]";
        }
    }

    public static class FFT
    {
        public static string DebugPrint(this IValueSequence<SymbolicValue> seq)
        {
            string hilite(int i, string s)
            {
                if ((i & 1) == 0)
                {
                    return "\e[0;1;32m" + s + "\e[0m";
                }
                else
                {
                    return "\e[0;1;33m" + s + "\e[0m";
                }
            }

            return "[" + string.Join(", ", Enumerable.Range(0, seq.Size).Select(i => hilite(i, seq[i]?.ToString() ?? ""))) + "]";
        }

        public static SymbolicValue[] MakeInputs(int size)
        {
            SymbolicValue[] inputs = new SymbolicValue[size];
            for (int i = 0; i < size; i++)
            {
                inputs[i] = new SymbolicTerm(BigRational.One, Option<int>.Some(i), BigRational.Zero);
            }
            return inputs;
        }

        public static SymbolicValue[] MakeZeroArray(int size)
        {
            SymbolicValue[] outputs = new SymbolicValue[size];
            for (int i = 0; i < size; i++)
            {
                outputs[i] = new SymbolicSum([]);
            }
            return outputs;
        }

        public static IValueSequence<T> CreateValueSequence<T>(this T[] arr)
        {
            return new ValueSequence<T>(arr.Length, i => arr[i], (i, v) => arr[i] = v);
        }

        public static IValueSequence<T> CreateTempArray<T>(int size, T initialItem)
        {
            T[] items = new T[size];
            for (int i = 0; i < size; i++)
            {
                items[i] = initialItem;
            }
            ValueSequence<T> dest = new ValueSequence<T>(size, i => items[i], (i, v) => items[i] = v);
            return dest;
        }

        public static void CopyTo<T>(this IValueSequence<T> src, IValueSequence<T> dest)
        {
            System.Diagnostics.Debug.Assert(src.Size == dest.Size);
            for (int i = 0; i < src.Size; i++)
            {
                dest[i] = src[i];
            }
        }

        public static void DFT(IValueSequence<SymbolicValue> inputs, IValueSequence<SymbolicValue> outputs)
        {
            System.Diagnostics.Debug.Assert(inputs.Size == outputs.Size);
            int iEnd = inputs.Size;
            for (int i = 0; i < iEnd; ++i)
            {
                SymbolicValue result = new SymbolicSum([]);
                for (int j = 0; j < iEnd; ++j)
                {
                    BigRational rootOfUnity = BigRational.Create(iEnd - j * i, iEnd);
                    rootOfUnity -= BigRational.Floor(rootOfUnity);

                    result += inputs[j] * SymbolicValue.RootOfUnity(rootOfUnity);
                }
                outputs[i] = result;
            }
        }

        public static void DFT_Split(IValueSequence<SymbolicValue> inputs, IValueSequence<SymbolicValue> outputs)
        {
            System.Diagnostics.Debug.Assert(inputs.Size == outputs.Size);

            Console.WriteLine($"DFT_Split {inputs.Size}");
            //Console.WriteLine(inputs.DebugPrint());
            
            void HandleRect(RectangleData rect)
            {
                Console.WriteLine(rect.ToString());

                IValueSequence<SymbolicValue> temp0 = CreateTempArray(rect.Area, SymbolicValue.Zero);
                rect.CreateRowMajorSequence(inputs).CopyTo(rect.CreateColumnMajorSequence(temp0));

                IValueSequence<SymbolicValue> temp1 = CreateTempArray(rect.Area, SymbolicValue.Zero);

                foreach (int y in Enumerable.Range(0, rect.Height))
                {
                    IValueSequence<SymbolicValue> inputRow = rect.CreateHorizontalSequence(temp0, y);
                    IValueSequence<SymbolicValue> outputRow = rect.CreateHorizontalSequence(temp1, y);

                    Console.Write($"  input = "); Console.Write(inputRow.DebugPrint()); Console.WriteLine();
                    Console.Write($"  {y} ->");  DFT_Split(inputRow, outputRow);
                    Console.Write($"  output = "); Console.Write(outputRow.DebugPrint()); Console.WriteLine();
                }

                IValueSequence<SymbolicValue> temp2 = CreateTempArray(rect.Area, SymbolicValue.Zero);

                IValueRect<SymbolicValue> temp1SrcRect = rect.CreateRect(temp1);
                IValueRect<SymbolicValue> temp2DestRect = rect.CreateRect(temp2);

                foreach (int y in Enumerable.Range(0, rect.Height))
                {
                    foreach (int x in Enumerable.Range(0, rect.Width))
                    {
                        BigRational rootOfUnity = BigRational.Create((rect.Area - x * y) % rect.Area, rect.Area);
                        temp2DestRect[x, y] = temp1SrcRect[x, y] * SymbolicValue.RootOfUnity(rootOfUnity);
                        Console.WriteLine($"[x = {x}, y = {y}] = {temp1SrcRect[x, y]} * \e[0;1;31m{rootOfUnity}\e[0m = {temp2DestRect[x, y]}");
                    }
                }

                IValueSequence<SymbolicValue> temp3 = CreateTempArray(rect.Area, SymbolicValue.Zero);
                
                foreach (int x in Enumerable.Range(0, rect.Width))
                {
                    IValueSequence<SymbolicValue> inputRow = rect.CreateVerticalSequence(temp2, x);
                    IValueSequence<SymbolicValue> outputRow = rect.CreateVerticalSequence(temp3, x);

                    Console.Write($"  input = "); Console.Write(inputRow.DebugPrint()); Console.WriteLine();
                    Console.Write($"  {x} ->"); DFT_Split(inputRow, outputRow);
                    Console.Write($"  output = "); Console.Write(outputRow.DebugPrint()); Console.WriteLine();
                }

                temp3.CopyTo(outputs);
                //rect.CreateColumnMajorSequence(temp3).CopyTo(rect.CreateRowMajorSequence(outputs));
            };

            if (inputs.Size % 2 == 0 && inputs.Size > 2)
            {
                HandleRect(RectangleData.CreateSimpleRowMajor(inputs.Size / 2, 2));
            }
            else if (inputs.Size % 3 == 0 && inputs.Size > 3)
            {
                HandleRect(RectangleData.CreateSimpleRowMajor(inputs.Size / 3, 3));
            }
            else if (inputs.Size % 5 == 0 && inputs.Size > 5)
            {
                HandleRect(RectangleData.CreateSimpleRowMajor(inputs.Size / 5, 5));
            }
            else
            {
                DFT(inputs, outputs);
            }
        }

        public static bool IsPowerOfTwo(int x)
        {
            return (x & (x - 1)) == 0;
        }

        public static int BitReverse(int i, int size)
        {
            System.Diagnostics.Debug.Assert(IsPowerOfTwo(size));

            int result = 0;
            int inBit = 1;
            int outBit = size >> 1;

            while (outBit > 0)
            {
                if ((i & inBit) != 0)
                {
                    result |= outBit;
                }
                inBit <<= 1;
                outBit >>= 1;
            }

            return result;
        }

        public static void BitReverseBuffer(IValueSequence<SymbolicValue> src, IValueSequence<SymbolicValue> dest)
        {
            System.Diagnostics.Debug.Assert(src.Size == dest.Size);
            System.Diagnostics.Debug.Assert(IsPowerOfTwo(src.Size));

            for (int i = 0; i < src.Size; i++)
            {
                dest[BitReverse(i, src.Size)] = src[i];
            }
        }

        public static void DFT_P2_Butterfly(IValueSequence<SymbolicValue> inputs, IValueSequence<SymbolicValue> outputs, int i0, int i1)
        {
            SymbolicValue a = inputs[i0];
            SymbolicValue b = inputs[i1];
            
            outputs[i0] = a + b;
            outputs[i1] = a - b;

            Console.WriteLine($"Butterfly: {i0} {i1} inputs: {a} {b} outputs: {outputs[i0]} {outputs[i1]}");
        }

        public static void DFT_P2_Stage(IValueSequence<SymbolicValue> inputs, IValueSequence<SymbolicValue> outputs, int groupSize, int groupCount, bool isInverse)
        {
            System.Diagnostics.Debug.Assert(inputs.Size == outputs.Size);
            System.Diagnostics.Debug.Assert(IsPowerOfTwo(inputs.Size));
            System.Diagnostics.Debug.Assert(groupSize * groupCount == inputs.Size);

            int maxOffset = groupSize / 2;

            Console.WriteLine($"Stage: groupSize = {groupSize}, groupCount = {groupCount}, maxOffset in group = {maxOffset}");

            for (int group = 0; group < groupCount; ++group)
            {
                int groupOffset = group * groupSize;

                Console.WriteLine($"Group: {group}, groupOffset = {groupOffset}");

                for (int offset = 0; offset < maxOffset; ++offset)
                {
                    int y0 = groupOffset + offset;
                    int y1 = groupOffset + offset + maxOffset;

                    DFT_P2_Butterfly(inputs, outputs, y0, y1);

                    if (groupCount > 1)
                    {
                        int x = (((group & 1) != 0) ? maxOffset : 0) + offset;
                        int xbr = BitReverse(x, groupSize);
                        // the twiddle factors have to be at (3, 5, 7), (3, 6, 7), or (5, 6, 7).
                        // (3, 6, 7) is the one that corresponds to the butterfly outputs.
                        // the next concern is getting the roots of unity right.
                        // Right now (3, 6, 7) is mapping to (1, 2, 3) but I want it to map to (2, 1, 3).
                        // Later it will be (3, 6, 7, 10, 11, 14, 15) and I will want it to map to (4, 2, 6, 1, 5, 3, 7).

                        if (x > 0)
                        {
                            SymbolicValue rootOfUnity = SymbolicValue.RootOfUnity(BigRational.Create(xbr, groupSize * 2));
                            if (!isInverse)
                            {
                                rootOfUnity = -rootOfUnity;
                            }
                            Console.WriteLine($"Twiddle: x = \e[0;1;32m{x}\e[0m, xbr = \e[0;1;32m{xbr}\e[0m, y1 = \e[0;1;32m{y1}\e[0m, rootOfUnity = {rootOfUnity}, input = {outputs[y1]}, output = {outputs[y1] * rootOfUnity}");
                            outputs[y1] *= rootOfUnity;
                        }
                    }
                }
            }

#if false
            if (groupCount > 1)
            {
                for (int groupPair = 0; groupPair < groupCount; groupPair += 2)
                {
                    int groupOffset = groupPair * groupSize;
                    for (int x = 0; x < groupSize; ++x)
                    {
                        if (x > 0)
                        {
                            int y1 = groupOffset + (x * 2) + 1;
                            SymbolicValue rootOfUnity = SymbolicValue.RootOfUnity(BigRational.Create(x, groupSize * 2));
                            if (!isInverse)
                            {
                                rootOfUnity = -rootOfUnity;
                            }
                            Console.WriteLine($"Twiddle: {y1} {rootOfUnity}");
                            outputs[y1] *= rootOfUnity;
                        }
                    }
                }
            }
#endif
        }

        public static void DFT_P2(IValueSequence<SymbolicValue> inputs, IValueSequence<SymbolicValue> outputs, bool isInverse)
        {
            System.Diagnostics.Debug.Assert(inputs.Size == outputs.Size);
            int size = inputs.Size;
            System.Diagnostics.Debug.Assert(IsPowerOfTwo(size));

            IValueSequence<SymbolicValue> temp0 = CreateTempArray(size, SymbolicValue.Zero);
            BitReverseBuffer(inputs, temp0);

            int groupSize = 2;
            int groupCount = size / 2;

            while(true)
            {
                DFT_P2_Stage(temp0, outputs, groupSize, groupCount, isInverse);
                groupSize <<= 1;
                groupCount >>= 1;
                (temp0, outputs) = (outputs, temp0);
                if (groupCount == 0) break;
            }

            if (isInverse)
            {
                for (int i = 0; i < size; i++)
                {
                    temp0[i] = temp0[i] * SymbolicValue.Constant(BigRational.Create(1, size));
                }
            }

            temp0.CopyTo(outputs);
        }
    }
}
