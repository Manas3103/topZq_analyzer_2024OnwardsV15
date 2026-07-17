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
        'btvfname': 'data/BTV/2024_Summer24/btagging.json',
        'btvtype' : 'UParTAK4_comb',
        'fname_btagEff' : 'BTag/btag_efficiency_2024_UParT.root',
        'hname_Loose_btagEff_bcflav' : 'hist_Loose_btagEff_bcflav',
        'hname_Loose_btagEff_lflav' : 'hist_Loose_btagEff_lflav',
        'hname_Medium_btagEff_bcflav' : 'hist_Medium_btagEff_bcflav',
        'hname_Medium_btagEff_lflav' : 'hist_Medium_btagEff_lflav',
        'hname_Tight_btagEff_bcflav' : 'hist_Tight_btagEff_bcflav',
        'hname_Tight_btagEff_lflav' : 'hist_Tight_btagEff_lflav',
    
        # Muon Correction 
        'muon_roch_fname': 'data/MUO/2024_Summer24/muon_scalesmearing.json', 
        'muon_fname': 'data/MUO/2024_Summer24/muon_Z.json', 
        'muonHLTtype': 'NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight', # not using this 
        'muonRECOtype': 'NUM_TrackerMuons_DEN_genTracks', # not using this 
        'muonIDtype': 'NUM_TightID_DEN_TrackerMuons', # dont not use this 
        'muonISOtype': 'NUM_TightPFIso_DEN_TightID',

        # Electron Correction 
        'electron_fname': 'data/EGM/2024_Summer24/electron.json',
        'electronHlt_fname':'data/EGM/2024_Summer24/electronHlt.json',
        'electronHlt_type':'HLT_SF_Ele30_TightID', #not using this 
        'electron_reco_type1': 'RecoAbove75',
        'electron_reco_type2' :  'Reco20to75',
        'electron_reco_type3' :  'RecoBelow20', #not using this 
        'electron_id_type': 'wp90iso',

        # json file name for JERC
        'jercfname': 'data/JERC/2024_Summer24/jet_jerc.json',

        # conbined correction type for jetsi
        'jerctag': 'Summer24Prompt24_V3_DATA_L1L2L3Res_AK4PFPuppi', #this is for Data
        'jettagMC' :'Summer24Prompt24_V3_MC_L1L2L3Res_AK4PFPuppi', #this is for MC

        # jet uncertainty 
        'jercunctag': ['Summer24Prompt24_V3_MC_Total_AK4PFPuppi'], 
       
        # for Jet veto map
        'jet_veto_f_name': 'data/JERC/2024_Summer24/jetvetomaps.json',
        'jet_veto_tag': 'Summer24Prompt24_RunBCDEFGHI_V1',

        'jetidfname' : 'data/JERC/2024_Summer24/jetid.json',
        'jetid_workingpoint' : 'AK4PUPPI_TightLeptonVeto',
        'JER_tag' : 'Summer24Prompt24_JRV1_MC_ScaleFactor_AK4PFPuppi',
	    'JER_tag_res' : 'Summer24Prompt24_JRV1_MC_PtResolution_AK4PFPuppi',
        
        'electron_SSF' :'data/EGM/2024_Summer24/electronSS_EtDependent.json',

        # MET correction 
        'metpt_fname' : 'data/JERC/2023_Summer23BPix/met_xyCorrections_2023_2023BPix.json'

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
        'nrootfiles': 5000,
        # 'nrootfiles': 3,
 
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
    ['/TZQB-Zto2L-4FS_Bin-MLL-30_TuneCP5_13p6TeV_amcatnlo-pythia8/RunIII2024Summer24NanoAODv15-Madgraph_2_6_5_150X_mcRun3_2024_realistic_v2-v2/NANOAODSIM',
     'tzq_3l.root',
     'tzq_3l.out']

]


