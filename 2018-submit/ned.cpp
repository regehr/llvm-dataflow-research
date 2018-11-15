
MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);
  //Wrapped sets contain allZero and allOne; therefor we can get anything from zero to the ax of the (maybe) unwrapped one.
  if (isWrappedSet()){
	  return MyConstantRange(APInt::getNullValue(getBitWidth()), Other.getUnsignedMax());
  }
  if (Other.isWrappedSet()){
	  return MyConstantRange(APInt::getNullValue(getBitWidth()), getUnsignedMax());
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

  //Wrapped sets include allZero and allOne; therefor we can get from the min of the (maybe) unwrapped one all the way to the max possible.
  if (isWrappedSet()){
	  if (Other.getUnsignedMin().isNullValue()) return MyConstantRange(getBitWidth(), /*isFullSet=*/true);
	  return MyConstantRange(Other.getUnsignedMin(), APInt::getNullValue(getBitWidth()));
  }
  if (Other.isWrappedSet()){
	  if (getUnsignedMin().isNullValue()) MyConstantRange(getBitWidth(), /*isFullSet=*/true);
	  return MyConstantRange(getUnsignedMin(), APInt::getNullValue(getBitWidth()));
  }

  

  APInt umax = APIntOps::umax(getUnsignedMin(), Other.getUnsignedMin());
  bool overflow;
  APInt sum = getUnsignedMax().uadd_ov(Other.getUnsignedMax(), overflow);
  if (overflow && sum.uge(umax)) //Sum not only wrapped, but wrapped past min
	  return MyConstantRange(getBitWidth(), /*isFullSet=*/ true);
  APInt pastSum = sum++; //because upper is not included, we need this
  return MyConstantRange(std::move(umax), std::move(pastSum)); 

  //if (umax.isNullValue())
  //  return ConstantRange(getBitWidth(), /*isFullSet=*/true);
  //return ConstantRange(std::move(umax), APInt::getNullValue(getBitWidth()));

  //Logical outline:
  //if (this wraps)
  	//return other.min, other.max
  //if (other wraps)
  	//return this.min, this.max
  //else
        //return max(this.min, other.min), 2*(this.max, other.max)
		//Improve? NO.
}
