#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import subprocess
import sys
from multiprocessing import Process
from importlib import import_module
from argparse import ArgumentParser
import ROOT
import cppyy

#from XRootD import client

def get_root_file_paths(indir, xrootd_prefix="root://cmsxrootd.fnal.gov/"):
    """
    Function to retrieve ROOT file paths using dasgoclient.
    """
    # Run dasgoclient command to get the list of files
    dataset_query = f"file dataset={indir}"
    command = f"dasgoclient --query='{dataset_query}'"
    try:
        output = subprocess.check_output(command, shell=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running dasgoclient: {e.output}")
        return []

    # Process the output and prepend the xrootd prefix
    files = output.strip().split('\n')
    root_files = [xrootd_prefix + f for f in files]

    return root_files

def is_valid_das_path(indir):
    """
    Check if the indir is a valid DAS path by querying dasgoclient.
    """
    command = f"dasgoclient --query='dataset={indir}'"
    try:
        output = subprocess.check_output(command, shell=True, text=True)
        return bool(output.strip())
    except subprocess.CalledProcessError:
        return False

def function_calling_PostProcessor(outdir, rootfileshere, jobconfmod, samplename):
    for afile in rootfileshere:
        rootfname = re.split('\/', afile)[-1]
        withoutext = re.split('\.root', rootfname)[0]
        outfname = outdir + '/' + withoutext + '_analyzed.root'
        subprocess.run(["./processonefile.py", afile, outfname, jobconfmod, samplename])



class Nanoaodprocessor:
    def __init__(self, indir, outdir, jobconfmod, procflags, config, samplename):
        self.outdir = outdir
        self.indir = indir
        self.jobconfmod = jobconfmod
        self.split = procflags['split']
        self.skipold = procflags['skipold']
        self.recursive = procflags['recursive']
        self.saveallbranches = procflags['saveallbranches']
        self.nrootfiles = procflags['nrootfiles']
        self.year = config['year']
        self.runtype = config['runtype']
        self.datatype = config['datatype']
        self.samplename = samplename  # Initialize samplename here

       
        # Get the list of ROOT files based on the input directory or DAS path
        if is_valid_das_path(indir):
            self.rootfiles = get_root_file_paths(indir)
        else:
            self.rootfiles = self._get_local_root_files(indir)

        if not self.rootfiles:
            print(f"No ROOT files found in {indir}")
            exit(1)

        print(f"Total ROOT files: {len(self.rootfiles)}")

        # Print the list of ROOT files
        print("List of ROOT files:")
        for rootfile in self.rootfiles:
            print(rootfile)

    def _get_local_root_files(self, inputdirectory):
        if not os.path.exists(inputdirectory):
            print(f"Directory {inputdirectory} does not exist.")
            return []

        flist = os.listdir(inputdirectory)
        rootfiles = []
        for fname in flist:
            fullname = os.path.join(inputdirectory, fname)
            if re.match('.*\.root', fname) and os.path.isfile(fullname):
                rootfiles.append(fullname)
        return rootfiles

    def process(self):
        self._processROOTfiles(self.rootfiles, self.outdir)

    def _processROOTfiles(self, rootfiles, outputdirectory):
        # Create output directory if it doesn't exist
        if not os.path.exists(outputdirectory):
            os.mkdir(outputdirectory)

        if self.skipold:
            rootfiles = self._filter_existing_files(rootfiles, outputdirectory)

        if self.split == 'Max':
            njobs = len(rootfiles)
            nfileperjob = 1
        else:
            njobs = self.split
            nfileperjob = len(rootfiles) * 1.0 / njobs
            if len(rootfiles) < njobs:
                njobs = len(rootfiles)
                nfileperjob = 1

            print(f"Splitting {len(rootfiles)} files into {njobs} jobs.")

        ap = []
        for i in range(njobs):
            if i < njobs - 1:
                filesforjob = rootfiles[int(i * nfileperjob):int((i + 1) * nfileperjob)]
            else:
                filesforjob = rootfiles[int(i * nfileperjob):]

            print(f"Job {i+1} will process {len(filesforjob)} files.")
            p = Process(target=function_calling_PostProcessor, args=(outputdirectory, filesforjob, self.jobconfmod, self.samplename))
            p.start()
            ap.append(p)
        for proc in ap:
            proc.join()

    def _filter_existing_files(self, rootfiles, outputdirectory):
        oflist = os.listdir(outputdirectory)
        filteredoflist = []
        for fname in oflist:
            fullname = os.path.join(outputdirectory, fname)
            if re.match('.*\.root', fname) and os.path.isfile(fullname):
                withoutext = re.split("\.root", fname)[0]
                wihoutskimtext = re.split("\_analyzed", withoutext)[0]
                filteredoflist.append(wihoutskimtext)

        filterediflist = []
        for ifname in rootfiles:
            rootfname = re.split('\/', ifname)[-1]
            withoutext = re.split('\.root', rootfname)[0]
            if withoutext not in filteredoflist:
                filterediflist.append(ifname)
            else:
                print(f"{withoutext} already processed. Skipping.")
        return filterediflist

class Nanoaodprocessor_singledir:
    def __init__(self, indir, outputroot, procflags, config, samplename):
        self.outputroot = outputroot
        self.indir = indir
        self.procflags = procflags
        self.config = config
        self.samplename = samplename
        self.nrootfiles = procflags['nrootfiles'] 

        if is_valid_das_path(indir):
            self.rootfiles = get_root_file_paths(indir)
        else:
            self.rootfiles = self._get_root_files(indir)

        if not self.rootfiles:
            print(f"No ROOT files found in {indir}")
            exit(1)

        if procflags['nrootfiles'] != 'All':
            try:
                n_files_to_process = int(procflags['nrootfiles'])
                self.rootfiles = self.rootfiles[:n_files_to_process]
            except ValueError:
                print(f"Invalid value for nrootfiles: {procflags['nrootfiles']}. Using all files.")
            print(f"Processing {len(self.rootfiles)} files out of {len(self.rootfiles)} available.")


    def _get_local_root_files(self, inputdirectory):
        flist = os.listdir(inputdirectory)
        rootfiles = []
        for fname in flist:
            fullname = os.path.join(inputdirectory, fname)
            if re.match('.*\.root', fname) and os.path.isfile(fullname):
                rootfiles.append(fullname)
        return rootfiles

    def _get_eos_root_files(self, eos_directory):
        """Get a list of ROOT files from the specified EOS directory, with full paths."""
        cmd = f"xrdfs root://cmseos.fnal.gov/ ls {eos_directory}"
        print(f"Executing command: {cmd}")
        
        result = subprocess.run(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        if result.returncode != 0:
            raise RuntimeError(f"Error accessing EOS directory: {result.stderr.strip()}")
        
        flist = result.stdout.splitlines()
        # Prepend the EOS root prefix to the file paths
        rootfiles = [f"root://cmseos.fnal.gov/{fname}" for fname in flist if fname.endswith('.root')]
        
        if not rootfiles:
            print(f"No ROOT files found in EOS directory: {eos_directory}")
        
        return rootfiles

    def _get_root_files(self, directory):
        """Determine whether to retrieve ROOT files from a local or EOS directory."""
        if directory.startswith('/eos/uscms/') or directory.startswith('/store/user/'):
            print(f"Reading ROOT files from EOS directory: {directory}")
            return self._get_eos_root_files(directory)
        else:
            print(f"Reading ROOT files from local directory: {directory}")
            return self._get_local_root_files(directory)

    def process(self):
        self._processROOTfiles(self.rootfiles, self.outputroot)

    def _processROOTfiles(self, rootfiles, outputdir):
        # Handle job splitting
        if self.procflags['split'] == 'Max':
            njobs = len(rootfiles)
            nfileperjob = 1
        else:
            njobs = self.procflags['split']
            nfileperjob = len(rootfiles) * 1.0 / njobs
            if len(rootfiles) < njobs:
                njobs = len(rootfiles)
                nfileperjob = 1

        print(f"Splitting {len(rootfiles)} files into {njobs} jobs.")

        ap = []
        for i in range(njobs):
            if i < njobs - 1:
                filesforjob = rootfiles[int(i * nfileperjob):int((i + 1) * nfileperjob)]
            else:
                filesforjob = rootfiles[int(i * nfileperjob):]
            
            print(f"Job {i+1} will process {len(filesforjob)} files.")

            job_output_file = os.path.join(outputdir, f"{self.samplename}_job{i+1}_AnalysisNtuple.root")

            p = Process(target=self._process_files_in_job, args=(filesforjob, job_output_file))
            p.start()
            ap.append(p)
        
        for proc in ap:
            proc.join()

    def _process_files_in_job(self, filesforjob, outputroot):
        intreename = self.config['intreename']
        outtreename = self.config['outtreename']
        saveallbranches = self.procflags['saveallbranches']
        # ROOT.ROOT.EnableImplicitMT()

        t = ROOT.TChain(intreename)
        for afile in filesforjob:
            t.Add(afile)
        nevents = t.GetEntries()
        print("-------------------------------------------------------------------")
        print(f"Total Number of Entries: {nevents}")
        print("-------------------------------------------------------------------")

        # Pass the samplename to the TopSemiLeptAnalyzer
        aproc = ROOT.TopSemiLeptAnalyzer(t, outputroot, self.samplename,self.indir)
        aproc.setParams(self.config['year'], self.config['runtype'], self.config['datatype'])

        # Setup corrections and analysis as before
        aproc.setupCorrections(
            config['goodjson'],
            config['pileupfname'],
            config['pileuptag'],
            config['btvfname'],
            config['btvtype'],
            config['fname_btagEff'],
            config['hname_Loose_btagEff_bcflav'],
            config['hname_Loose_btagEff_lflav'],
            config['hname_Medium_btagEff_bcflav'],
            config['hname_Medium_btagEff_lflav'],
            config['hname_Tight_btagEff_bcflav'],
            config['hname_Tight_btagEff_lflav'],
            config['muon_roch_fname'],
            config['muon_fname'],
            config['muon_HLT_type'],
            config['muon_RECO_type'],
            config['muon_ID_type'],
            config['muon_ISO_type'],
            config['electron_fname'],
            config['electron_reco_type'],
            config['electron_id_type'],
            config['photon_fname'],
            config['photon_id_type'],
            config['jercfname'],
            # config['jerctag_MC'],
            config['jerctag_DATA_Run2017D'],
            config['jercunctag'],
            config['jerctag'],
            config['fname_metphimod']
            
    )


        sys.stdout.flush()  # to force printout in the right order

        print("Setting up objects...")
        aproc.setupObjects()

        print("Setting up analysis...")
        aproc.setupAnalysis()
        
        print("Running the analysis...")
        aproc.run(saveallbranches, outtreename)

        print(f"Processing complete for job {outputroot}.")

if __name__ == '__main__':

    from importlib import import_module
    from argparse import ArgumentParser
    
    # Setup argument parser
    # parser = ArgumentParser(usage="%prog [options] inputDir outputDir jobconfmod")
    parser = ArgumentParser(usage="%(prog)s indir outdir jobconfmod samplename")
    parser.add_argument("indir", help="Input directory or DAS dataset path")
    parser.add_argument("outdir", help="Output directory or output ROOT file")
    parser.add_argument("jobconfmod", help="Job configuration module (Python file)")
    parser.add_argument("samplename", help="Name of the sample being processed")  # New argument for sample name

    args = parser.parse_args()

    indir = args.indir
    outdir = args.outdir
    jobconfmod = args.jobconfmod
    samplename = args.samplename

    # Load compiled C++ library into ROOT/Python
    cppyy.load_reflection_info("libcorrectionlib.so")
    cppyy.load_reflection_info("libMathMore.so")
    cppyy.load_reflection_info("libnanoadrdframe.so")

    # Import the job configuration module
    mod = import_module(jobconfmod)
    procflags = getattr(mod, 'procflags')
    config = getattr(mod, 'config')

    if not procflags['allinone']:
        print("Processing with Nanoaodprocessor")
        n = Nanoaodprocessor(indir, outdir, jobconfmod, procflags, config,samplename)
        n.process()
    else:
        print("Processing with Nanoaodprocessor_singledir")
        Nanoaodprocessor_singledir(indir, outdir, procflags, config,samplename).process()


