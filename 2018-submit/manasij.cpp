MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  if (!isWrappedSet() && !Other.isWrappedSet()) {
    APInt MaxOfMax = APIntOps::umax(getUnsignedMax(), Other.getUnsignedMax());
    APInt Zero = MaxOfMax;
    Zero = 0;
    return MyConstantRange(Zero, MaxOfMax);
  }

  APInt umin = APIntOps::umin(Other.getUnsignedMax(), getUnsignedMax());
  if (umin.isAllOnesValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(APInt::getNullValue(getBitWidth()), std::move(umin) + 1);
}

MyConstantRange
binaryOr(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  if (!isWrappedSet() && !Other.isWrappedSet()) {
    APInt MaxOfMax = APIntOps::umax(getUnsignedMax(), Other.getUnsignedMax());
    auto HighestBitSet = MaxOfMax.ceilLogBase2();
    auto Low = MaxOfMax;
    Low = 0;
    auto High = Low;
    High.setBit(HighestBitSet + 1);
    return MyConstantRange(Low, High);
  }

  APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
  if (umax.isNullValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(std::move(umax), APInt::getNullValue(getBitWidth()));
}
