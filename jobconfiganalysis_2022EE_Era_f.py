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
        'year': 2022,

        # is ReReco or Ultra Legacy
        'runtype': 'PostEE',

        'datatype': -1, # 0=MC ; 1=DATA ; -1=Auto


        #for correction
        
        # good json file
        'goodjson' : 'data/GoldenJSON/golden_json_latest_2022.json',

        # pileup weight for MC
        'pileupfname': 'data/LUM/2022_Summer22EE/puWeights.json',

        'pileuptag': 'Collisions2022_359022_362760_eraEFG_GoldenJson',

        # json filename for BTV correction
        'btvfname': 'data/BTV/2022_Summer22EE/btagging.json',

        # BTV correction type
        'btvtype': 'deepJet_shape',
        #'btvtype': 'deepJet_mujets',
    
        # Muon Correction 
        'muon_roch_fname': 'data/MUO/2022_Summer22EE/muon_scalesmearing.json', 
        'muon_fname': 'data/MUO/2022_Summer22EE/muon_Z.json', 
        'muonHLTtype': 'NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight',
        'muonRECOtype': 'NUM_TrackerMuons_DEN_genTracks',
        'muonIDtype': 'NUM_TightID_DEN_TrackerMuons',
        'muonISOtype': 'NUM_TightPFIso_DEN_TightID',

        # Electron Correction 
        'electron_fname': 'data/EGM/2022_Summer22EE/electron.json',
        'electronHlt_fname':'data/EGM/2022_Summer22EE/electronHlt.json',
        'electronHlt_type':'HLT_SF_Ele30_TightID',
        'electron_reco_type1': 'RecoAbove75',
        'electron_reco_type2' :  'Reco20to75',
        'electron_reco_type3' :  'RecoBelow20',
        'electron_id_type': 'wp90iso',

        # json file name for JERC
        'jercfname': 'data/JERC/2022_Summer22EE/jet_jerc.json',

        # conbined correction type for jetsi
        'jerctag': 'Summer22EE_22Sep2023_RunF_V2_DATA_L1L2L3Res_AK4PFPuppi', #this is for Data
        'jettagMC' : 'Summer22EE_22Sep2023_V2_MC_L1L2L3Res_AK4PFPuppi', #this is for MC

        # jet uncertainty 
        'jercunctag': 'Summer22EE_22Sep2023_V2_MC_Total_AK4PFPuppi', 
       
        # for Jet veto map
        'jet_veto_f_name': 'data/JERC/2022_Summer22EE/jetvetomaps.json',
        'jet_veto_tag': 'Summer22EE_23Sep2023_RunEFG_V1',

        
        'electron_SSF' :'data/EGM/2022_Summer22EE/electronSS.json',

        # MET correction 
        'metpt_fname' : 'data/JERC/2022_Summer22EE/met_xyCorrections_2022_2022EE.json'
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
    ['/store/user/msahoo/2022EE/Era_f/Muon',
     'Data_Muon_fromEOS_f.root',
     'Data_Muon_fromEOS_f.out']

]

