#include "mlir/Analysis/Presburger/Dependence.h"
#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/MPInt.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"
#include "mlir/Analysis/Presburger/Simplex.h"
#include "llvm/ADT/ArrayRef.h"
#include <vector>

void DependenceAnalysis::compute() {
  unsigned depth = 2 * sink.getNumDomainVars() + 1;
  for(unsigned level = depth; level >= 1; level--) {
    std::vector<PresburgerRelation> mustDeps, mayDeps;
    for(unsigned j = mustSources.size() - 1; j >= 0; j--) {
      PresburgerSpace s = PresburgerSpace::getRelationSpace(sink.getNumDomainVars(), mustSources[j].getNumDomainVars());
      mustDeps.push_back(PresburgerRelation::getEmpty(s));
      mayDeps.push_back(PresburgerRelation::getEmpty(s));
    }

    unsigned j = mustSources.size() - 1;
    for(; j >= 0; j--) {
      // TODO: If mustSources[j] cannot precede sink at this level continue
      mustDeps[j].unionInPlace(lastSource(mustNoSource, mustSources[j], level, mustNoSource));

      assert(intermediateSources(mustDeps, j, level)
          && "intermediateSources failed");

      mayDeps[j].unionInPlace(lastSource(mayNoSource, mustSources[j], level, mayNoSource));

      assert(intermediateSources(mayDeps, j, level)
          && "intermediateSources failed");

      if(mustNoSource.isIntegerEmpty() && mayNoSource.isIntegerEmpty())
        break;
    }

    for(; j >= 0; j--) {
      // TODO: If mustSources[j] cannot precede sink at this level continue

      assert(intermediateSources(mustDeps, j, level)
          && "intermediateSources failed");

      assert(intermediateSources(mayDeps, j, level)
          && "intermediateSources failed");
    }

    for(j = 0; j < maySources.size(); j++) {
      // TODO: If maySources[j] cannot precede sink at this level continue
      PresburgerRelation T = allSources(mayNoSource, j, level);
      maySourceMayDeps[j].unionInPlace(T);
      T = allSources(mustNoSource, j, level);
      maySourceMayDeps[j].unionInPlace(T);
      PresburgerSet foundSink = T.getRangeSet();
      mustNoSource.subtract(foundSink);
      mayNoSource.unionInPlace(foundSink);

      maySourceMayDeps[j] = allIntermediateSources(mustDeps, mayDeps, j, level);
    }

    for(j = mustSources.size(); j >= 0; j--) {
      mustSourceMustDeps[j].unionInPlace(mustDeps[j]);
      mustSourceMayDeps[j].unionInPlace(mayDeps[j]);
    }

    if(mustNoSource.isIntegerEmpty() && mayNoSource.isIntegerEmpty()) {
      break;
    }
  }
}

// TODO: upstream this into the PresburgerRelation class
static PresburgerRelation PREqual(PresburgerSpace space, int tillPos) {
  mlir::presburger::IntegerRelation ir =
    mlir::presburger::IntegerRelation::getUniverse(space);
  for(int i = 0, l = space.getNumDomainVars(); i < l && i < tillPos; i++) {
    std::vector<mlir::presburger::MPInt> vEq(space.getNumDimVars(), mlir::presburger::MPInt(0));
    vEq[i] = 1;
    vEq[space.getNumDomainVars() + i] = -1;
    ir.addEquality(vEq);
  }
  return PresburgerRelation(ir);
}

// TODO: upstream this into the PresburgerRelation class
static PresburgerRelation PRMoreAt(PresburgerSpace space, int pos) {
  mlir::presburger::IntegerRelation ir =
    mlir::presburger::IntegerRelation::getUniverse(space);
    std::vector<mlir::presburger::MPInt> vIneq(space.getNumDimVars(), mlir::presburger::MPInt(0));
    vIneq[pos] = 1;
    vIneq[space.getNumDomainVars() + pos] = -1;
    ir.addInequality(vIneq);
    return PresburgerRelation(ir);
}

static PresburgerRelation afterAtLevel(PresburgerSpace depSpace, int level) {
  if(level % 2)
    return PREqual(depSpace, level / 2);
  return PRMoreAt(depSpace, level / 2 - 1);
}

