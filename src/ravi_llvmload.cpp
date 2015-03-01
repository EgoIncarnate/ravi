#include "ravi_llvmcodegen.h"

namespace ravi {

void RaviCodeGenerator::emit_MOVE(RaviFunctionDef *def, llvm::Value *L_ci,
                                  llvm::Value *proto, int A, int B) {

  // case OP_MOVE: {
  //  setobjs2s(L, ra, RB(i));
  //} break;

  // Load pointer to base
  llvm::Instruction *base_ptr = def->builder->CreateLoad(def->Ci_base);
  base_ptr->setMetadata(llvm::LLVMContext::MD_tbaa,
                        def->types->tbaa_luaState_ci_baseT);

  llvm::Value *src;
  llvm::Value *dest;

  lua_assert(A != B);

  if (A == 0) {
    // If A is 0 we can use the base pointer which is &base[0]
    dest = base_ptr;
  } else {
    // emit &base[A]
    dest = emit_array_get(def, base_ptr, A);
  }
  if (B == 0) {
    // If Bx is 0 we can use the base pointer which is &k[0]
    src = base_ptr;
  } else {
    // emit &base[B]
    src = emit_array_get(def, base_ptr, B);
  }

  // destvalue->value->i = srcvalue->value->i;
  llvm::Value *srcvalue = emit_gep(def, "srcvalue", src, 0, 0, 0);
  llvm::Value *destvalue = emit_gep(def, "destvalue", dest, 0, 0, 0);
  llvm::Instruction *store =
      def->builder->CreateStore(def->builder->CreateLoad(srcvalue), destvalue);
  store->setMetadata(llvm::LLVMContext::MD_tbaa, def->types->tbaa_TValue_nT);

  // destvalue->type = srcvalue->type
  llvm::Value *srctype = emit_gep(def, "srctype", src, 0, 1);
  llvm::Value *desttype = emit_gep(def, "desttype", dest, 0, 1);
  store =
      def->builder->CreateStore(def->builder->CreateLoad(srctype), desttype);
  store->setMetadata(llvm::LLVMContext::MD_tbaa, def->types->tbaa_TValue_ttT);
}

void RaviCodeGenerator::emit_LOADK(RaviFunctionDef *def, llvm::Value *L_ci,
                                   llvm::Value *proto, int A, int Bx) {
  //    case OP_LOADK: {
  //      TValue *rb = k + GETARG_Bx(i);
  //      setobj2s(L, ra, rb);
  //    } break;

  // Load pointer to base
  llvm::Instruction *base_ptr = def->builder->CreateLoad(def->Ci_base);
  base_ptr->setMetadata(llvm::LLVMContext::MD_tbaa,
                        def->types->tbaa_luaState_ci_baseT);

  // Load pointer to k
  llvm::Value *k_ptr = def->k_ptr;

  // LOADK requires a structure assignment
  // in LLVM as far as I can tell this requires a call to
  // an intrinsic memcpy
  llvm::Value *src;
  llvm::Value *dest;

  if (A == 0) {
    // If A is 0 we can use the base pointer which is &base[0]
    dest = base_ptr;
  } else {
    // emit &base[A]
    dest = emit_array_get(def, base_ptr, A);
  }
  if (Bx == 0) {
    // If Bx is 0 we can use the base pointer which is &k[0]
    src = k_ptr;
  } else {
    // emit &k[Bx]
    src = emit_array_get(def, k_ptr, Bx);
  }

#if 1
  // destvalue->value->i = srcvalue->value->i;
  llvm::Value *srcvalue = emit_gep(def, "srcvalue", src, 0, 0, 0);
  llvm::Value *destvalue = emit_gep(def, "destvalue", dest, 0, 0, 0);
  llvm::Instruction *store =
      def->builder->CreateStore(def->builder->CreateLoad(srcvalue), destvalue);
  store->setMetadata(llvm::LLVMContext::MD_tbaa, def->types->tbaa_TValue_nT);

  // destvalue->type = srcvalue->type
  llvm::Value *srctype = emit_gep(def, "srctype", src, 0, 1);
  llvm::Value *desttype = emit_gep(def, "desttype", dest, 0, 1);
  store =
      def->builder->CreateStore(def->builder->CreateLoad(srctype), desttype);
  store->setMetadata(llvm::LLVMContext::MD_tbaa, def->types->tbaa_TValue_ttT);

#else
  // First get the declaration for the inttrinsic memcpy
  llvm::SmallVector<llvm::Type *, 3> vec;
  vec.push_back(def->types->C_pcharT); /* i8 */
  vec.push_back(def->types->C_pcharT); /* i8 */
  vec.push_back(def->types->C_intT);
  llvm::Function *f = llvm::Intrinsic::getDeclaration(
      def->raviF->module(), llvm::Intrinsic::memcpy, vec);
  lua_assert(f);

  // Cast src and dest to i8*
  llvm::Value *dest_ptr =
      def->builder->CreateBitCast(dest, def->types->C_pcharT);
  llvm::Value *src_ptr = def->builder->CreateBitCast(src, def->types->C_pcharT);

  // Create call to intrinsic memcpy
  llvm::SmallVector<llvm::Value *, 5> values;
  values.push_back(dest_ptr);
  values.push_back(src_ptr);
  values.push_back(llvm::ConstantInt::get(def->types->C_intT, sizeof(TValue)));
  values.push_back(
      llvm::ConstantInt::get(def->types->C_intT, sizeof(L_Umaxalign)));
  values.push_back(def->types->kFalse);
  def->builder->CreateCall(f, values);
#endif
}
}