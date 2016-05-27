#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const int Width = 3;

ConstantRange exhaustive(const ConstantRange L, const ConstantRange R,
                         const ConstantRange Untrusted) {
  if (L.isEmptySet() || R.isEmptySet())
    return ConstantRange(Width, /* isFullSet= */ false);
  bool Seen[1 << Width] = { false };
  ConstantRange Joined(Width, /* isFullSet= */ false);
  auto LI = L.getLower();
  do {
    auto RI = R.getLower();
    do {
      auto Val = LI << RI;
      if (!Untrusted.contains(Val)) {
        outs() << "oops!\n";
        exit(-1);
      }
      Joined.unionWith(ConstantRange(Val));
      Seen[Val.getLimitedValue()] = true;
      RI++;
    } while (RI != R.getUpper());
    LI++;
  } while (LI != L.getUpper());
  ConstantRange Res(Width, /* isFullSet= */ false);
  return Res;
}

void check(ConstantRange L, ConstantRange R) {
  ConstantRange Res1 = L.shl(R);
  ConstantRange Res2 = exhaustive(L, R, Res1);
  if (Res2.getSetSize().ult(Res1.getSetSize()))
    outs() << L << " op " << R << " = " << Res1 << " " << Res2 << "\n";
}

ConstantRange next(const ConstantRange CR) {
  auto L = CR.getLower();
  auto U = CR.getUpper();
  do {
    if (U.isMaxValue())
      L++;
    U++;
  } while (L == U && !L.isMinValue() && !L.isMaxValue());
  return ConstantRange(L, U);
}

void test() {
  ConstantRange L(Width, /* isFullSet= */ false);
  ConstantRange R(Width, /* isFullSet= */ false);
  long count = 0;
  do {
    do {
      check(L, R);
      count++;
      R = next(R);
    } while (!R.isEmptySet());
    L = next(L);
  } while (!L.isEmptySet());
  outs() << "checked " << count << " ConstantRanges\n";
}

int main(void) {
  test();
  return 0;
}
