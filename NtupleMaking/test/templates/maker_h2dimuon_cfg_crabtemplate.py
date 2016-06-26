#
#   Ntuple Making Stage - Crab Template DATA
#   all the s.things will be replaced
#

import FWCore.ParameterSet.Config as cms
process = cms.Process("NtupleMaking")

#
#   loading sequences
#
process.load("Configuration.StandardSequences.MagneticField_38T_cff")
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 1000
process.load("Configuration.Geometry.GeometryIdeal_cff")
process.load('Configuration.EventContent.EventContent_cff')
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")

#
#   a few settings
#
thisIsData = s.isData
globalTag = s.globaltag
readFiles = cms.untracked.vstring();
readFiles.extend(s.files);

#
#   Differentiate between DATA and MC
#
if not thisIsData:
    process.load("Analysis.NtupleMaking.H2DiMuounMaker_MC")
    process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
    globalTag+="::All"
else:
    process.load("Analysis.NtupleMaking.H2DiMuounMaker_Data")
    process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff")

#
#   Pool Source with proper LSs
#
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )
process.source = cms.Source("PoolSource",fileNames = cms.untracked.vstring())
process.options   = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )

#
#   TFile Service to handle output
#
process.TFileService = cms.Service("TFileService", fileName = cms.string("ntuples_"+s.name+".root") )

#
#   Execution Path
#
process.p = cms.Path(process.ntuplemaker_H2DiMuonMaker)