// Compute the last iterations of a given source writing to some address before
// the sink iterations read that address, at a given level for the sink
// iterations in setC. I.e. compute the dependence relation between a given
// source and the sink at a given level for the sink iterations in setC.
PresburgerRelation DependenceAnalysis::lastSource(PresburgerSet& setC, PresburgerRelation source, unsigned level, PresburgerSet& rest) {
  PresburgerRelation depMap = sink;
  source.inverse();
  depMap.compose(source);
  depMap = depMap.intersect(afterAtLevel(depMap.getSpace(), level));
  depMap = depMap.intersectDomain(setC);
  SymbolicLexOpt lexmax = depMap.findSymbolicIntegerLexMax();
  rest = setC.subtract(depMap.getDomainSet());
  return lexmax.lexopt.getAsRelation();
}

// Try to improve the current sinkLevelDeps by checking for each source k < j if
// k occurs in between sourceJ and the sink, at any level. Levels proceed form
// inner to outer as the closer a sourceK's level is to the sinkLevel, the
// latter it will be executed.
bool DependenceAnalysis::intermediateSources(std::vector<PresburgerRelation>& sinkLevelDeps, unsigned j, unsigned sinkLevel) {
  if(sinkLevelDeps[j].isIntegerEmpty())
    return true;
 
  int depth = 2 * mustSources[j].getNumDomainVars() + 1;

  for(int k = j - 1; k >= 0; k--) {
    // TODO: if sourceK cannot preceed sink at sinkLevel continue
    for(int level = sinkLevel; level <= depth; level++) {
      // TODO: if sourceJ cannot preceed sourceK at this level continue
      PresburgerSet trest = PresburgerSet::getEmpty(sinkLevelDeps[j].getSpace().getRangeSpace());
      PresburgerRelation lastLaterDeps = lastLaterSource(sinkLevelDeps[j], j, level, k, sinkLevel, trest);
      if(lastLaterDeps.isIntegerEmpty()) {
        continue;
      }
      sinkLevelDeps[j] = sinkLevelDeps[j].intersectDomain(trest);
      sinkLevelDeps[k].unionInPlace(lastLaterDeps);
    }
  }
  return true;
}

// source j
// .
// .
// source k <--- source k writes to same location as source j later
// .
// .
// sink
//
// Returns all such sink -> source k deps
PresburgerRelation DependenceAnalysis::lastLaterSource(PresburgerRelation curJDeps, int j, int afterLevel, int k, int sinkLevel, PresburgerSet &empty) {
  PresburgerRelation depMap = sink, writeMap = mustSources[k];
  writeMap.inverse();
  depMap.compose(writeMap);// sink -> mustSourceK iterations
  // s.t. sink reads from some location that mustSourceK writes to

  PresburgerSpace space = PresburgerSpace::getRelationSpace(mustSources[k].getNumDomainVars(), mustSources[j].getNumDomainVars());
  // All possible mustSourceK -> sourceJ iterations
  // s.t. mustSourceK iteration is after sourceJ iteration
  PresburgerRelation afterWrite = afterAtLevel(space, afterLevel);
  // All possible mustSourceK -> sink iterations
  // s.t. mustSourceK is after corresponding sourceJ dependence in curJDeps
  afterWrite.compose(curJDeps);
  afterWrite.inverse();
  depMap = depMap.intersect(afterWrite);// sink -> mustSourceK dependencies
  // s.t. sink reads from some location that mustSourceK writes to
  // and mustSourceK is after corresponding sourceJ dependence in curJDeps

  PresburgerRelation beforeSinkRead = afterAtLevel(depMap.getSpace(), sinkLevel);
  depMap = depMap.intersect(beforeSinkRead);// sink -> mustSourceK dependencies
  // s.t. sink reads from some location that mustSourceK writes to
  // and mustSourceK is after corresponding sourceJ dependence in curJDeps
  // and sink read is after SourceK write

  empty = curJDeps.getDomainSet().subtract(depMap.getDomainSet());
  depMap = depMap.intersectDomain(curJDeps.getDomainSet());
  SymbolicLexOpt result = depMap.findSymbolicIntegerLexMax();
  return result.lexopt.getAsRelation();
}

