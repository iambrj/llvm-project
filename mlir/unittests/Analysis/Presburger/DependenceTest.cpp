#include "mlir/Analysis/Presburger/Dependence.h"
#include "Parser.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(DependenceAnalysisTest, lastSource) {
  PresburgerRelation mustSource = parsePresburgerRelationFromPresburgerSet();
  PresburgerRelation sink = parsePresburgerRelationFromPresburgerSet();
}

TEST(DependenceAnalysisTest, allSources) {
  PresburgerRelation mustSource = parsePresburgerRelationFromPresburgerSet();
  PresburgerRelation sink = parsePresburgerRelationFromPresburgerSet();
}

TEST(DependenceAnalysisTest, lastLaterSource) {
  PresburgerRelation mustSource = parsePresburgerRelationFromPresburgerSet();
  PresburgerRelation sink = parsePresburgerRelationFromPresburgerSet();
}

TEST(DependenceAnalysisTest, allLaterSources) {
  PresburgerRelation mustSource = parsePresburgerRelationFromPresburgerSet();
  PresburgerRelation sink = parsePresburgerRelationFromPresburgerSet();
}

// TODO
// TEST(DependenceAnalysisTest, intermediateSources) {
// }
// 
// TEST(DependenceAnalysisTest, allIntermediateTests) {
// }
// 
// TEST(DependenceAnalysisTest, onlyMustSource) {
// }
// 
// TEST(DependenceAnalysisTest, onlyMaySource) {
// }
// 
// TEST(DependenceAnalysisTest, mustAndMayNonKilling) {
// }
// 
// TEST(DependenceAnalysisTest, mustKillingMust) {
// }
// 
// TEST(DependenceAnalysisTest, mayKillingMust) {
// }
// 
// TEST(DependenceAnalysisTest, mustKillingMay) {
// }
// 
// TEST(DependenceAnalysisTest, mayKillingMay) {
// }
// 
// TEST(DependenceAnalysisTest, mixed) {
// }
