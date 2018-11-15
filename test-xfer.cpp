#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/KnownBits.h"
#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>

using namespace llvm;
using namespace std::chrono;

static const bool Speed = false;
static const bool Verbose = false;
static const int MaxWidth = 4;

class MyConstantRange : public ConstantRange {
public:
  explicit MyConstantRange(uint32_t BitWidth, bool isFullSet) :
      ConstantRange(BitWidth, isFullSet) {}
  MyConstantRange(APInt Value) : ConstantRange(Value) {}
  MyConstantRange(APInt Lower, APInt Upper) : ConstantRange(Lower, Upper) {}
  using ConstantRange::operator=;

  // your transfer functions go here

#if 0
  // this is the code from ConstantRange.cpp
  MyConstantRange
  binaryOr(const MyConstantRange &Other) const {
    if (isEmptySet() || Other.isEmptySet())
      return MyConstantRange(getBitWidth(), /*isFullSet=*/false);
    APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
    if (umax.isNullValue())
      return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
    return MyConstantRange(std::move(umax), APInt::getNullValue(getBitWidth()));
  }
#endif

#ifdef TEST
#include "funcs.cpp"
#endif
  
};

static void printUnsigned(const MyConstantRange &CR, raw_ostream &OS) {
  if (CR.isFullSet())
    OS << "full-set";
  else if (CR.isEmptySet())
    OS << "empty-set";
  else {
    OS << "[";
    CR.getLower().print(OS, /*isSigned=*/false);
    OS << ",";
    CR.getUpper().print(OS, /*isSigned=*/false);
    OS << ")";
  }
}

static bool ok(const MyConstantRange &R, const bool Table[], const int Width) {
  const int Range = 1 << Width;
  for (int i = 0; i < Range; ++i) {
    if (Table[i]) {
      APInt a(Width, i);
      if (!R.contains(a))
        return false;
    }
  }
  return true;
}

// Find the largest hole and build a ConstantRange around it
static MyConstantRange bestCR(const bool Table[], const int Width) {
  const int Range = 1 << Width;
  unsigned Pop = 0;
  unsigned Any;
  for (unsigned i = 0; i < Range; ++i)
    if (Table[i]) {
      ++Pop;
      Any = i;
    }
  if (Pop == 0)
    return MyConstantRange(Width, /*isFullSet=*/false);
  if (Pop == Range)
    return MyConstantRange(Width, /*isFullSet=*/true);

  unsigned Hole = 0, MaxHole = 0, MaxSize = 0;
  bool inHole = false;
  for (unsigned i = 0; i < Range; ++i) {
    if (Table[i]) {
      if (inHole) {
        inHole = false;
        if ((i - Hole) > MaxSize) {
          MaxHole = Hole;
          MaxSize = i - Hole;
        }
      }
    } else {
      if (!inHole) {
        inHole = true;
        Hole = i;
      }
    }
  }
  if (inHole && ((Range - Hole) > MaxSize)) {
    MaxHole = Hole;
    MaxSize = Range - Hole;
  }

  unsigned Bottom = 0;
  while (!Table[Bottom])
    ++Bottom;
  unsigned Top = Range - 1;
  while (!Table[Top])
    --Top;

  MyConstantRange R(Width, false);
  if ((Bottom + (Range - 1 - Top)) > MaxSize) {
    APInt Lo(Width, Bottom);
    APInt Hi(Width, (Top + 1) % Range);
    R = MyConstantRange(Lo, Hi);
  } else {
    APInt Lo(Width, (MaxHole + MaxSize) % Range);
    APInt Hi(Width, MaxHole);
    R = MyConstantRange(Lo, Hi);
  }

  assert(ok(R, Table, Width));
  if (Pop == 1) {
    assert(R.getLower().getLimitedValue() == Any);
    assert(R.getUpper().getLimitedValue() == (Any + 1) % Range);
  } else {
    APInt L1 = R.getLower() + 1;
    MyConstantRange R2(L1, R.getUpper());
    assert(!ok(R2, Table, Width));
    MyConstantRange R3(R.getLower(), R.getUpper() - 1);
    assert(!ok(R3, Table, Width));
  }

  return R;
}

static std::string tostr(const unsigned Opcode) {
  switch (Opcode) {
  case Instruction::And:
    return "&";
  case Instruction::Or:
    return "|";
  case Instruction::Add:
    return "+";
  case Instruction::Sub:
    return "-";
  case Instruction::Shl:
    return "<<";
  case Instruction::LShr:
    return ">>l";
  case Instruction::AShr:
    return ">>a";
  default:
    report_fatal_error("unsupported opcode");
  }
}

