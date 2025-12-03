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
        'year': 2022,

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
        'split': 10,

        ###### if False, one output file per input file, if True then one output file for everything ######
        #'allinone': False,
        'allinone': True,

        ###### if True then skip existing analyzed files ######
        'skipold': True,

        ###### travel through the subdirectories and their subdirecties when processing. ######
        ###### becareful not to mix MC and real DATA in them. ######
        'recursive': True,

        ###### if False then only selected branches which is done in the .cpp file will be saved ######
        'saveallbranches': False,
        
        ###### How many input files? ######
        #'nrootfiles': 'All',set the bellow to 65 while running for the data and 15 for the simulation 
        'nrootfiles': 65,

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





nanoaod_inputdir_outputdir_pairs = [
    ['/TZQB-Zto2L-4FS_MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/Run3Summer22NanoAODv12-130X_mcRun3_2022_realistic_v5-v2/NANOAODSIM',
     'TOP_tzq_2022_old.root',
     'TOP_tzq_2022_old.out'],

]
"""
nanoaod_inputdir_outputdir_pairs = [
    ['/store/user/msahoo/2017/Era_b',
     'Era_b_all.root',
     'Era_b_all.out']

]

"""


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

