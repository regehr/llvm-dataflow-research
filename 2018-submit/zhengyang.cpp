MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isSingleElement() && Other.isSingleElement())
    return MyConstantRange ((*getSingleElement()) & (*Other.getSingleElement()));

  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  APInt umin = APIntOps::umin(getUnsignedMax(), Other.getUnsignedMax());
  if (umin.isAllOnesValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);

  APInt RangeMin = APInt::getNullValue(getBitWidth());

  // Do not reason wrapping for now
  if ((!isWrappedSet() && !isSignWrappedSet()) &&
      (!Other.isWrappedSet() && !Other.isSignWrappedSet())) {

    APInt A_min = getUnsignedMin();
    APInt B_min = Other.getUnsignedMin();

    APInt hbit_set = APInt::getOneBitSet(getBitWidth(), getBitWidth() - 1);

    while(hbit_set != 0) {
      if ((hbit_set & (~A_min) & (~B_min)).getBoolValue()) {
        RangeMin = A_min & B_min & ~(hbit_set - 1);
        break;
      }
      hbit_set = hbit_set.lshr(1);
    }
  }

  return MyConstantRange(RangeMin, std::move(umin) + 1);
}
MyConstantRange
binaryOr(const MyConstantRange &Other) const {
  if (isSingleElement() && Other.isSingleElement())
    return MyConstantRange ((*getSingleElement()) | (*Other.getSingleElement()));

  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  // TODO: replace this with something less conservative

  APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
  if (umax.isNullValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);

  APInt RangeMax = APInt::getNullValue(getBitWidth());

  // Do not reason wrapping for now
  if ((!isWrappedSet() && !isSignWrappedSet()) &&
      (!Other.isWrappedSet() && !Other.isSignWrappedSet())) {

    APInt A_max = getUnsignedMax();
    APInt B_max = Other.getUnsignedMax();

    APInt hbit_set = APInt::getOneBitSet(getBitWidth(), getBitWidth() - 1);

    while(hbit_set != 0) {
      if ((hbit_set & A_max & B_max).getBoolValue()) {
        RangeMax = (A_max | (hbit_set - 1) | B_max) + 1;
        break;
      }
      hbit_set = hbit_set.lshr(1);
    }
  }

  return MyConstantRange(std::move(umax), std::move(RangeMax));
}