static MyConstantRange exhaustive(const MyConstantRange L, const MyConstantRange R,
                                  const unsigned Opcode, const MyConstantRange Untrusted,
                                  const int Width) {
  if (L.isEmptySet() || R.isEmptySet())
    return MyConstantRange(Width, /*isFullSet=*/false);
  bool Table[1 << Width];
  for (int i = 0; i < (1 << Width); ++i)
    Table[i] = false;
  auto LI = L.getLower();
  do {
    auto RI = R.getLower();
    do {
      APInt Val;
      switch (Opcode) {
      case Instruction::And:
        Val = LI & RI;
        break;
      case Instruction::Or:
        Val = LI | RI;
        break;
      case Instruction::Add:
        Val = LI + RI;
        break;
      case Instruction::Sub:
        Val = LI - RI;
        break;
      case Instruction::Shl:
        Val = LI.shl(RI);
        break;
      case Instruction::LShr:
        Val = LI.lshr(RI);
        break;
      case Instruction::AShr:
        Val = LI.ashr(RI);
        break;
      default:
        report_fatal_error("unknown opcode");
      }
      if (!Untrusted.contains(Val)) {
        outs() << "\n";
        outs() << "width = " << Width << "\n";
        outs() << "left operand: " << L << " (unsigned :";
        printUnsigned(L, outs());
        outs() << ")\n";
        outs() << "right operand: " << R << " (unsigned : ";
        printUnsigned(R, outs());
        outs() << ")\n";
        outs() << "operation: " << tostr(Opcode) << "\n";
        outs() << "untrusted: " << Untrusted << "\n";
        outs() << "must contain: " << Val << "\n";
        report_fatal_error("BUG! MyConstantRange transfer function is not sound");
      }
      Table[Val.getLimitedValue()] = true;
      ++RI;
    } while (RI != R.getUpper());
    ++LI;
  } while (LI != L.getUpper());
  return bestCR(Table, Width);
}

static void check(const MyConstantRange L, const MyConstantRange R, const unsigned Opcode,
                  double &FastBits, double &PreciseBits, const int Width, int &Count, int &PreciseCount) {
  MyConstantRange FastRes(Width, true);
  switch (Opcode) {
  case Instruction::And:
    FastRes = L.binaryAnd(R);
    break;
  case Instruction::Or:
    FastRes = L.binaryOr(R);
    break;
  case Instruction::Add:
    FastRes = L.add(R);
    break;
  case Instruction::Sub:
    FastRes = L.sub(R);
    break;
  case Instruction::Shl:
    FastRes = L.shl(R);
    break;
  case Instruction::LShr:
    FastRes = L.lshr(R);
    break;
  case Instruction::AShr:
    FastRes = L.ashr(R);
    break;
  default:
    report_fatal_error("unsupported opcode");
  }

  MyConstantRange PreciseRes = exhaustive(L, R, Opcode, FastRes, Width);

  long FastSize = FastRes.getSetSize().getLimitedValue();
  long PreciseSize = PreciseRes.getSetSize().getLimitedValue();

  if (Verbose) {
    outs() << "signed: " << L << " " << tostr(Opcode) << " " << R << " =   fast : " << FastRes
           << "   precise: " << PreciseRes << "\n";
    outs() << "unsigned: ";
    printUnsigned(L, outs());
    outs() << " " << tostr(Opcode) << " ";
    printUnsigned(R, outs());
    outs() << " =   fast: ";
    printUnsigned(FastRes, outs());
    outs() << "   precise: ";
    printUnsigned(PreciseRes, outs());
    outs() << "\n";
    if (FastRes.getSetSize().ugt(PreciseRes.getSetSize())) {
	outs() << "fast transfer function is imprecise! "
	       << "fast size: " << FastRes.getSetSize().getLimitedValue()
	       << "; Precise size: " << PreciseRes.getSetSize().getLimitedValue() << "\n";
    }
    outs() << "\n";
  }

  assert(FastSize >= 0 && FastSize <= (1 << Width));
  assert(PreciseSize >= 0 && PreciseSize <= (1 << Width));
  assert(PreciseSize <= FastSize);

  if (FastSize > 0) {
    FastBits += log2((double)FastSize);
    Count++;
  }
  if (PreciseSize > 0) {
    PreciseBits += log2((double)PreciseSize);
    PreciseCount++;
  }
}

