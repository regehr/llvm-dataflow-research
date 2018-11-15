  // your transfer functions go here

    KnownBits getRangeActiveBits(MyConstantRange range) const {
      unsigned bitWidth = range.getBitWidth();
      KnownBits ret (bitWidth);
      if (range.isEmptySet())
        return ret;

      if (range.isFullSet() || range.isWrappedSet()) {
        ret.One = APInt::getMaxValue(bitWidth).zextOrTrunc(bitWidth);
        return ret;
      }

      if (range.isSingleElement()) {
        ret.One = *range.getSingleElement();
        ret.Zero = ~ret.One;
        return ret;
      }

      APInt lower = range.getLower();
      APInt upper = range.getUpper() - 1;

      if (lower.getActiveBits() != upper.getActiveBits()) {
        ret.One = APInt::getOneBitSet(bitWidth, upper.getActiveBits() - 1);
        ret.Zero = ~(APInt::getMaxValue(upper.getActiveBits()).zextOrTrunc(bitWidth));
        return ret;
      }

      int idx = range.getUpper().getActiveBits() - 1;
      while (idx > 0) {
        if (upper[idx] == lower[idx]) {
          bool bit = upper[idx];
          if (bit)
            ret.One.setBit(idx);
          else
            ret.Zero.setBit(idx);
        }
        else
          break;
        idx--;
      }
      return ret;
    }

    APInt getTopMax(KnownBits Known) const {
      APInt ret (Known.getBitWidth(), 0);
      unsigned fillOne = std::min(Known.One.countTrailingZeros(), Known.Zero.countTrailingZeros());
      if (fillOne == 0)
        return Known.One | ~Known.Zero;
      APInt filler = APInt::getMaxValue(fillOne).zextOrTrunc(Known.getBitWidth());
      return Known.One | filler;
    }

    MyConstantRange binaryOr(const MyConstantRange &Other) const {
      if (isEmptySet() || Other.isEmptySet())
        return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

      if (isSingleElement() && Other.isSingleElement()) {
        APInt singleVal = getLower() | Other.getLower();
        return MyConstantRange(singleVal, singleVal + 1);
      }

      KnownBits selfMaxBits = getRangeActiveBits(*this);
      APInt selfTopMax = getTopMax(selfMaxBits);
      KnownBits otherfMaxBits = getRangeActiveBits(Other);
      APInt otherTopMax = getTopMax(otherfMaxBits);

      APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
      if (umax.isNullValue() && (selfTopMax | otherTopMax).isAllOnesValue())
        return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
      return MyConstantRange(std::move(umax), (selfTopMax | otherTopMax) + 1);
    }

    MyConstantRange binaryAnd(const MyConstantRange &Other) const {
      if (isEmptySet() || Other.isEmptySet())
        return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

      if (isSingleElement() && Other.isSingleElement()) {
        APInt singleVal = getLower() & Other.getLower();
        return MyConstantRange(singleVal, singleVal + 1);
      }

      KnownBits selfMaxBits = getRangeActiveBits(*this);
      KnownBits otherMaxBits = getRangeActiveBits(Other);

      APInt minInRanges = getUnsignedMax().ult(Other.getUnsignedMax()) ? this->getUnsignedMax() : Other.getUnsignedMax();
      KnownBits& maxBits = getUnsignedMax().ult(Other.getUnsignedMax()) ? otherMaxBits : selfMaxBits;

      APInt upper = minInRanges;
      for (int i = std::min(maxBits.Zero.getActiveBits(), upper.getActiveBits()) - 1; i >= 0; --i) {
        if (maxBits.One[i] == 0 && maxBits.Zero[i] == 0)
          break;
        if (maxBits.Zero[i]) {
          upper.clearBit(i);
          upper.setBits(0, i);
        }
      }
      if (upper.isAllOnesValue())
        return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
      return MyConstantRange(APInt::getNullValue(getBitWidth()), upper + 1);
    }

