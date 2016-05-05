#include "llvm/ADT/APInt.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

static const int Width = 8;

ConstantRange next(const ConstantRange CR) {
  auto L = CR.getLower();
  auto U = CR.getUpper();
  do {
    if (U.isMaxValue()) {
      L++;
      U = APInt(Width, 0);
    } else {
      U++;
    }
  } while (L == U && !L.isMinValue() && !L.isMaxValue());
  return ConstantRange(L, U);
}

int main(void) {
  ConstantRange C(Width, /* isFullSet= */ false);
  do {
    outs() << C << "\n";
    C = next(C);
  } while (!C.isEmptySet());
  return 0;
}
