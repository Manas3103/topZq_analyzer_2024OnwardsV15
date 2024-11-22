"""
File contains job options 
"""


# options for Nanoaodrdframe
config = {
        
        # tree name of input file(s)
        'intreename': "Events",

        # tree name of output file(s) it cannot be the same as the input tree name or it'll crash
        'outtreename': "outputTree",

        #data year (2016,2017,2018)
        'year': 2017,

        # is ReReco or Ultra Legacy
        'runtype': 'UL',

        'datatype': -1, # 0=MC ; 1=DATA ; -1=Auto


        #for correction
        
        # good json file
        # 'goodjson': 'data/Cert_294927-306462_13TeV_EOY2017ReReco_Collisions17_JSON.txt',
        'goodjson' : 'data/Legacy_RunII/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt',

        # pileup weight for MC
        'pileupfname': 'data/LUM/2017_UL/puWeights.json',

        'pileuptag': 'Collisions17_UltraLegacy_goldenJSON',

        # json filename for BTV correction
        'btvfname': 'data/BTV/2017_UL/btagging.json',

        # BTV correction type
        #'btvtype': 'deepJet_shape',
        'btvtype': 'deepJet_mujets',
        'fname_btagEff': 'data/BTV/2017_UL/BtaggingEfficiency.root',
        'hname_btagEff_bcflav': 'h_btagEff_bcflav',
        'hname_btagEff_lflav': 'h_btagEff_lflav',
    
        # Muon Correction 
        'muon_roch_fname': 'data/MUO/2017_UL/RoccoR2017UL.txt', 
        'muon_fname': 'data/MUO/2017_UL/muon_Z.json.gz', 
        'muonHLTtype': 'NUM_IsoMu27_DEN_CutBasedIdTight_and_PFIsoTight',
        'muonRECOtype': 'NUM_TrackerMuons_DEN_genTracks',
        'muonIDtype': 'NUM_MediumID_DEN_TrackerMuons',
        'muonISOtype': 'NUM_TightRelIso_DEN_MediumID',
        #'muontype': 'NUM_TightRelIso_DEN_MediumID',#'Medium ISO UL scale factor',

        # Electron Correction 
        'electron_fname': 'data/EGM/2017_UL/electron.json.gz', 
        'electron_reco_type': 'RecoAbove20',
        'electron_id_type': 'Tight',#'Tight ID UL scale factor',
        #'muontype': 'NUM_TightRelIso_DEN_MediumID',#'Medium ISO UL scale factor',


        # json file name for JERC
        'jercfname': 'data/JERC/UL17_jerc.json',

        # conbined correction type for jets
        'jerctag': 'Summer19UL17_V5_MC_L1L2L3Res_AK4PFchs', 

        # jet uncertainty 
        'jercunctag': 'Summer19UL17_V5_MC_Total_AK4PFchs', 
        
        }

# processing options
procflags = {
        ###### how many jobs? ######
        #'split': 'Max', #only use 'Max' when nrootfiles = 'All'
        'split': 1,

        ###### if False, one output file per input file, if True then one output file for everything ######
       # 'allinone': False,
        'allinone': True,

        ###### if True then skip existing analyzed files ######
        'skipold': True,

        ###### travel through the subdirectories and their subdirecties when processing. ######
        ###### becareful not to mix MC and real DATA in them. ######
        'recursive': True,

        ###### if False then only selected branches which is done in the .cpp file will be saved ######
        'saveallbranches': False,
        
        ###### How many input files? ######
        #'nrootfiles': 'All',
        'nrootfiles': 5,

        ###### Make a copy of the analyzer in the same directory as the analyzed root files ######
        'copyInstance': False,
        }

# input directory where your input root tree resides
#data='/eos/lyoeos.in2p3.fr/grid/cms/store/data/'
#mc17 = '/eos/lyoeos.in2p3.fr/grid/cms/store/mc/RunIISummer20UL17NanoAODv9'
#outDir = '/eos/lyoeos.in2p3.fr/grid/cms/store/user/apurohit/SingleTop_Data_Oct2023_Results'
#outDir = '/eos/lyoeos.in2p3.fr/grid/cms/store/user/apurohit/SingleTop_MC_Results_Oct2023'
#outDir = '/uscms/home/msahoo/nobackup/Project_tzq/CMSSW_12_3_4/src/single_top_analyser/output_file'
# output directory where your output should go
# dump of stderr/stdout to file