static MyConstantRange next(const MyConstantRange CR) {
  auto L = CR.getLower();
  auto U = CR.getUpper();
  do {
    if (U.isMaxValue())
      ++L;
    ++U;
  } while (L == U && !L.isMinValue() && !L.isMaxValue());
  return MyConstantRange(L, U);
}

static void testAllConstantRanges(const unsigned Opcode, const int Width) {
  MyConstantRange L(Width, /*isFullSet=*/false);
  MyConstantRange R(Width, /*isFullSet=*/false);
  double FastBits = 0.0, PreciseBits = 0.0;
  long count = 0;
  int Count = 0, PreciseCount = 0;
  do {
    do {
      check(L, R, Opcode, FastBits, PreciseBits, Width, Count, PreciseCount);
      R = next(R);
    } while (!R.isEmptySet());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges for Op = " << tostr(Opcode) << "\n";
  double Precise = PreciseBits / PreciseCount;
  double Fast = FastBits / Count;
  assert(Precise <= Fast);
  outs() << "best possible bits = " << Precise << "\n";
  outs() << "fast transfer function bits = " << Fast << "\n";
}

static void timeAllConstantRanges(const unsigned Opcode, const int Width) {
  const int REPS = 5;
  std::vector<duration<double>> Times(REPS);
  std::vector<double> Bits(REPS);
  for (int Rep = 0; Rep < REPS; Rep++) {
    high_resolution_clock::time_point start = high_resolution_clock::now();
    MyConstantRange L(Width, /*isFullSet=*/false);
    MyConstantRange R(Width, /*isFullSet=*/false);
    double B = 0;
    long Count = 0;
    do {
      do {
        MyConstantRange FastRes(Width, true);
        switch (Opcode) {
        case Instruction::And:
          FastRes = L.binaryAnd(R);
          break;
        case Instruction::Or:
          FastRes = L.binaryOr(R);
          break;
        case Instruction::Add:
          FastRes = L.add(R);
          break;
        case Instruction::Sub:
          FastRes = L.sub(R);
          break;
        case Instruction::Shl:
          FastRes = L.shl(R);
          break;
        case Instruction::LShr:
          FastRes = L.lshr(R);
          break;
        case Instruction::AShr:
          FastRes = L.ashr(R);
          break;
        default:
          report_fatal_error("unsupported opcode");
        }
        long N = FastRes.getSetSize().getLimitedValue();
        if (N > 0) {
          B += log2((double)N);
        }
        Count++;
        R = next(R);
      } while (!R.isEmptySet());
      L = next(L);
    } while (!L.isEmptySet());
    high_resolution_clock::time_point stop = high_resolution_clock::now();
    duration<double> T = duration_cast<duration<double>>(stop - start);
    Times.at(Rep) = T;
    Bits.at(Rep) = B / Count;
  }
  double Best = 1e33;
  for (int Rep = 0; Rep < REPS; Rep++) {
    double t = Times.at(Rep).count();
    if (t < Best)
      Best = t;
  }
  const std::string s = "";
  outs() << Best << "  " << Bits.at(0) << "  " << s << "\n";
}

static void test(const int Width) {
  if (Speed) {
    timeAllConstantRanges(Instruction::And, Width);
    //timeAllConstantRanges(Instruction::Or, Width);
  } else {
    outs() << "\nWidth = " << Width << "\n";
    testAllConstantRanges(Instruction::And, Width);
    //testAllConstantRanges(Instruction::Or, Width);
  }
#if 0
  testAllConstantRanges(Instruction::Add, Width);
  testAllConstantRanges(Instruction::Sub, Width);
  testAllConstantRanges(Instruction::Shl, Width);
  testAllConstantRanges(Instruction::LShr, Width);
  testAllConstantRanges(Instruction::AShr, Width);
#endif
}

static void printSizes() {
  MyConstantRange R(2, /*isFullSet=*/false);
  do {
    outs() << R << "  " << R.getSetSize().getLimitedValue() << "\n";
    R = next(R);
  } while (!R.isEmptySet());
}

int main(void) {
  if (Speed) {
    test(6);
  } else {
    if (1) {
      for (int Width = 1; Width <= MaxWidth; ++Width) {
	test(Width);
      }
    } else {
      test(4);
    }
  }

  return 0;
}
