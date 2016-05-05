#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const int Width = 4;

ConstantRange exhaustive(const ConstantRange L, const ConstantRange R) {
  ConstantRange Res(Width, /* isFullSet= */ false);
  if (L.isEmptySet() || R.isEmptySet())
    return Res;
  auto LI = L.getLower();
  do {
    auto RI = R.getLower();
    do {
      Res = Res.unionWith(ConstantRange(LI - RI));
      RI++;
    } while (RI != R.getUpper());
    LI++;
  } while (LI != L.getUpper());
  return Res;
}

void check(ConstantRange L, ConstantRange R) {
  ConstantRange Res1 = L.sub(R);
  ConstantRange Res2 = exhaustive(L, R);
  if (Res1 != Res2)
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
