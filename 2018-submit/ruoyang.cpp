MyConstantRange
binaryOr(const MyConstantRange &Other) const {

  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

    // outs() << "is wrapped " << isWrappedSet() << "\n";
    // outs() << "is Full " << isFullSet() << "\n";
    // getLower().print(outs(),false);
    // outs() << "\n";
    // getUpper().print(outs(),false);
    // outs() << "\n";

    // outs() << "is other wrapped " << Other.isWrappedSet() << "\n";
    // outs() << "is other full " << Other.isFullSet() << "\n";
    // Other.getLower().print(outs(),false);
    // outs() << "\n";
    // Other.getUpper().print(outs(),false);
    // outs() << "\n";

  if(isWrappedSet() && getUpper() != APInt::getMinValue(getBitWidth()))
  {
      MyConstantRange tmp_a = MyConstantRange(getLower(),APInt::getMinValue(getBitWidth()));
      MyConstantRange tmp_b = MyConstantRange(APInt::getMinValue(getBitWidth()),getUpper());

    if(Other.isWrappedSet() && Other.getUpper() != APInt::getMinValue(Other.getBitWidth())){
      MyConstantRange tmp_c = MyConstantRange(Other.getLower(),APInt::getMinValue(Other.getBitWidth()));
      MyConstantRange tmp_d = MyConstantRange(APInt::getMinValue(Other.getBitWidth()),Other.getUpper());

      MyConstantRange ret_a = tmp_a.binaryOr(tmp_c);
      MyConstantRange ret_b = tmp_a.binaryOr(tmp_d);
      MyConstantRange ret_c = tmp_b.binaryOr(tmp_c);
      MyConstantRange ret_d = tmp_b.binaryOr(tmp_d);
      ConstantRange ret = ret_a.unionWith(ret_b).unionWith(ret_c).unionWith(ret_d);
      return MyConstantRange(ret.getLower(),ret.getUpper());
    }
    else
    {
      MyConstantRange ret_a = tmp_a.binaryOr(Other);
      MyConstantRange ret_b = tmp_b.binaryOr(Other);
      ConstantRange ret = ret_a.unionWith(ret_b);
      return MyConstantRange(ret.getLower(),ret.getUpper());

    }
  }
  else{
    if(Other.isWrappedSet() && Other.getUpper() != APInt::getMinValue(Other.getBitWidth())){

      MyConstantRange tmp_c = MyConstantRange(Other.getLower(),APInt::getMinValue(Other.getBitWidth()));
      MyConstantRange tmp_d = MyConstantRange(APInt::getMinValue(Other.getBitWidth()),Other.getUpper());

      //outs() << "other wrapped" << "\n";
      MyConstantRange ret_a = binaryOr(tmp_c);
      MyConstantRange ret_b = binaryOr(tmp_d);

      //ret_a.print(outs());
      //ret_b.print(outs());
      ConstantRange ret = ret_a.unionWith(ret_b);
      return MyConstantRange(ret.getLower(),ret.getUpper());
    }
  }

  uint64_t a,b,c,d;
  if(isFullSet())
    a = 0;
  else
    a = getUnsignedMin().getZExtValue();

  b = getUnsignedMax().getZExtValue();

  if(Other.isFullSet())
    c = 0;
  else
    c = Other.getUnsignedMin().getZExtValue();

  d = Other.getUnsignedMax().getZExtValue();



  // std::cout << std::bitset<64>(a) <<std::endl;
  // std::cout << std::bitset<64>(b) <<std::endl;
  // std::cout << std::bitset<64>(c) <<std::endl;
  // std::cout << std::bitset<64>(d) <<std::endl;



  uint64_t m, temp;
  m = 0x8000000000000000;

  uint64_t negAandC = ~a & c;
  uint64_t AandnegC = a & ~c;

  while (m != 0) {
    if (negAandC & m) {
      temp = (a | m)& -m;
      if (temp <= b) {a = temp; break;}
      }
    else if (AandnegC & m) {
      temp = (c | m) & -m;
      if (temp <= d) {c = temp; break;}
      }
    m = m >> 1;
  }
  uint64_t min = a |c;

  if(isFullSet())
    a = 0;
  else
    a = getUnsignedMin().getZExtValue();

  b = getUnsignedMax().getZExtValue();

  if(Other.isFullSet())
    c = 0;
  else
    c = Other.getUnsignedMin().getZExtValue();

  d = Other.getUnsignedMax().getZExtValue();


  m = 0x8000000000000000;
  temp = 0;
  uint64_t BandD = b & d;
  while (m != 0) {
    if (BandD & m) {
      temp = (b - m) | (m - 1);
      if (temp >= a) {b = temp; break;}
      temp = (d - m) | (m - 1);
      if (temp >= c) {d = temp; break;}
      }
    m = m >> 1;
  }
  uint64_t max = b | d;
  //outs() << min<< "\t" << max << "\n";

  APInt a_max  = APInt(getBitWidth(),max + 1);
  APInt a_min  = APInt(getBitWidth(),min);
  if (a_min == a_max)
    return MyConstantRange(getBitWidth(),true);

  return MyConstantRange(a_min, a_max);

}

