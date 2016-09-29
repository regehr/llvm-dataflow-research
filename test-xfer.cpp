#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const bool Verbose = false;
static const int MaxWidth = 5;

int Width, Range;
double Bits, PreciseBits;

bool ok(const ConstantRange &R, const bool Table[Range]) {
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
ConstantRange bestCR(const bool Table[Range]) {

  unsigned Pop = 0;
  unsigned Any;
  for (unsigned i = 0; i < Range; ++i)
    if (Table[i]) {
      ++Pop;
      Any = i;
    }
  if (Pop == 0)
    return ConstantRange(Width, false);
  if (Pop == Range)
    return ConstantRange(Width, true);

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

  ConstantRange R(Width, false);
  if ((Bottom + (Range - 1 - Top)) > MaxSize) {
    APInt Lo(Width, Bottom);
    APInt Hi(Width, (Top + 1) % Range);
    R = ConstantRange(Lo, Hi);
  } else {
    APInt Lo(Width, (MaxHole + MaxSize) % Range);
    APInt Hi(Width, MaxHole);
    R = ConstantRange (Lo, Hi);
  }

  assert(ok(R, Table));
  if (Pop == 1) {
    assert(R.getLower().getLimitedValue() == Any);
    assert(R.getUpper().getLimitedValue() == (Any + 1) % Range);
  } else {
    APInt L1 = R.getLower() + 1;
    ConstantRange R2(L1, R.getUpper());
    assert(!ok(R2, Table));
    ConstantRange R3(R.getLower(), R.getUpper() - 1);
    assert(!ok(R3, Table));
  }

  return R;
}

ConstantRange exhaustive(const ConstantRange L, const ConstantRange R,
                         unsigned Opcode, const ConstantRange Untrusted) {
  if (L.isEmptySet() || R.isEmptySet())
    return ConstantRange(Width, /* isFullSet= */ false);
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
      case Instruction::Xor:
        Val = LI ^ RI;
        break;
      case Instruction::Add:
        Val = LI + RI;
        break;
      case Instruction::Sub:
        Val = LI - RI;
        break;
      default:
        report_fatal_error("unknown opcode");
      }
      if (false)
        outs() << LI << " op " << RI << " = " << Val << "\n";
      if (!Untrusted.contains(Val))
        report_fatal_error("oops1");
      Table[Val.getLimitedValue()] = true;
      ++RI;
    } while (RI != R.getUpper());
    ++LI;
  } while (LI != L.getUpper());
  return bestCR(Table);
}

void check(ConstantRange L, ConstantRange R, unsigned Opcode) {
  ConstantRange Res1(Width, true);
  switch (Opcode) {
  case Instruction::And:
    Res1 = L.binaryAnd(R);
    break;
  case Instruction::Or:
    Res1 = L.binaryOr(R);
    break;
  case Instruction::Xor:
    report_fatal_error("xor unsupported");
    break;
  case Instruction::Add:
    Res1 = L.add(R);
    break;
  case Instruction::Sub:
    Res1 = L.sub(R);
    break;
  default:
    report_fatal_error("unsupported opcode");
  }
  ConstantRange Res2 = exhaustive(L, R, Opcode, Res1);
  if (Res2.getSetSize().ult(Res1.getSetSize())) {
    int diff = (Res1.getSetSize() - Res2.getSetSize()).getLimitedValue();
    if (Verbose)
      outs() << L << " op " << R << " =   LLVM: " << Res1 << "   precise: " << Res2 <<
        " diff = " << diff << "\n";
  }
  if (Res1.getSetSize().ult(Res2.getSetSize())) {
    if (Verbose)
      outs() << L << " op " << R << " =   LLVM: " << Res1 << "   precise: " << Res2 << "\n";
    report_fatal_error("oops2");
  }

  //outs() << "width = " << Res1.getSetSize().getLimitedValue() << " ";
  //outs() << "log width = " << log2((double)Res1.getSetSize().getLimitedValue()) << "\n";

  long W = Res1.getSetSize().getLimitedValue();
  if (W > 0)
    PreciseBits += log2((double)W);
  W = Res2.getSetSize().getLimitedValue();
  if (W > 0)
    Bits += log2((double)W);
}

ConstantRange next(const ConstantRange CR) {
  auto L = CR.getLower();
  auto U = CR.getUpper();
  do {
    if (U.isMaxValue())
      ++L;
    ++U;
  } while (L == U && !L.isMinValue() && !L.isMaxValue());
  return ConstantRange(L, U);
}

void testAllConstantRanges(unsigned Opcode) {
  ConstantRange L(Width, /*isFullSet=*/false);
  ConstantRange R(Width, /*isFullSet=*/false);
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
  outs() << "checked " << count << " ConstantRanges\n";
  outs() << "best possible bits = " << (PreciseBits / count) << "\n";
  outs() << "transfer function bits = " << (Bits / count) << "\n";
}

int main(void) {
  for (Width = 1; Width <= MaxWidth; ++Width) {
    outs() << "Width = " << Width << "\n";
    Range = 1 << Width;
    testAllConstantRanges(Instruction::And);
    testAllConstantRanges(Instruction::Or);
  }
  return 0;
}
