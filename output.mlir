module attributes {dlti.dl_spec = #dlti.dl_spec<#dlti.dl_entry<f64, dense<64> : vector<2xi32>>, #dlti.dl_entry<f128, dense<128> : vector<2xi32>>, #dlti.dl_entry<f16, dense<16> : vector<2xi32>>, #dlti.dl_entry<i32, dense<32> : vector<2xi32>>, #dlti.dl_entry<i16, dense<16> : vector<2xi32>>, #dlti.dl_entry<i8, dense<8> : vector<2xi32>>, #dlti.dl_entry<i1, dense<8> : vector<2xi32>>, #dlti.dl_entry<!llvm.ptr, dense<64> : vector<4xi32>>, #dlti.dl_entry<f80, dense<128> : vector<2xi32>>, #dlti.dl_entry<i64, dense<64> : vector<2xi32>>, #dlti.dl_entry<!llvm.ptr<271>, dense<32> : vector<4xi32>>, #dlti.dl_entry<!llvm.ptr<272>, dense<64> : vector<4xi32>>, #dlti.dl_entry<!llvm.ptr<270>, dense<32> : vector<4xi32>>, #dlti.dl_entry<"dlti.endianness", "little">, #dlti.dl_entry<"dlti.stack_alignment", 128 : i32>>, llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "polygeist.target-cpu" = "x86-64", "polygeist.target-features" = "+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87", "polygeist.tune-cpu" = "generic"} {
  memref.global @read_value : memref<8xi8> = uninitialized
  memref.global @i : memref<1xi64> = uninitialized
  memref.global @target_address : memref<1xmemref<?xi8>> = uninitialized
  func.func @_use(%arg0: memref<?xi8>) -> memref<?xi8> attributes {llvm.linkage = #llvm.linkage<external>} {
    return %arg0 : memref<?xi8>
  }
  func.func @f() -> i32 attributes {llvm.linkage = #llvm.linkage<external>} {
    %c0_i32 = arith.constant 0 : i32
    %c-86_i8 = arith.constant -86 : i8
    %alloca = memref.alloca() : memref<8xi8>
    %c0 = arith.constant 0 : index
    memref.store %c-86_i8, %alloca[%c0] : memref<8xi8>
    %c1 = arith.constant 1 : index
    memref.store %c-86_i8, %alloca[%c1] : memref<8xi8>
    %c2 = arith.constant 2 : index
    memref.store %c-86_i8, %alloca[%c2] : memref<8xi8>
    %c3 = arith.constant 3 : index
    memref.store %c-86_i8, %alloca[%c3] : memref<8xi8>
    %c4 = arith.constant 4 : index
    memref.store %c-86_i8, %alloca[%c4] : memref<8xi8>
    %c5 = arith.constant 5 : index
    memref.store %c-86_i8, %alloca[%c5] : memref<8xi8>
    %c6 = arith.constant 6 : index
    memref.store %c-86_i8, %alloca[%c6] : memref<8xi8>
    %c7 = arith.constant 7 : index
    memref.store %c-86_i8, %alloca[%c7] : memref<8xi8>
    %0 = memref.get_global @target_address : memref<1xmemref<?xi8>>
    %cast = memref.cast %alloca : memref<8xi8> to memref<?xi8>
    %c0_0 = arith.constant 0 : index
    memref.store %cast, %0[%c0_0] : memref<1xmemref<?xi8>>
    return %c0_i32 : i32
  }
  func.func @main() -> i32 attributes {llvm.linkage = #llvm.linkage<external>} {
    %c0_i32 = arith.constant 0 : i32
    %c42_i32 = arith.constant 42 : i32
    %c1_i64 = arith.constant 1 : i64
    %c8_i64 = arith.constant 8 : i64
    %c0_i64 = arith.constant 0 : i64
    %c-86_i8 = arith.constant -86 : i8
    %alloca = memref.alloca() : memref<8xi8>
    %c0 = arith.constant 0 : index
    memref.store %c-86_i8, %alloca[%c0] : memref<8xi8>
    %c1 = arith.constant 1 : index
    memref.store %c-86_i8, %alloca[%c1] : memref<8xi8>
    %c2 = arith.constant 2 : index
    memref.store %c-86_i8, %alloca[%c2] : memref<8xi8>
    %c3 = arith.constant 3 : index
    memref.store %c-86_i8, %alloca[%c3] : memref<8xi8>
    %c4 = arith.constant 4 : index
    memref.store %c-86_i8, %alloca[%c4] : memref<8xi8>
    %c5 = arith.constant 5 : index
    memref.store %c-86_i8, %alloca[%c5] : memref<8xi8>
    %c6 = arith.constant 6 : index
    memref.store %c-86_i8, %alloca[%c6] : memref<8xi8>
    %c7 = arith.constant 7 : index
    memref.store %c-86_i8, %alloca[%c7] : memref<8xi8>
    %0 = memref.get_global @target_address : memref<1xmemref<?xi8>>
    %cast = memref.cast %alloca : memref<8xi8> to memref<?xi8>
    %c0_0 = arith.constant 0 : index
    memref.store %cast, %0[%c0_0] : memref<1xmemref<?xi8>>
    %1 = memref.get_global @i : memref<1xi64>
    %c0_1 = arith.constant 0 : index
    memref.store %c0_i64, %1[%c0_1] : memref<1xi64>
    %2 = memref.get_global @read_value : memref<8xi8>
    %3 = scf.while (%arg0 = %c0_i64) : (i64) -> i64 {
      %4 = arith.cmpi slt, %arg0, %c8_i64 : i64
      scf.condition(%4) %arg0 : i64
    } do {
    ^bb0(%arg0: i64):
      %4 = arith.index_cast %arg0 : i64 to index
      %c0_2 = arith.constant 0 : index
      %5 = memref.load %0[%c0_2] : memref<1xmemref<?xi8>>
      %6 = memref.load %5[%4] : memref<?xi8>
      memref.store %6, %2[%4] : memref<8xi8>
      %c0_3 = arith.constant 0 : index
      %7 = memref.load %1[%c0_3] : memref<1xi64>
      %8 = arith.addi %7, %c1_i64 : i64
      %c0_4 = arith.constant 0 : index
      memref.store %8, %1[%c0_4] : memref<1xi64>
      scf.yield %8 : i64
    }
    call @_exit(%c42_i32) : (i32) -> ()
    return %c0_i32 : i32
  }
  func.func private @_exit(i32) attributes {llvm.linkage = #llvm.linkage<external>}
}

