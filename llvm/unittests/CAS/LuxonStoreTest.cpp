#include "llvm/CAS/ObjectStore.h"
#include "llvm/Luxon/LuxonCAS.h"
#include "llvm/Testing/Support/Error.h"
#include "llvm/Testing/Support/SupportHelpers.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::cas;

TEST(LuxonCASTest, Extensions) {
  unittest::TempDir Temp("luxon-cas", /*Unique=*/true);
  std::unique_ptr<ObjectStore> CAS;
  ASSERT_THAT_ERROR(createLuxonCAS(Temp.path()).moveInto(CAS), Succeeded());
  Optional<ObjectRef> Ref1;
  ASSERT_THAT_ERROR(CAS->storeFromString({}, "hello").moveInto(Ref1),
                    Succeeded());

  Optional<ObjectProxy> Obj1;
  ASSERT_THAT_ERROR(luxon_ObjectStore_load(*CAS, *Ref1).moveInto(Obj1),
                    Succeeded());
  ASSERT_TRUE(Obj1.has_value());
  EXPECT_EQ(Obj1->getData(), "hello");

  ArrayRef<uint8_t> Hash1 = luxon_ObjectStore_getHash(*CAS, *Ref1);
  Optional<ObjectRef> Ref2;
  ASSERT_THAT_ERROR(
      luxon_ObjectStore_createRefFromHash(*CAS, Hash1).moveInto(Ref2),
      Succeeded());
  EXPECT_EQ(Ref1, Ref2);

  ObjectRef Ref3 = luxon_ObjectStore_refDigest(
      *CAS, {}, arrayRefFromStringRef<char>("hello"));
  EXPECT_EQ(*Ref1, Ref3);

  ObjectRef Ref4 = luxon_ObjectStore_refDigest(
      *CAS, {}, arrayRefFromStringRef<char>("world"));
  Optional<ObjectProxy> Obj2;
  ASSERT_THAT_ERROR(luxon_ObjectStore_load(*CAS, Ref4).moveInto(Obj2),
                    Succeeded());
  EXPECT_FALSE(Obj2.has_value());
  EXPECT_FALSE(luxon_ObjectStore_containsRef(*CAS, Ref4));

  ASSERT_THAT_ERROR(luxon_ObjectStore_storeForRef(
                        *CAS, Ref4, {}, arrayRefFromStringRef<char>("world")),
                    Succeeded());
  Optional<ObjectProxy> Obj3;
  ASSERT_THAT_ERROR(luxon_ObjectStore_load(*CAS, Ref4).moveInto(Obj3),
                    Succeeded());
  ASSERT_TRUE(Obj3.has_value());
  EXPECT_EQ(Obj3->getData(), "world");
  EXPECT_TRUE(luxon_ObjectStore_containsRef(*CAS, Ref4));
}
