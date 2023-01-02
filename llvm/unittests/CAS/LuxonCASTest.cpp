#include "llvm/Luxon/LuxonCAS.h"
#include "llvm/Testing/Support/Error.h"
#include "llvm/Testing/Support/SupportHelpers.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::cas;
using namespace llvm::cas::luxon;

TEST(LuxonCASTest, Extensions) {
  unittest::TempDir Temp("luxon-cas", /*Unique=*/true);
  std::unique_ptr<LuxonCAS> CAS;
  ASSERT_THAT_ERROR(LuxonCAS::create(Temp.path()).moveInto(CAS), Succeeded());
  Optional<ObjectID> ID1;
  ASSERT_THAT_ERROR(CAS->store("hello", {}).moveInto(ID1), Succeeded());

  Optional<LoadedObject> Obj1;
  ASSERT_THAT_ERROR(CAS->load(*ID1).moveInto(Obj1), Succeeded());
  ASSERT_TRUE(Obj1.has_value());
  EXPECT_EQ(Obj1->getData(), "hello");

  DigestRef Digest1 = ID1->getDigest(*CAS);
  Optional<ObjectID> ID2;
  ASSERT_THAT_ERROR(CAS->getID(Digest1).moveInto(ID2), Succeeded());
  EXPECT_EQ(ID1, ID2);

  ObjectID ID3 = CAS->getID("hello", {});
  EXPECT_EQ(*ID1, ID3);

  ObjectID ID4 = CAS->getID("world", {});
  ;
  Optional<LoadedObject> Obj2;
  ASSERT_THAT_ERROR(CAS->load(ID4).moveInto(Obj2), Succeeded());
  EXPECT_FALSE(Obj2.has_value());
  EXPECT_FALSE(CAS->containsObject(ID4));

  ASSERT_THAT_ERROR(CAS->storeForKnownID(ID4, "world", {}), Succeeded());
  Optional<LoadedObject> Obj3;
  ASSERT_THAT_ERROR(CAS->load(ID4).moveInto(Obj3), Succeeded());
  ASSERT_TRUE(Obj3.has_value());
  EXPECT_EQ(Obj3->getData(), "world");
  EXPECT_TRUE(CAS->containsObject(ID4));
}