MyConstantRange
binaryAnd(const MyConstantRange &Other) const {
  if (isEmptySet() || Other.isEmptySet())
    return MyConstantRange(getBitWidth(), /*isFullSet=*/false);

    if(isWrappedSet() && getUpper() != APInt::getMinValue(getBitWidth()))
  {
      MyConstantRange tmp_a = MyConstantRange(getLower(),APInt::getMinValue(getBitWidth()));
      MyConstantRange tmp_b = MyConstantRange(APInt::getMinValue(getBitWidth()),getUpper());

    if(Other.isWrappedSet() && Other.getUpper() != APInt::getMinValue(Other.getBitWidth())){
      MyConstantRange tmp_c = MyConstantRange(Other.getLower(),APInt::getMinValue(Other.getBitWidth()));
      MyConstantRange tmp_d = MyConstantRange(APInt::getMinValue(Other.getBitWidth()),Other.getUpper());

      MyConstantRange ret_a = tmp_a.binaryAnd(tmp_c);
      MyConstantRange ret_b = tmp_a.binaryAnd(tmp_d);
      MyConstantRange ret_c = tmp_b.binaryAnd(tmp_c);
      MyConstantRange ret_d = tmp_b.binaryAnd(tmp_d);
      ConstantRange ret = ret_a.unionWith(ret_b).unionWith(ret_c).unionWith(ret_d);
      return MyConstantRange(ret.getLower(),ret.getUpper());
    }
    else
    {
      MyConstantRange ret_a = tmp_a.binaryAnd(Other);
      MyConstantRange ret_b = tmp_b.binaryAnd(Other);
      ConstantRange ret = ret_a.unionWith(ret_b);
      return MyConstantRange(ret.getLower(),ret.getUpper());

    }
  }
  else{
    if(Other.isWrappedSet() && Other.getUpper() != APInt::getMinValue(Other.getBitWidth())){

      MyConstantRange tmp_c = MyConstantRange(Other.getLower(),APInt::getMinValue(Other.getBitWidth()));
      MyConstantRange tmp_d = MyConstantRange(APInt::getMinValue(Other.getBitWidth()),Other.getUpper());

      //outs() << "other wrapped" << "\n";
      MyConstantRange ret_a = binaryAnd(tmp_c);
      MyConstantRange ret_b = binaryAnd(tmp_d);

      //ret_a.print(outs());
      //ret_b.print(outs());
      ConstantRange ret = ret_a.unionWith(ret_b);
      return MyConstantRange(ret.getLower(),ret.getUpper());
    }
  }

  uint64_t a,b,c,d;
  if(isFullSet())
    a = 0;
  else
    a = getUnsignedMin().getZExtValue();

  b = getUnsignedMax().getZExtValue();

  if(Other.isFullSet())
    c = 0;
  else
    c = Other.getUnsignedMin().getZExtValue();

  d = Other.getUnsignedMax().getZExtValue();

  uint64_t m, temp;
  m = 0x8000000000000000;
  uint64_t negAandnegC = ~a & ~c;
  while (m != 0) {
    if (negAandnegC & m) {
      temp = (a | m) & -m;
      if (temp <= b) {a = temp; break;}
      temp = (c | m) & -m;
      if (temp <= d) {c = temp; break;}
      }
    m = m >> 1;
  }
  uint64_t  minAND = a & c;

  if(isFullSet())
    a = 0;
  else
    a = getUnsignedMin().getZExtValue();

  b = getUnsignedMax().getZExtValue();

  if(Other.isFullSet())
    c = 0;
  else
    c = Other.getUnsignedMin().getZExtValue();

  d = Other.getUnsignedMax().getZExtValue();

  m = 0x8000000000000000;
  temp = 0;
  uint64_t BandnegD = b & ~d;
  uint64_t negBandD = ~b & d;

  while (m != 0) {
    if (BandnegD & m) {
     temp = (b & ~m) | (m - 1);
     if (temp >= a) {b = temp; break;}
    }
    else if (negBandD& m) {
      temp = (d & ~m) | (m - 1);
      if (temp >= c) {d = temp; break;}
    }
    m = m >> 1;
  }

  uint64_t maxAND =  b & d;
  
  APInt a_max  = APInt(getBitWidth(),maxAND + 1);
  APInt a_min  = APInt(getBitWidth(),minAND);
  if (a_min == a_max)
    return MyConstantRange(getBitWidth(),true);

  return MyConstantRange(a_min, a_max);
}