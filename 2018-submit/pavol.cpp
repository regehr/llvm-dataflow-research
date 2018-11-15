MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  // TODO: replace this with something less conservative

  APInt umin = APIntOps::umin(Other.getUnsignedMax(), getUnsignedMax());
  if (umin.isAllOnesValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(APInt::getNullValue(getBitWidth()), std::move(umin) + 1);
}

MyConstantRange
binaryOr(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

  // TODO: replace this with something less conservative

  APInt lower = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());

  unsigned active_bits = std::max(getUnsignedMax().getActiveBits(), getUnsignedMax().getActiveBits());
  APInt upper = APInt::getOneBitSet(getBitWidth(), active_bits); // upper of constant range is exclusive, 1111 + 1 -> 10000

  return MyConstantRange(std::move(lower), std::move(upper));
  
  APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
  if (umax.isNullValue())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
  return MyConstantRange(std::move(umax), APInt::getNullValue(getBitWidth()));
}