"""
nanoaod_inputdir_outputdir_pairs = [
        ['/uscms/home/msahoo/nobackup/Project_tzq/root_files',
    'output_file/test1Sept.root',
    'errr.out']
]
"""
"""
nanoaod_inputdir_outputdir_pairs = [
        [' /tZq_ll_4f_ckm_NLO_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
    'output_file/top_tzq_signal.root',
    'top_err.out'],
        ['/DYJetsToLL_M-50_HT-100to200_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
            'output_file/DYjet_100to200.root',
            'DYjet100_err.out'],
        ['/DYJetsToLL_M-50_HT-70to100_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
            'output_file/DYjet_70to100.root',
            'DYjet70_err.out'],
        ['/TTTo2L2Nu_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIISummer16NanoAODv7-PUMoriond17_Nano02Apr2020_102X_mcRun2_asymptotic_v8-v1/NANOAODSIM',
            'output_file/TTto2l2nu.root',
            'TTto2l2nu_err.out']

]
"""
"""
nanoaod_inputdir_outputdir_pairs = [
    ['/DYJetsToLL_M-50_HT-200to400_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-200to400.root',
     'DYJetsToLL_M-50_HT-200to400_err.out'],

    ['/DYJetsToLL_M-50_HT-400to600_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-400to600.root',
     'DYJetsToLL_M-50_HT-400to600_err.out'],

    ['/DYJetsToLL_M-50_HT-600to800_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-600to800.root',
     'DYJetsToLL_M-50_HT-600to800_err.out'],

    ['/DYJetsToLL_M-50_HT-800to1200_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-800to1200.root',
     'DYJetsToLL_M-50_HT-800to1200_err.out'],

    ['/DYJetsToLL_M-50_HT-1200to2500_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-1200to2500.root',
     'DYJetsToLL_M-50_HT-1200to2500_err.out'],

    ['/DYJetsToLL_M-50_HT-2500toInf_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/DYJetsToLL_M-50_HT-2500toInf.root',
     'DYJetsToLL_M-50_HT-2500toInf_err.out'],

    ['/TTZToLLNuNu_M-10_TuneCP5_PSweights_13TeV-amcatnlo-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/TTZToLLNuNu_M-10.root',
     'TTZToLLNuNu_M-10_err.out']
]
"""
"""
nanoaod_inputdir_outputdir_pairs = [
    ['/TTToSemiLeptonic_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/TTToSemiLeptonic.root',
     'TTToSemiLeptonic_err.out'],

    ['/ST_t-channel_antitop_4f_InclusiveDecays_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ST_t-channel_antitop.root',
     'ST_t-channel_antitop_err.out'],

    ['/ST_t-channel_top_4f_InclusiveDecays_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ST_t-channel_top.root',
     'ST_t-channel_top_err.out'],

    ['/WJetsToLNu_TuneCP5_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WJetsToLNu.root',
     'WJetsToLNu_err.out'],

    ['/WGToLNuG_TuneCP5_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WGToLNuG.root',
     'WGToLNuG_err.out'],

    ['/ZGToLLG_01J_5f_lowMLL_lowGPt_TuneCP5_13TeV-amcatnloFXFX-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/ZGToLLG.root',
     'ZGToLLG_err.out'],

    ['/TTGamma_Dilept_TuneCP5_PSweights_13TeV-madgraph-pythia8/RunIISummer16NanoAODv7-PUMoriond17_Nano02Apr2020_102X_mcRun2_asymptotic_v8-v1/NANOAODSIM',
     'output_file/TTGamma_Dilept.root',
     'TTGamma_Dilept_err.out']
]
"""
"""
nanoaod_inputdir_outputdir_pairs = [
    ['/ZZZ_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/ZZZ.root',
     'ZZZ_err.out'],

    ['/WZZ_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WZZ.root',
     'WZZ_err.out'],

    ['/WWZ_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WWZ.root',
     'WWZ_err.out'],

    ['/WWW_4F_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WWW.root',
     'WWW_err.out'],

    ['/WWTo2L2Nu_NNPDF31_TuneCP5_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/WWTo2L2Nu.root',
     'WWTo2L2Nu_err.out'],

    ['/ZZTo2L2Q_13TeV_amcatnloFXFX_madspin_pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ZZTo2L2Q.root',
     'ZZTo2L2Q_err.out'],

    ['/ZZTo2L2Nu_13TeV_powheg_pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ZZTo2L2Nu.root',
     'ZZTo2L2Nu_err.out'],

    ['/WZTo2L2Q_13TeV_amcatnloFXFX_madspin_pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/WZTo2L2Q.root',
     'WZTo2L2Q_err.out'],

    ['/ST_tW_top_5f_inclusiveDecays_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_new_pmx_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ST_tW_top.root',
     'ST_tW_top_err.out'],

    ['/ST_tW_antitop_5f_inclusiveDecays_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_new_pmx_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ST_tW_antitop.root',
     'ST_tW_antitop_err.out'],

    ['/TTWH_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v2/NANOAODSIM',
     'output_file/TTWH.root',
     'TTWH_err.out'],

    ['/TTZH_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v2/NANOAODSIM',
     'output_file/TTZH.root',
     'TTZH_err.out'],

    ['/TTHH_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v2/NANOAODSIM',
     'output_file/TTHH.root',
     'TTHH_err.out'],

    ['/TTWW_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v2/NANOAODSIM',
     'output_file/TTWW.root',
     'TTWW_err.out'],

    ['/TTWZ_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/TTWZ.root',
     'TTWZ_err.out'],

    ['/TTZZ_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/TTZZ.root',
     'TTZZ_err.out'],

    ['/TTTT_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v2/NANOAODSIM',
     'output_file/TTTT.root',
     'TTTT_err.out'],

    ['/TTWJetsToLNu_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/TTWJetsToLNu.root',
     'TTWJetsToLNu_err.out']
]
"""
"""
nanoaod_inputdir_outputdir_pairs = [
        ['/DYJetsToLL_M-50_TuneCP5_13TeV-madgraphMLM-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
             'output_file/DYJetsToLL_M-50.root',
             'DYJetsToLL_M-50_err.out'],
        ['/TTTo2L2Nu_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIISummer16NanoAODv7-PUMoriond17_Nano02Apr2020_102X_mcRun2_asymptotic_v8-v1/NANOAODSIM',
            'output_file/TTto2l2nu.root',
            'TTto2l2nu_err.out']
]

"""
nanoaod_inputdir_outputdir_pairs = [
    ['/ST_tW_antitop_5f_inclusiveDecays_TuneCP5_PSweights_13TeV-powheg-pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ST_tW_antitop_5f_inclusiveDecays.root',
     'ST_tW_antitop_5f_inclusiveDecays_err.out'],

    ['/TTWW_TuneCP5_13TeV-madgraph-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/TTWW.root',
     'TTWW_err.out'],

    ['/WZZ_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/WZZ.root',
     'WZZ_err.out'],

    ['/ZZTo2L2Q_13TeV_amcatnloFXFX_madspin_pythia8/RunIIFall17NanoAODv7-PU2017_12Apr2018_Nano02Apr2020_102X_mc2017_realistic_v8-v1/NANOAODSIM',
     'output_file/ZZTo2L2Q.root',
     'ZZTo2L2Q_err.out'],

    ['/ZZZ_TuneCP5_13TeV-amcatnlo-pythia8/RunIISummer20UL17NanoAODv9-106X_mc2017_realistic_v9-v1/NANOAODSIM',
     'output_file/ZZZ.root',
     'ZZZ_err.out']
]









