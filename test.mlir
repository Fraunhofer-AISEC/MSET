module attributes {dlti.dl_spec = #dlti.dl_spec<#dlti.dl_entry<f80, dense<128> : vector<2xi32>>, #dlti.dl_entry<i64, dense<64> : vector<2xi32>>, #dlti.dl_entry<!llvm.ptr<271>, dense<32> : vector<4xi32>>, #dlti.dl_entry<!llvm.ptr<270>, dense<32> : vector<4xi32>>, #dlti.dl_entry<!llvm.ptr<272>, dense<64> : vector<4xi32>>, #dlti.dl_entry<i32, dense<32> : vector<2xi32>>, #dlti.dl_entry<f128, dense<128> : vector<2xi32>>, #dlti.dl_entry<f64, dense<64> : vector<2xi32>>, #dlti.dl_entry<f16, dense<16> : vector<2xi32>>, #dlti.dl_entry<i1, dense<8> : vector<2xi32>>, #dlti.dl_entry<!llvm.ptr, dense<64> : vector<4xi32>>, #dlti.dl_entry<i16, dense<16> : vector<2xi32>>, #dlti.dl_entry<i8, dense<8> : vector<2xi32>>, #dlti.dl_entry<"dlti.stack_alignment", 128 : i32>, #dlti.dl_entry<"dlti.endianness", "little">>, llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "polygeist.target-cpu" = "x86-64", "polygeist.target-features" = "+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87", "polygeist.tune-cpu" = "generic"} {
  memref.global @target_addresses : memref<16xmemref<?xi8>> = uninitialized
  func.func @_use(%arg0: memref<?xi8>) -> memref<?xi8> attributes {llvm.linkage = #llvm.linkage<external>} {
    return %arg0 : memref<?xi8>
  }
  func.func @other_f() -> i32 attributes {llvm.linkage = #llvm.linkage<external>} {
    %c16_i32 = arith.constant 16 : i32
    %true = arith.constant true
    %false = arith.constant false
    %c16 = arith.constant 16 : index
    %c1 = arith.constant 1 : index
    %c0_i8 = arith.constant 0 : i8
    %c8 = arith.constant 8 : index
    %c42_i32 = arith.constant 42 : i32
    %c1_i32 = arith.constant 1 : i32
    %c281474976710655_i64 = arith.constant 281474976710655 : i64
    %c0_i32 = arith.constant 0 : i32
    %c0 = arith.constant 0 : index
    %alloca = memref.alloca() : memref<8xi8>
    %0 = "polygeist.memref2pointer"(%alloca) : (memref<8xi8>) -> !llvm.ptr
    %1 = llvm.ptrtoint %0 : !llvm.ptr to i64
    %2 = arith.andi %1, %c281474976710655_i64 : i64
    %3 = memref.get_global @target_addresses : memref<16xmemref<?xi8>>
    %4:2 = scf.for %arg0 = %c0 to %c16 step %c1 iter_args(%arg1 = %c0_i32, %arg2 = %true) -> (i32, i1) {
      %9:2 = scf.if %arg2 -> (i32, i1) {
        %10 = arith.index_cast %arg1 : i32 to index
        %11 = memref.load %3[%10] : memref<16xmemref<?xi8>>
        %12 = "polygeist.memref2pointer"(%11) : (memref<?xi8>) -> !llvm.ptr
        %13 = llvm.ptrtoint %12 : !llvm.ptr to i64
        %14 = arith.andi %13, %c281474976710655_i64 : i64
        %15 = arith.cmpi ne, %2, %14 : i64
        %16 = scf.if %15 -> (i32) {
          %17 = arith.addi %arg1, %c1_i32 : i32
          scf.yield %17 : i32
        } else {
          scf.yield %arg1 : i32
        }
        scf.yield %16, %15 : i32, i1
      } else {
        scf.yield %arg1, %false : i32, i1
      }
      scf.yield %9#0, %9#1 : i32, i1
    }
    %5 = arith.cmpi eq, %4#0, %c16_i32 : i32
    scf.if %5 {
      func.call @_exit(%c42_i32) : (i32) -> ()
    }
    %6 = memref.get_global @target_addresses : memref<16xmemref<?xi8>>
    %7 = arith.index_cast %4#0 : i32 to index
    %8 = affine.load %6[symbol(%7)] : memref<16xmemref<?xi8>>
    scf.for %arg0 = %c0 to %c8 step %c1 {
      memref.store %c0_i8, %8[%arg0] : memref<?xi8>
    }
    call @_exit(%c42_i32) : (i32) -> ()
    return %c0_i32 : i32
  }
  func.func private @_exit(i32) attributes {llvm.linkage = #llvm.linkage<external>}
  func.func @f() -> i32 attributes {llvm.linkage = #llvm.linkage<external>} {
    %c0 = arith.constant 0 : index
    %c16 = arith.constant 16 : index
    %c1 = arith.constant 1 : index
    %c-86_i8 = arith.constant -86 : i8
    %c0_i32 = arith.constant 0 : i32
    %c2 = arith.constant 2 : index
    %c3 = arith.constant 3 : index
    %c4 = arith.constant 4 : index
    %c5 = arith.constant 5 : index
    %c6 = arith.constant 6 : index
    %c7 = arith.constant 7 : index
    %alloca = memref.alloca() : memref<16x8xi8>
    scf.for %arg0 = %c0 to %c16 step %c1 {
      memref.store %c-86_i8, %alloca[%arg0, %c0] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c1] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c2] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c3] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c4] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c5] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c6] : memref<16x8xi8>
      memref.store %c-86_i8, %alloca[%arg0, %c7] : memref<16x8xi8>
    }
    %0 = memref.get_global @target_addresses : memref<16xmemref<?xi8>>
    scf.for %arg0 = %c0 to %c16 step %c1 {
      %1 = "polygeist.subindex"(%alloca, %arg0) : (memref<16x8xi8>, index) -> memref<?xi8>
      memref.store %1, %0[%arg0] : memref<16xmemref<?xi8>>
    }
    return %c0_i32 : i32
  }
  func.func @main() -> i32 attributes {llvm.linkage = #llvm.linkage<external>} {
    %c0_i32 = arith.constant 0 : i32
    %0 = call @f() : () -> i32
    %1 = call @other_f() : () -> i32
    return %c0_i32 : i32
  }
}
