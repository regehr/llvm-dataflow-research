#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const int Width = 5;
static const int Range = 1 << Width;

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
                         const ConstantRange Untrusted) {
  if (L.isEmptySet() || R.isEmptySet())
    return ConstantRange(Width, /* isFullSet= */ false);
  bool Table[1 << Width] = { 0 };
  auto LI = L.getLower();
  do {
    auto RI = R.getLower();
    do {
      auto Val = LI & RI;
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

double check(ConstantRange L, ConstantRange R) {
  ConstantRange Res1 = L.binaryAnd(R);
  ConstantRange Res2 = exhaustive(L, R, Res1);
  if (Res2.getSetSize().ult(Res1.getSetSize())) {
    int diff = (Res1.getSetSize() - Res2.getSetSize()).getLimitedValue();
    outs() << L << " op " << R << " =   LLVM: " << Res1 << "   precise: " << Res2 <<
      " diff = " << diff << "\n";
  }
  if (Res1.getSetSize().ult(Res2.getSetSize())) {
    outs() << L << " op " << R << " =   LLVM: " << Res1 << "   precise: " << Res2 << "\n";
    report_fatal_error("oops2");
  }

  APInt difference = Res1.getSetSize() - Res2.getSetSize();
  if (difference.isMinValue())
    return 0;
  return log2((double)difference.getLimitedValue());
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

void testAllConstantRanges() {
  ConstantRange L(Width, /* isFullSet= */ false);
  ConstantRange R(Width, /* isFullSet= */ false);
  long count = 0;
  double bits = 0;
  do {
    do {
      bits += check(L, R);
      ++count;
      R = next(R);
    } while (!R.isEmptySet());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges\n";
  outs() << "average bits saved = " << (bits / count) << "\n";
  outs() << "total bits saved = " << bits << "\n";
}

void testAllRHSConstant() {
  ConstantRange L(Width, /* isFullSet= */ false);
  APInt R(Width, 0);
  long count = 0;
  double bits = 0;
  do {
    do {
      bits += check(L, R);
      ++count;
      ++R;
    } while (!R.isMaxValue());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges\n";
  outs() << "average bits saved = " << (bits / count) << "\n";
  outs() << "total bits saved = " << bits << "\n";
}

int main(void) {
  testAllConstantRanges();
  //testAllRHSConstant();
  return 0;
}
