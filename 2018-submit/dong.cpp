MyConstantRange binaryAnd(const ConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  if (!isWrappedSet() && !Other.isWrappedSet() && !isFullSet() && !Other.isFullSet()) {
    unsigned width1 = ((getUpper() - 1) ^ getLower()).logBase2() + 1;
    unsigned width2 = ((Other.getUpper() - 1) ^ Other.getLower()).logBase2() + 1;
    APInt res1 = getLower().lshr(width1) << width1;
    APInt res2 = Other.getLower().lshr(width2) << width2;
    APInt res_high1 = getLower();
    APInt res_high2 = Other.getLower();
    res_high1.setLowBits(width1);
    res_high2.setLowBits(width2);
    if ((res1 & res2).isNullValue() && (res_high1 & res_high2).isAllOnesValue()) {
        return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
    }
    return MyConstantRange(res1 & res2, (res_high1 & res_high2) + 1);
  }

  APInt umin = APIntOps::umin(Other.getUnsignedMax(), getUnsignedMax());
  if (umin.isAllOnesValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(APInt::getNullValue(getBitWidth()), std::move(umin) + 1);
}

MyConstantRange binaryOr(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  if (!isWrappedSet() && !Other.isWrappedSet() && !isFullSet() && !Other.isFullSet()) {
    unsigned width1 = ((getUpper() - 1) ^ getLower()).getActiveBits();
    unsigned width2 = ((Other.getUpper() - 1) ^ Other.getLower()).getActiveBits();
    APInt res1 = getLower().lshr(width1) << width1;
    APInt res2 = Other.getLower().lshr(width2) << width2;
    APInt res_high1 = getLower();
    APInt res_high2 = Other.getLower();
    res_high1.setLowBits(width1);
    res_high2.setLowBits(width2);
    if ((res1 | res2).isNullValue() && (res_high1 | res_high2).isAllOnesValue()) {
        return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
    }
    return MyConstantRange(res1 | res2, (res_high1 | res_high2) + 1);
  }

  APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
  if (umax.isNullValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(std::move(umax), APInt::getNullValue(getBitWidth()));
}