// Return all maySourceJ deps at given level for given sink iterations in setC
PresburgerRelation DependenceAnalysis::allSources(const PresburgerSet& setC, unsigned j, unsigned level) {
  PresburgerRelation depMap = sink.intersectDomain(setC), writeMap = maySources[j];
  writeMap.inverse();
  depMap.compose(writeMap);
  depMap.intersect(afterAtLevel(depMap.getSpace(), level));
  return depMap;
}

// Given the must and may dependence relations, check if any of the may accesses
// occur in between a must access and the sink. If so, Convert the must access
// into a may access.
PresburgerRelation DependenceAnalysis::allIntermediateSources(std::vector<PresburgerRelation> &mustDeps, std::vector<PresburgerRelation> &mayDeps, unsigned j, unsigned sinkLevel) {
  unsigned depth = maySources[j].getNumDomainVars() * 2 + 1;
  PresburgerRelation result = PresburgerRelation::getEmpty(PresburgerSpace::getRelationSpace(sink.getNumDomainVars(), maySources[j].getNumDomainVars()));

  for(unsigned k = 0; k < mustSources.size(); k++) {
    if(mustDeps[k].isIntegerEmpty() && mayDeps[k].isIntegerEmpty()) continue;
    for(unsigned level = sinkLevel; level <= depth; level++) {
      // TOOD: if sourceK cannot preceed sourceJ at this level continue
      result.unionInPlace(allLaterSources(mayDeps[k], j, sinkLevel, k, level));
      PresburgerRelation T = allLaterSources(mustDeps[k], j, sinkLevel, k, level);
      result.unionInPlace(T);
      PresburgerSet dom = T.getDomainSet();
      mayDeps[k].unionInPlace(mustDeps[k].intersectDomain(dom));
      PresburgerRelation removeFromMust = mustDeps[k].intersectDomain(dom);
      mustDeps[k] = mustDeps[k].subtract(removeFromMust);
    }
  }

  return result;
}

// must source k
// .
// .
// may source j <--- source j may write to same location as must source k later
// .
// .
// sink
// 
// Returns all such sink -> maySourceJ deps
PresburgerRelation DependenceAnalysis::allLaterSources(PresburgerRelation curKDeps, int j, int sinkLevel, int k, int afterLevel) {
  PresburgerRelation depMap = sink.intersectDomain(curKDeps.getRangeSet()), writeMap = maySources[j];
  writeMap.inverse();
  depMap.compose(writeMap);// sink -> maySourceJ iterations
  // s.t. each sink has a RAW dep with some mustSourceK
  // and sink reads from some location that maySourceJ writes to

  PresburgerSpace space = PresburgerSpace::getRelationSpace(maySources[j].getNumDomainVars(), mustSources[k].getNumDomainVars());
  // all possible maySourceJ -> mustSourceK iterations such that maySourceJ is after mustSourceK
  PresburgerRelation afterWrite = afterAtLevel(space, afterLevel);
  // all possible maySourceJ -> sink iterations such that maySourceJ is after
  // corresponding mustSourceK from RAW dep bw sink and mustSourceK
  afterWrite.compose(curKDeps);
  afterWrite.inverse();
  depMap.intersect(afterWrite);// sink -> maySourceJ iterations
  // s.t. each sink has a RAW dep with some mustSourceK
  // and maySourceJ is after mustSourceK from RAW dep bw sink and mustSourceK
  // and there is a RAW dep bw sink and maySourceJ

  // all possible sink -> maySourceJ iterations such that sink is after maySourceJ
  PresburgerRelation beforeRead = afterAtLevel(depMap.getSpace(), sinkLevel);
  depMap.intersect(beforeRead);// sink -> maySourceJ iterations
  // s.t. each sink has a RAW dep with some mustSourceK
  // and maySourceJ is after mustSourceK from RAW dep bw sink and mustSourceK
  // and sink reads from some location that maySourceJ writes to
  // and sink reads after maySourceJ writes
  // i.e. there is a RAW dep bw sink and maySourceJ

  return depMap;
}
