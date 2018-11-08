#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const bool Verbose = false;
static const int MaxWidth = 5;

int Width, Range;
double Bits, PreciseBits;

class MyConstantRange : public ConstantRange {
public:
  explicit MyConstantRange(uint32_t BitWidth, bool isFullSet) :
      ConstantRange(BitWidth, isFullSet) {}
  MyConstantRange(APInt Value) : ConstantRange(Value) {}
  MyConstantRange(APInt Lower, APInt Upper) : ConstantRange(Lower, Upper) {}
  using ConstantRange::operator=;
};

void printUnsigned(const MyConstantRange &CR, raw_ostream &OS) {
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

bool ok(const MyConstantRange &R, const bool Table[Range]) {
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
MyConstantRange bestCR(const bool Table[Range]) {

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

  assert(ok(R, Table));
  if (Pop == 1) {
    assert(R.getLower().getLimitedValue() == Any);
    assert(R.getUpper().getLimitedValue() == (Any + 1) % Range);
  } else {
    APInt L1 = R.getLower() + 1;
    MyConstantRange R2(L1, R.getUpper());
    assert(!ok(R2, Table));
    MyConstantRange R3(R.getLower(), R.getUpper() - 1);
    assert(!ok(R3, Table));
  }

  return R;
}

MyConstantRange exhaustive(const MyConstantRange L, const MyConstantRange R,
                         unsigned Opcode, const MyConstantRange Untrusted) {
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
      if (!Untrusted.contains(Val))
        report_fatal_error("BUG! MyConstantRange transfer function is not sound");
      Table[Val.getLimitedValue()] = true;
      ++RI;
    } while (RI != R.getUpper());
    ++LI;
  } while (LI != L.getUpper());
  return bestCR(Table);
}

std::string tostr(unsigned Opcode) {
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

void check(MyConstantRange L, MyConstantRange R, unsigned Opcode) {
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
  MyConstantRange Res2 = exhaustive(L, R, Opcode, Res1);
  if (Verbose) {
    outs() << "unsigned: " << L << " " << tostr(Opcode) << " " << R << " =   LLVM: " << Res1
           << "   precise: " << Res2 << "\n";
    outs() << "signed: ";
    printUnsigned(L, outs());
    outs() << " op ";
    printUnsigned(R, outs());
    outs() << " =   LLVM: ";
    printUnsigned(Res1, outs());
    outs() << "   precise: ";
    printUnsigned(Res2, outs());
    outs() << "\n";
    if (Res1.getSetSize().ugt(Res2.getSetSize())) {
      outs() << "imprecise!\n";
    }
    outs() << "\n";
  }

  long W = Res1.getSetSize().getLimitedValue();
  if (W > 0)
    Bits += log2((double)W);
  W = Res2.getSetSize().getLimitedValue();
  if (W > 0)
    PreciseBits += log2((double)W);
}

MyConstantRange next(const MyConstantRange CR) {
  auto L = CR.getLower();
  auto U = CR.getUpper();
  do {
    if (U.isMaxValue())
      ++L;
    ++U;
  } while (L == U && !L.isMinValue() && !L.isMaxValue());
  return MyConstantRange(L, U);
}

void testAllConstantRanges(unsigned Opcode) {
  MyConstantRange L(Width, /*isFullSet=*/false);
  MyConstantRange R(Width, /*isFullSet=*/false);
  Bits = PreciseBits = 0.0;
  long count = 0;
  do {
    do {
      check(L, R, Opcode);
      ++count;
      R = next(R);
    } while (!R.isEmptySet());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges for Op = " << tostr(Opcode) << "\n";
  outs() << "best possible bits = " << (PreciseBits / count) << "\n";
  outs() << "transfer function bits = " << (Bits / count) << "\n";
}

int main(void) {
  for (Width = 1; Width <= MaxWidth; ++Width) {
    outs() << "\nWidth = " << Width << "\n";
    Range = 1 << Width;
    testAllConstantRanges(Instruction::Add);
    testAllConstantRanges(Instruction::Sub);
    testAllConstantRanges(Instruction::And);
    testAllConstantRanges(Instruction::Or);
    testAllConstantRanges(Instruction::Shl);
    testAllConstantRanges(Instruction::LShr);
    testAllConstantRanges(Instruction::AShr);
  }
  return 0;
}
