#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"
#include "mlir/Analysis/Presburger/Simplex.h"

using llvm::ArrayRef;
using mlir::presburger::PresburgerSpace;
using mlir::presburger::PresburgerRelation;
using mlir::presburger::PresburgerSet;
using mlir::presburger::SymbolicLexOpt;

class DependenceAnalysis {
  protected:
    const PresburgerRelation sink;
    ArrayRef<PresburgerRelation> mustSources, maySources;
    // mustNoSource are those sink iterations for which no dependencies are
    // found and mayNoSource are those sink iterations for which only may
    // dependencies are found.
    PresburgerSet mustNoSource, mayNoSource;
    std::vector<PresburgerRelation> mustSourceMustDeps, mustSourceMayDeps, maySourceMayDeps;

  public:
    PresburgerRelation lastSource(PresburgerSet& setC, PresburgerRelation source, unsigned level, PresburgerSet& rest);
    bool intermediateSources(std::vector<PresburgerRelation>& sources, unsigned j, unsigned level);
    PresburgerRelation allSources(const PresburgerSet &setC, unsigned j, unsigned level);
    PresburgerRelation allIntermediateSources(std::vector<PresburgerRelation>&, std::vector<PresburgerRelation>&, unsigned, unsigned);
    PresburgerRelation lastLaterSource(PresburgerRelation curJDeps, int j, int afterLevel, int k, int sinkLevel, PresburgerSet &trest);
    PresburgerRelation allLaterSources(PresburgerRelation curKDeps, int j, int sinkLevel, int k, int afterLevel);
    bool canPreceedAtLevel(int j, int k, int level);
    DependenceAnalysis(PresburgerRelation sink, ArrayRef<PresburgerRelation> maySources,
        ArrayRef<PresburgerRelation> mustSources) : sink(sink),
    mustSources(mustSources), maySources(maySources),
    mustNoSource(sink.getDomainSet()),
    mayNoSource(PresburgerSet::getEmpty(sink.getSpace().getDomainSpace())) {
      for(const PresburgerRelation& mustSource : mustSources) {
        PresburgerSpace s = PresburgerSpace::getRelationSpace(sink.getNumDomainVars(), mustSource.getNumDomainVars());
        mustSourceMustDeps.push_back(PresburgerRelation::getEmpty(s));
        mustSourceMayDeps.push_back(PresburgerRelation::getEmpty(s));
      }
      for(const PresburgerRelation& maySource : maySources) {
        PresburgerSpace s = PresburgerSpace::getRelationSpace(sink.getNumDomainVars(), maySource.getNumDomainVars());
        maySourceMayDeps.push_back(PresburgerRelation::getEmpty(s));
      }
    }
    void compute();
    const PresburgerSet& getMustNoSource();
    const PresburgerSet& getMayNoSource();
};
