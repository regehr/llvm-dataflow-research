#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const bool Verbose = true;
static const int MaxWidth = 2;

class MyConstantRange : public ConstantRange {
public:
  explicit MyConstantRange(uint32_t BitWidth, bool isFullSet) :
      ConstantRange(BitWidth, isFullSet) {}
  MyConstantRange(APInt Value) : ConstantRange(Value) {}
  MyConstantRange(APInt Lower, APInt Upper) : ConstantRange(Lower, Upper) {}
  using ConstantRange::operator=;

  // your transfer functions go here

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
                  double &Bits, double &PreciseBits, const int Width, int &Count, int &PreciseCount) {
  MyConstantRange Res1(Width, true);
  switch (Opcode) {
  case Instruction::And:
    Res1 = L.binaryAnd(R);
    break;
  case Instruction::Or:
    Res1 = L.binaryOr(R);
    break;
  case Instruction::Add:
    Res1 = L.add(R);
    break;
  case Instruction::Sub:
    Res1 = L.sub(R);
    break;
  case Instruction::Shl:
    Res1 = L.shl(R);
    break;
  case Instruction::LShr:
    Res1 = L.lshr(R);
    break;
  case Instruction::AShr:
    Res1 = L.ashr(R);
    break;
  default:
    report_fatal_error("unsupported opcode");
  }
  MyConstantRange Res2 = exhaustive(L, R, Opcode, Res1, Width);
  if (Verbose) {
    outs() << "signed: " << L << " " << tostr(Opcode) << " " << R << " =   LLVM: " << Res1
           << "   precise: " << Res2 << "\n";
    outs() << "unsigned: ";
    printUnsigned(L, outs());
    outs() << " " << tostr(Opcode) << " ";
    printUnsigned(R, outs());
    outs() << " =   LLVM: ";
    printUnsigned(Res1, outs());
    outs() << "\n";
    outs() << "set size = " << Res1.getSetSize() << "  " << Res1.getSetSize().getLimitedValue() << "  " << Res1 << "\n";
    outs() << "   precise: ";
    printUnsigned(Res2, outs());
    outs() << "\n";
    outs() << "set size = " << Res2.getSetSize() << "  " << Res2.getSetSize().getLimitedValue() << "  " << Res2 << "\n";
    if (Res1.getSetSize().ugt(Res2.getSetSize())) {
	outs() << "imprecise! "
	       << "LLVM size: " << Res1.getSetSize().getLimitedValue()
	       << "; Precise size: " << Res2.getSetSize().getLimitedValue() << "\n";
    }
    outs() << "\n";
  }

  long W = Res1.getSetSize().getLimitedValue();
  if (W > 0) {
    Bits += log2((double)W);
    Count++;
  }
  W = Res2.getSetSize().getLimitedValue();
  if (W > 0) {
    PreciseBits += log2((double)W);
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
  double Bits = 0.0, PreciseBits = 0.0;
  long count = 0;
  int Count = 0, PreciseCount = 0;
  do {
    do {
      check(L, R, Opcode, Bits, PreciseBits, Width, Count, PreciseCount);
      R = next(R);
    } while (!R.isEmptySet());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges for Op = " << tostr(Opcode) << "\n";
  double Precise = PreciseBits / PreciseCount;
  double Other = Bits / Count;
  // assert(Precise <= Other);
  outs() << "best possible bits = " << Precise << "\n";
  outs() << "transfer function bits = " << Other << "\n";
}

static void test(const int Width) {
  outs() << "\nWidth = " << Width << "\n";
  testAllConstantRanges(Instruction::Or, Width);
  testAllConstantRanges(Instruction::And, Width);
#if 0
  testAllConstantRanges(Instruction::Add, Width);
  testAllConstantRanges(Instruction::Sub, Width);
  testAllConstantRanges(Instruction::Shl, Width);
  testAllConstantRanges(Instruction::LShr, Width);
  testAllConstantRanges(Instruction::AShr, Width);
#endif
}

int main(void) {

#if 1
  for (int Width = 1; Width <= MaxWidth; ++Width) {
    test(Width);
  }
#else
  test(2);
#endif

  return 0;
}
