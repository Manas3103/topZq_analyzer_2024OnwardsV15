"""
File contains job options 
"""


# options for Nanoaodrdframe
config = {
        
        # tree name of input file(s)
        'intreename': "Events",

        # tree name of output file(s) it cannot be the same as the input tree name or it'll crash
        'outtreename': "outputTree",

        #data year (2017,2022,2023,2024)
        'year': 2024,

        # is ReReco or Ultra Legacy
        'runtype': '',

        'datatype': -1, # 0=MC ; 1=DATA ; -1=Auto


        #for correction
        
        # good json file
        'goodjson' : 'data/GoldenJSON/Cert_Collisions2024_378981_386951_Golden.json',

        # pileup weight for MC
        'pileupfname': 'data/LUM/2024/puWeights_BCDEFGHI.json',

        'pileuptag': 'Collisions24_BCDEFGHI_goldenJSON',

        # json filename for BTV correction
        'btvfname': 'data/BTV/2022_Summer22/btagging.json',

        # BTV correction type
        'btvtype': 'deepJet_shape',
        #'btvtype': 'deepJet_mujets',
    
        # Muon Correction 
        'muon_roch_fname': 'data/MUO/2024_Summer24/muon_scalesmearing.json', 
        'muon_fname': 'data/MUO/2024_Summer24/muon_Z.json', 
        'muonHLTtype': 'NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight',
        'muonRECOtype': 'NUM_TrackerMuons_DEN_genTracks',
        'muonIDtype': 'NUM_TightID_DEN_TrackerMuons',
        'muonISOtype': 'NUM_TightPFIso_DEN_TightID',

        # Electron Correction 
        'electron_fname': 'data/EGM/2024_Summer24/electron.json',
        'electronHlt_fname':'data/EGM/2024_Summer24/electronHlt.json',
        'electronHlt_type':'HLT_SF_Ele30_TightID',
        'electron_reco_type1': 'RecoAbove75',
        'electron_reco_type2' :  'Reco20to75',
        'electron_reco_type3' :  'RecoBelow20',
        'electron_id_type': 'PromptMVA-Tight',

        # json file name for JERC
        'jercfname': 'data/JERC/2024_Summer24/jet_jerc.json',

        # conbined correction type for jetsi
        'jerctag': 'Summer24Prompt24_V2_DATA_L1L2L3Res_AK4PFPuppi', #this is for Data
        'jettagMC' :'Summer24Prompt24_V2_MC_L1L2L3Res_AK4PFPuppi', #this is for MC

        # jet uncertainty 
        'jercunctag': 'Summer24Prompt24_V2_MC_Total_AK4PFPuppi', 
       
        # for Jet veto map
        'jet_veto_f_name': 'data/JERC/2024_Summer24/jetvetomaps.json',
        'jet_veto_tag': 'Summer24Prompt24_RunBCDEFGHI_V1',

        'jetidfname' : 'data/JERC/2024_Summer24/jetid.json',
        'jetid_workingpoint' : 'AK4PUPPI_TightLeptonVeto',
        'JER_tag' : 'Summer23BPixPrompt23_RunD_JRV1_MC_ScaleFactor_AK4PFPuppi',

        
        'electron_SSF' :'data/EGM/2024_Summer24/electronSS_EtDependent.json',

        # MET correction 
        'metpt_fname' : 'data/JERC/2022_Summer22/met_xyCorrections_2022_2022.json'
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
        'nrootfiles': 1000,

        ###### Make a copy of the analyzer in the same directory as the analyzed root files ######
        'copyInstance': False,
        }



'''
nanoaod_inputdir_outputdir_pairs = [
    ['/MuonEG/Run2022D-22Sep2023-v1/NANOAOD',
     'Data_MuonEG_d.root',
     'Data_MuonEG_d.out'],

    ['/TTWZ_TuneCP5_13p6TeV_madgraph-pythia8/Run3Summer22NanoAODv11-126X_mcRun3_2022_realistic_v2-v2/NANOAODSIM',
     'ttwz.root',
     'ttwz.out'],

    ['/EGamma/Run2022C-16Dec2023-v1/NANOAOD',
     'Data_EGamma_c.root',
     'Data_EGamma_c.out'],

    ['/Muon/Run2022C-16Dec2023-v1/NANOAOD',
     'Data_Muon_c.root',
     'Data_Muon_c.out'],
]



'''
nanoaod_inputdir_outputdir_pairs = [
    ['/store/user/msahoo/2024/wg_1jet',
     'wg_1jet.root',
     'wg_1jet.out']

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

