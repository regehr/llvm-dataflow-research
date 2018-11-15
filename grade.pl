#!/usr/bin/perl -w

use strict;

# my $OPTS = "-O3 -DNDEBUG   -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS";
my $OPTS = "-O0 -g   -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS";

my @files = glob "2018-submit/*.cpp";

foreach my $file (@files) {
    die unless ($file =~ /^.*\/(.*)\.cpp$/);
    my $root = $1;
    print "\n================================================================\n";
    print "$file $root\n";
    system "rm -f funcs.cpp";
    system "cp $file funcs.cpp";
    system "rm -f a.out";
    system "/usr/bin/c++ -DUSER=\'\"$root\"\' -DTEST -I/home/regehr/llvm-install/include  -std=c++11  -fno-rtti $OPTS test-xfer.cpp /home/regehr/llvm-install/lib/libLLVMSupport.a /home/regehr/llvm-install/lib/libLLVMCore.a /home/regehr/llvm-install/lib/libLLVMIRReader.a /home/regehr/llvm-install/lib/libLLVMAsmParser.a /home/regehr/llvm-install/lib/libLLVMBitReader.a /home/regehr/llvm-install/lib/libLLVMCore.a /home/regehr/llvm-install/lib/libLLVMBinaryFormat.a /home/regehr/llvm-install/lib/libLLVMSupport.a -lz -lrt -ldl -ltinfo -lpthread -lm /home/regehr/llvm-install/lib/libLLVMDemangle.a";
    if (-f "a.out") {
	print "  ok!\n";
	system "./a.out";
    } else {
	print "  oops can't compile\n";
    }
}

system "rm -f funcs.cpp";
