MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
      return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  APInt umin = APIntOps::umin(Other.getUnsignedMax(), getUnsignedMax());
  if (umin.isAllOnesValue())
      return MyConstantRange(getBitWidth(), /*isFullSet=*/true);

  APInt res = APInt::getNullValue(getBitWidth());

  const APInt upper1 = getUnsignedMax();
  const APInt upper2 = Other.getUnsignedMax();
  APInt lower1 = getUnsignedMin();
  APInt lower2 = Other.getUnsignedMin();
  const APInt tmp = lower1 & lower2;
  const unsigned bitPos = getBitWidth() - tmp.countLeadingZeros();
  // if there are no zeros from bitPos upto both barriers, lower bound have bit
  // set at bitPos. Barrier is the point beyond which you cannot set the bit
  // because it will be greater than the upper bound then
  if (!isWrappedSet() && !Other.isWrappedSet() &&
      (lower1.countLeadingZeros() == upper1.countLeadingZeros()) &&
      (lower2.countLeadingZeros() == upper2.countLeadingZeros()) &&
      bitPos > 0)
  {
      lower1.lshrInPlace(bitPos - 1);
      lower2.lshrInPlace(bitPos - 1);
      if (lower1.countTrailingOnes() == (getBitWidth() - lower1.countLeadingZeros()) &&
          lower2.countTrailingOnes() == (getBitWidth() - lower2.countLeadingZeros()))
      {
          res = APInt::getOneBitSet(getBitWidth(), bitPos - 1);
      }
  }

  return MyConstantRange(std::move(res), std::move(umin) + 1);
}

MyConstantRange
binaryOr(const MyConstantRange &Other) const {
    if (isEmptySet() || Other.isEmptySet())
        return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

    APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
    APInt res = APInt::getNullValue(getBitWidth());
    if (!isWrappedSet() && !Other.isWrappedSet())
    {
        APInt umaxupper = APIntOps::umax(getUnsignedMax(), Other.getUnsignedMax());
        APInt uminupper = APIntOps::umin(getUnsignedMax(), Other.getUnsignedMax());
        res = APInt::getLowBitsSet(getBitWidth(),
                                   getBitWidth() - uminupper.countLeadingZeros());
        res = res | umaxupper;
        res = res + 1;
    }

    if (umax == res)
        return MyConstantRange(getBitWidth(), /*isFullSet=*/true);

    return MyConstantRange(std::move(umax), std::move(res));
}
