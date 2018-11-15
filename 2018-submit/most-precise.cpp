bool ok(const MyConstantRange &R, const bool Table[], const int Width) const {
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

MyConstantRange xbestCR(const bool Table[], const int Width) const {
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

MyConstantRange
binaryAnd(const MyConstantRange &R) const {
  int Width = getBitWidth();
  if (isEmptySet() || R.isEmptySet())
    return MyConstantRange(Width, /*isFullSet=*/false);
  bool Table[1 << Width];
  for (int i = 0; i < (1 << Width); ++i)
    Table[i] = false;
  auto LI = getLower();
  do {
    auto RI = R.getLower();
    do {
      APInt Val = LI & RI;
      Table[Val.getLimitedValue()] = true;
      ++RI;
    } while (RI != R.getUpper());
    ++LI;
  } while (LI != getUpper());
  return xbestCR(Table, Width);
}

MyConstantRange
binaryOr(const MyConstantRange &R) const {
  int Width = getBitWidth();
  if (isEmptySet() || R.isEmptySet())
    return MyConstantRange(Width, /*isFullSet=*/false);
  bool Table[1 << Width];
  for (int i = 0; i < (1 << Width); ++i)
    Table[i] = false;
  auto LI = getLower();
  do {
    auto RI = R.getLower();
    do {
      APInt Val = LI | RI;
      Table[Val.getLimitedValue()] = true;
      ++RI;
    } while (RI != R.getUpper());
    ++LI;
  } while (LI != getUpper());
  return xbestCR(Table, Width);
}