"""








nanoaod_inputdir_outputdir_pairs = [

    # ['/eos/lyoeos.in2p3.fr/grid/cms/store/mc/RunIISummer20UL17NanoAODv9/ST_t-channel_top_4f_InclusiveDecays_TuneCP5_13TeV-powheg-madspin-pythia8/NANOAODSIM/106X_mc2017_realistic_v9-v1/30000/2D19CD0F-4A29-4D43-8414-82BDCCA04783.root', 'test.root', 'outputTest.out'],
        
        # [mc17 + '/ST_t-channel_top_4f_InclusiveDecays_TuneCP5_13TeV-powheg-madspin-pythia8 ', outDir + '/PROC_ST_t-channel_top_UL17.root', outDir +  '/PROC_ST_t-channel_top_UL17.out' ], #197
        # [mc17 + 'tZq_ll_4f_ckm_NLO_TuneCP5_PSweights_13TeV-amcatnlo-pythia8 ', outDir + '/PROC_ST_t-channel_top_UL17.root', outDir +  '/PROC_ST_t-channel_top_UL17.out' ]
        # [mc17 + '/ST_t-channel_antitop_4f_InclusiveDecays_TuneCP5_13TeV-powheg-madspin-pythia8', outDir +  '/PROC_ST_t-channel_antitop_UL17.root', outDir +  '/PROC_ST_t-channel_antitop_UL17.out'], #60

        # [mc17 + '/ST_tW_top_5f_inclusiveDecays_TuneCP5_13TeV-powheg-pythia8', outDir + '/PROC_ST_tW-channel_top_UL17.root', outDir + '/PROC_ST_tW-channel_top_UL17.out'], #43
        # [mc17 + '/ST_tW_antitop_5f_inclusiveDecays_TuneCP5_13TeV-powheg-pythia8', outDir + '/PROC_ST_tW-channel_antitop_UL17.root', outDir + '/PROC_ST_tW-channel_antitop_UL17.out'], #48
        # [mc17 + '/ST_s-channel_4f_leptonDecays_TuneCP5_13TeV-amcatnlo-pythia8', outDir + '/PROC_ST_s-channel_UL17.root', outDir + '/PROC_ST_s-channel_UL17.out'], #16

        # [mc17 + '/DYJetsToLL_M-10to50_TuneCP5_13TeV-madgraphMLM-pythia8', outDir + '/PROC_DYJetsToLL_M-10to50_UL17.root', outDir + '/PROC_DYJetsToLL_M-10to50_UL17.out'], #77
        # [mc17 + '/DYJetsToLL_M-50_TuneCP5_13TeV-madgraphMLM-pythia8', outDir + '/PROC_DYJetsToLL_M-50_UL17.root', outDir + '/PROC_DYJetsToLL_M-50_UL17.out'], #75

        # [mc17 + '/TTTo2L2Nu_TuneCP5_13TeV-powheg-pythia8', outDir + '/PROC_TTTo2L2Nu_UL17.root', outDir + '/PROC_TTTo2L2Nu_UL17.out'], #99
        # [mc17 + '/TTToSemiLeptonic_TuneCP5_13TeV-powheg-pythia8', outDir + '/PROC_TTToSemiLeptonic_UL17.root', outDir + '/PROC_TTToSemiLeptonic_UL17.out'], #297
        # [mc17 + '/TTWJetsToLNu_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8', outDir + '/PROC_TTWJetsToLNu_UL17.root', outDir + '/PROC_TTWJetsToLNu_UL17.out'], #9
        # [mc17 + '/TTWJetsToQQ_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8', outDir + '/PROC_TTWJetsToQQ_UL17.root', outDir + '/PROC_TTWJetsToQQ_UL17.out'], #10
        # [mc17 + '/TTZToLLNuNu_M-10_TuneCP5_13TeV-amcatnlo-pythia8', outDir + '/PROC_TTZToLLNuNu_UL17.root', outDir + '/PROC_TTZToLLNuNu_UL17.out'], #19
        # [mc17 + '/TTZToQQ_TuneCP5_13TeV-amcatnlo-pythia8', outDir + '/PROC_TTZToQQ_UL17.root', outDir + '/PROC_TTZToQQ_UL17.out'], #35
     
        # [mc17 + '/WJetsToLNu_TuneCP5_13TeV-madgraphMLM-pythia8', outDir + '/PROC_WJetsToLNu_UL17.root', outDir + '/PROC_WJetsToLNu_UL17.out'], #81

        # [mc17 + '/WW_TuneCP5_13TeV-pythia8', outDir + '/PROC_WW_UL17.root', outDir + '/PROC_WW_UL17.out'], #16
        # [mc17 + '/WZ_TuneCP5_13TeV-pythia8', outDir + '/PROC_WZ_UL17.root', outDir + '/PROC_WZ_UL17.out'], #20
        # [mc17 + '/ZZ_TuneCP5_13TeV-pythia8', outDir + '/PROC_ZZ_UL17.root', outDir + '/PROC_ZZ_UL17.out'], #2

        # [mc17 + '/QCD_Pt-15to20_EMEnriched_TuneCP5_13TeV-pythia8'      ,  outDir + '/PROC_QCD_Pt-15to20_EMEnriched.root'  ,  outDir + '/PROC_QCD_Pt-15to20_EMEnriched.out'], #25
        # [mc17 + '/QCD_Pt-20to30_EMEnriched_TuneCP5_13TeV-pythia8'      ,  outDir + '/PROC_QCD_Pt-20to30_EMEnriched.root'  ,  outDir + '/PROC_QCD_Pt-20to30_EMEnriched.out'], #72
        # [mc17 + '/QCD_Pt-30to50_EMEnriched_TuneCP5_13TeV-pythia8'      ,  outDir + '/PROC_QCD_Pt-30to50_EMEnriched.root'  ,  outDir + '/PROC_QCD_Pt-30to50_EMEnriched.out'], #7
        # [mc17 + '/QCD_Pt-50to80_EMEnriched_TuneCP5_13TeV-pythia8'      ,  outDir + '/PROC_QCD_Pt-50to80_EMEnriched.root'  ,  outDir + '/PROC_QCD_Pt-50to80_EMEnriched.out'], #8
        # [mc17 + '/QCD_Pt-80to120_EMEnriched_TuneCP5_13TeV-pythia8'     ,  outDir + '/PROC_QCD_Pt-80to120_EMEnriched.root' ,  outDir + '/PROC_QCD_Pt-80to120_EMEnriched.out'], #21
        # [mc17 + '/QCD_Pt-120to170_EMEnriched_TuneCP5_13TeV-pythia8'    ,  outDir + '/PROC_QCD_Pt-120to170_EMEnriched.root',  outDir + '/PROC_QCD_Pt-120to170_EMEnriched.out'], #37
        # [mc17 + '/QCD_Pt-170to300_EMEnriched_TuneCP5_13TeV-pythia8'    ,  outDir + '/PROC_QCD_Pt-70to300_EMEnriched.root' ,  outDir + '/PROC_QCD_Pt-70to300_EMEnriched.out'], #5
        # [mc17 + '/QCD_Pt-300toInf_EMEnriched_TuneCP5_13TeV-pythia8'    ,  outDir + '/PROC_QCD_Pt-300toInf_EMEnriched.root',  outDir + '/PROC_QCD_Pt-300toInf_EMEnriched.out'], #5

        # [mc17 + '/QCD_Pt-15To20_MuEnrichedPt5_TuneCP5_13TeV-pythia8'   ,  outDir + '/PROC_QCD_Pt-15To20_MuEnrichedPt5.root'   ,  outDir + '/PROC_QCD_Pt-15To20_MuEnrichedPt5.out'], #18
        # [mc17 + '/QCD_Pt-20To30_MuEnrichedPt5_TuneCP5_13TeV-pythia8'   ,  outDir + '/PROC_QCD_Pt-20To30_MuEnrichedPt5.root'   ,  outDir + '/PROC_QCD_Pt-20To30_MuEnrichedPt5.out'], #87
        # [mc17 + '/QCD_Pt-30To50_MuEnrichedPt5_TuneCP5_13TeV-pythia8'   ,  outDir + '/PROC_QCD_Pt-30To50_MuEnrichedPt5.root'   ,  outDir + '/PROC_QCD_Pt-30To50_MuEnrichedPt5.out'], #110
        # [mc17 + '/QCD_Pt-50To80_MuEnrichedPt5_TuneCP5_13TeV-pythia8'   ,  outDir + '/PROC_QCD_Pt-50To80_MuEnrichedPt5.root'   ,  outDir + '/PROC_QCD_Pt-50To80_MuEnrichedPt5.out'], #46
        # [mc17 + '/QCD_Pt-80To120_MuEnrichedPt5_TuneCP5_13TeV-pythia8'  ,  outDir + '/PROC_QCD_Pt-80To120_MuEnrichedPt5.root'  ,  outDir + '/PROC_QCD_Pt-80To120_MuEnrichedPt5.out'], #60
        # [mc17 + '/QCD_Pt-120To170_MuEnrichedPt5_TuneCP5_13TeV-pythia8' ,  outDir + '/PROC_QCD_Pt-120To170_MuEnrichedPt5.root' ,  outDir + '/PROC_QCD_Pt-120To170_MuEnrichedPt5.out'], #78
        # [mc17 + '/QCD_Pt-170To300_MuEnrichedPt5_TuneCP5_13TeV-pythia8' ,  outDir + '/PROC_QCD_Pt-170To300_MuEnrichedPt5.root' ,  outDir + '/PROC_QCD_Pt-170To300_MuEnrichedPt5.out'], #94
        # [mc17 + '/QCD_Pt-300To470_MuEnrichedPt5_TuneCP5_13TeV-pythia8' ,  outDir + '/PROC_QCD_Pt-300To470_MuEnrichedPt5.root' ,  outDir + '/PROC_QCD_Pt-300To470_MuEnrichedPt5.out'], #68
        # [mc17 + '/QCD_Pt-470To600_MuEnrichedPt5_TuneCP5_13TeV-pythia8' ,  outDir + '/PROC_QCD_Pt-470To600_MuEnrichedPt5.root' ,  outDir + '/PROC_QCD_Pt-470To600_MuEnrichedPt5.out'], #42
        # [mc17 + '/QCD_Pt-600To800_MuEnrichedPt5_TuneCP5_13TeV-pythia8' ,  outDir + '/PROC_QCD_Pt-600To800_MuEnrichedPt5.root' ,  outDir + '/PROC_QCD_Pt-600To800_MuEnrichedPt5.out'], #58
        # [mc17 + '/QCD_Pt-800To1000_MuEnrichedPt5_TuneCP5_13TeV-pythia8',  outDir + '/PROC_QCD_Pt-800To1000_MuEnrichedPt5.root',  outDir + '/PROC_QCD_Pt-800To1000_MuEnrichedPt5.out'], #101
        # [mc17 + '/QCD_Pt-1000_MuEnrichedPt5_TuneCP5_13TeV-pythia8'     ,  outDir + '/PROC_QCD_Pt-1000_MuEnrichedPt5.root'     ,  outDir + '/PROC_QCD_Pt-1000_MuEnrichedPt5.out'], #39


        # [data + '/Run2017B/SingleElectron/NANOAOD' ,  outDir + '/Run2017B_UL_SingleElectron.root' ,  outDir + '/Run2017B_UL_SingleElectron.out'], #32
        # [data + '/Run2017C/SingleElectron/NANOAOD' ,  outDir + '/Run2017C_UL_SingleElectron.root' ,  outDir + '/Run2017C_UL_SingleElectron.out'], #59
        # [data + '/Run2017D/SingleElectron/NANOAOD' ,  outDir + '/Run2017D_UL_SingleElectron.root' ,  outDir + '/Run2017D_UL_SingleElectron.out'], #37
        # [data + '/Run2017E/SingleElectron/NANOAOD' ,  outDir + '/Run2017E_UL_SingleElectron.root' ,  outDir + '/Run2017E_UL_SingleElectron.out'],
        # [data + '/Run2017F/SingleElectron/NANOAOD' ,  outDir + '/Run2017F_UL_SingleElectron.root' ,  outDir + '/Run2017F_UL_SingleElectron.out'], #66

    # [data + '/Run2017B/SingleMuon/NANOAOD/UL2017_MiniAODv2_NanoAODv9_GT36-v1' ,  outDir + '/Run2017B_UL_SingleMuon.root' ,  outDir + '/Run2017B_UL_SingleMuon.out'], #79
        # [data + '/Run2017C/SingleMuon/NANOAOD' ,  outDir + '/Run2017C_UL_SingleMuon.root' ,  outDir + '/Run2017C_UL_SingleMuon.out'], #117
        # [data + '/Run2017D/SingleMuon/NANOAOD' ,  outDir + '/Run2017D_UL_SingleMuon.root' ,  outDir + '/Run2017D_UL_SingleMuon.out'], #47
        # [data + '/Run2017E/SingleMuon/NANOAOD' ,  outDir + '/Run2017E_UL_SingleMuon.root' ,  outDir + '/Run2017E_UL_SingleMuon.out'],
        # [data + '/Run2017F/SingleMuon/NANOAOD' ,  outDir + '/Run2017F_UL_SingleMuon.root' ,  outDir + '/Run2017F_UL_SingleMuon.out'], #115

]"""
