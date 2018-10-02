import FWCore.ParameterSet.Config as cms

def customizeJetTools(process,jecDB,jecTag,jerDB,jerTag,baseJetCollection,runOnData):

	#general configurations
	jecTag += '_DATA' if runOnData else '_MC'
        jerTag += '_DATA' if runOnData else '_MC'
	payload='AK4PFchs'
	jecLevels=['L1FastJet','L2Relative','L3Absolute','L2L3Residual']
	print '[customizeJetTools]',jecDB,jecTag,payload,jecLevels

	#setup the source for JEC 
	process.load('CondCore.CondDB.CondDB_cfi')
	from CondCore.CondDB.CondDB_cfi import CondDB
	process.jec = cms.ESSource("PoolDBESSource",
				   DBParameters = cms.PSet(messageLevel = cms.untracked.int32(0)),
				   timetype = cms.string('runnumber'),
				   toGet = cms.VPSet( cms.PSet(record = cms.string('JetCorrectionsRecord'),
							       tag    = cms.string('JetCorrectorParametersCollection_%s_AK4PFchs'%jecTag),
							       label  = cms.untracked.string('AK4PFchs')
							       )						      
						      ), 
				   connect = cms.string('sqlite_file:%s'%jecDB)
				   )

	## add an es_prefer statement to resolve a possible conflict from simultaneous connection to a global tag
	process.es_prefer_jec = cms.ESPrefer('PoolDBESSource','jec')

        from CondCore.DBCommon.CondDBSetup_cfi import CondDBSetup
        process.jerDB = cms.ESSource('PoolDBESSource', CondDBSetup,
                                     connect = cms.string('sqlite_file:%s'%jerDB),
                                     toGet = cms.VPSet(cms.PSet(record = cms.string('JetResolutionRcd'),
                                                                tag = cms.string('JR_%s_PtResolution_AK4PFchs'%jerTag),
                                                                label = cms.untracked.string('AK4PFchs_pt')
                                                                ),
                                                       cms.PSet(record = cms.string('JetResolutionRcd'),
                                                                tag = cms.string('JR_%s_PhiResolution_AK4PFchs'%jerTag),
                                                                label = cms.untracked.string('AK4PFchs_phi')
                                                                ),
                                                       cms.PSet(record = cms.string('JetResolutionScaleFactorRcd'),
                                                                tag = cms.string('JR_%s_SF_AK4PFchs'%jerTag),
                                                                label = cms.untracked.string('AK4PFchs')
                                                                ),
                                                       )
                                     )        
        process.jerDBPreference = cms.ESPrefer('PoolDBESSource', 'jerDB')

	
	from PhysicsTools.PatAlgos.tools.jetTools import updateJetCollection
	updateJetCollection(
		process,
		labelName='UpdatedJECBTag',
		jetSource = cms.InputTag(baseJetCollection),
		jetCorrections = (payload, cms.vstring(jecLevels), 'None')
		)

	#MET
	from PhysicsTools.PatUtils.tools.runMETCorrectionsAndUncertainties import runMetCorAndUncFromMiniAOD
	runMetCorAndUncFromMiniAOD(process,isData=runOnData)
	

	process.load('RecoMET.METFilters.BadPFMuonFilter_cfi')
	process.BadPFMuonFilter.muons = cms.InputTag("slimmedMuons")
	process.BadPFMuonFilter.PFCandidates = cms.InputTag("packedPFCandidates")

	process.load('RecoMET.METFilters.BadChargedCandidateFilter_cfi')
	process.BadChargedCandidateFilter.muons = cms.InputTag("slimmedMuons")
	process.BadChargedCandidateFilter.PFCandidates = cms.InputTag("packedPFCandidates")

	process.extraMetFilters = cms.Path(process.BadPFMuonFilter*process.BadChargedCandidateFilter)
	#process.jetmetSeq=cms.Sequence(process.BadPFMuonFilter+process.BadChargedCandidateFilter)
