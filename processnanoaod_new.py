#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Modified version of the NanoAOD processor with added XRootD support and extended corrections
Original author: Suyong Choi (Department of Physics, Korea University suyong@korea.ac.kr)

>>> ADDED: JSON-batch input mode <<<
This version can also accept the JSON produced by get_das_files_batched.py
(one entry per dataset, each with a list of pre-split file batches). When
`indir` is a path ending in `.json`, the processor loops over every dataset
and every batch inside it and writes ONE output ROOT file per batch/job,
named `<outdir>/<dataset_key>_batch<N>_analyzed.root`.

>>> ADDED: --dataset/--batch filter <<<
For HTCondor use, pass --dataset NAME --batch N so a single job processes
only that one batch out of the JSON, instead of looping over everything.
"""
import os
import re
import json
import subprocess
import sys
from multiprocessing import Process
import cppyy
import ROOT

def is_filelist(path):
    """
    Check if the input path is a text file (list of files) or a ROOT file/directory.
    Returns True if path looks like a text file containing a file list.
    """
    return os.path.isfile(path) and path.endswith(".txt")


# >>> ADDED: recognize the batched-JSON input
def is_json_batchfile(path):
    """
    Check if the input path is the JSON produced by get_das_files_batched.py
    (or anything with the same {"datasets": {...}} shape).
    """
    return os.path.isfile(path) and path.endswith(".json")
# <<< END ADDED


def get_root_file_paths(indir, xrootd_prefix="root://cmsxrootd.fnal.gov/"):
    """
    Function to retrieve ROOT file paths using dasgoclient.
    """
    dataset_query = f"file dataset={indir}"
    command = f"dasgoclient --query='{dataset_query}'"
    try:
        output = subprocess.check_output(command, shell=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running dasgoclient: {e.output}")
        return []
   
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


# >>> ADDED: enumerate (job_name, files) jobs out of the batched JSON
def get_jobs_from_json(jsonpath, only_status=("ok", "partial")):
    """
    Parse the batched-JSON file (as produced by get_das_files_batched.py) and
    return a flat list of (job_name, files) tuples, one per batch, e.g.:

        [("top_zq_batch0", [...20 files...]),
         ("top_zq_batch1", [...20 files...]),
         ("data_2024_batch0", [...20 files...]), ...]

    Datasets whose "status" is not in `only_status` (e.g. "failed" or
    "no_files") are skipped with a warning.
    """
    with open(jsonpath) as fh:
        data = json.load(fh)

    jobs = []
    datasets = data.get("datasets", {})
    for key, dset in datasets.items():
        status = dset.get("status")
        if status not in only_status:
            print(f"  SKIP dataset '{key}' (status={status})")
            continue
        for batch in dset.get("batches", []):
            job_name = f"{key}_batch{batch['batch_index']}"
            files = batch.get("files", [])
            if not files:
                print(f"  SKIP {job_name} (no files in batch)")
                continue
            jobs.append((job_name, files))
    return jobs
# <<< END ADDED


def function_calling_PostProcessor(outdir, rootfileshere, jobconfmod):
    for afile in rootfileshere:
        rootfname = re.split('\/', afile)[-1]
        withoutext = re.split('\.root', rootfname)[0]
        outfname = outdir + '/' + withoutext + '_analyzed.root'
        subprocess.run(["./processonefile.py", afile, outfname, jobconfmod])
    pass

class Nanoaodprocessor:
    def __init__(self, indir, outdir, jobconfmod, procflags, config):
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
        self.skipcorrections = procflags.get('skipcorrections', False)  # Added skipcorrections flag
        print("year=", self.year)

        # Check if input is a DAS path or local directory
        self.is_das_path = is_valid_das_path(self.indir)
        self.is_eos_path = self.indir.startswith("/store/user/msahoo/")
        if not (self.is_das_path or self.is_eos_path) and not os.path.exists(self.indir):
            print(f'Path {indir} is neither a valid DAS path nor an existing local directory')
            exit(1)

    def process(self):
        self._processROOTfiles(self.indir, self.outdir)
        pass

    def _processROOTfiles(self, inputdirectory, outputdirectory):
        if not os.path.exists(outputdirectory):
            os.makedirs(outputdirectory)

        rootfileshere = []
        if self.is_das_path:
            # Handle remote files using XRootD
            rootfileshere = get_root_file_paths(inputdirectory)
            if self.nrootfiles > 0:
                rootfileshere = rootfileshere[:self.nrootfiles]
        else:
            # Original local file handling
            flist = os.listdir(inputdirectory)
            counter = 0
            for fname in flist:
                fullname = os.path.join(inputdirectory, fname)
                if re.match('.*\.root', fname) and os.path.isfile(fullname):
                    counter += 1
                    if counter <= self.nrootfiles and self.nrootfiles != 0:
                        rootfileshere.append(fullname)
                    elif self.nrootfiles == 0:
                        rootfileshere.append(fullname)

        print(f"Files found in {'DAS dataset' if self.is_das_path else 'directory'} {inputdirectory}")
        print(rootfileshere)

        if len(rootfileshere) > 0:
            if self.skipold:
                oflist = os.listdir(outputdirectory)
                filteredoflist = []
                for fname in oflist:
                    fullname = os.path.join(outputdirectory, fname)
                    if re.match('.*\.root', fname) and os.path.isfile(fullname):
                        withoutext = re.split("\.root", fname)[0]
                        wihoutskimtext = re.split("\_analyzed", withoutext)[0]
                        filteredoflist.append(wihoutskimtext)

                filterediflist = []
                for ifname in rootfileshere:
                    rootfname = re.split('\/', ifname)[-1]
                    withoutext = re.split('\.root', rootfname)[0]
                    if withoutext not in filteredoflist:
                        print(f'{withoutext} not yet in output dir')
                        filterediflist.append(ifname)
                    else:
                        print(f'{withoutext} in output dir')

                rootfileshere = filterediflist

            if self.split > 1:
                njobs = min(self.split, len(rootfileshere))
                nfileperjob = len(rootfileshere) / njobs

                ap = []
                for i in range(njobs):
                    start_idx = int(i * nfileperjob)
                    end_idx = int((i + 1) * nfileperjob) if i < njobs - 1 else None
                    filesforjob = rootfileshere[start_idx:end_idx]
                    p = Process(target=function_calling_PostProcessor,
                              args=(outputdirectory, filesforjob, self.jobconfmod))
                    p.start()
                    ap.append(p)
                for proc in ap:
                    proc.join()
            else:
                for afile in rootfileshere:
                    rootfname = re.split('\/', afile)[-1]
                    withoutext = re.split('\.root', rootfname)[0]
                    outfname = outputdirectory + '/' + withoutext + '_analyzed.root'
                    subprocess.run(["./processonefile.py", afile, outfname, self.jobconfmod])


# >>> ADDED: shared "files-in -> analyzed ROOT file-out" core, factored out of
# Nanoaodprocessor_singledir so it can be reused for a single directory/DAS
# path AND for one batch out of the JSON job list.
def _run_baseanalyser_on_files(rootfilestoprocess, outputroot, procflags, config):
    if not re.match('.*\.root', outputroot):
        print("Output file should be a root file! Quitting")
        exit(-1)

    print("FILES to PROCESS")
    print(rootfilestoprocess)

    intreename = config['intreename']
    outtreename = config['outtreename']
    saveallbranches = procflags['saveallbranches']

    t = ROOT.TChain(intreename)
    for afile in rootfilestoprocess:
        t.Add(afile)
    nevents = t.GetEntries()
    print("-------------------------------------------------------------------")
    print("Total Number of Entries:", nevents)
    print("-------------------------------------------------------------------")

    aproc = ROOT.BaseAnalyser(t, outputroot)

    try:
        aproc.setParams(config['year'], config['runtype'], config['datatype'])
    except Exception as e:
        print(f"Error calling setParams(): {e}")
        print(f"config['year']: {config['year']}")
        print(f"config['runtype']: {config['runtype']}")
        print(f"config['datatype']: {config['datatype']}")
        raise

    # Handle corrections with expanded configuration
    skipcorrections = procflags.get('skipcorrections', False)
    if not skipcorrections:
        print("Applying corrections...")
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
            config['muonHLTtype'],
            config['muonIDtype'],
            config['muonISOtype'],
            config['electron_fname'],
            config['electronHlt_fname'],
            config['electronHlt_type'],
            config['electron_reco_type1'],
            config['electron_reco_type2'],
            config['electron_id_type'],
            config['jercfname'],
            config['jerctag'],
            config['jettagMC'],
            config['jercunctag'],
            config['jet_veto_f_name'],
            config['jet_veto_tag'],
            config['electron_SSF'],
            config['metpt_fname'],
            config['jetidfname'],
            config['jetid_workingpoint'],
            config['JER_tag'],
            config['JER_tag_res']
        )
    else:
        print("Skipping corrections step")

    print("Starting setupanalysis")
    sys.stdout.flush()
    aproc.setupObjects()
    aproc.setupAnalysis()
    aproc.run(saveallbranches, outtreename)
# <<< END ADDED


def Nanoaodprocessor_singledir(indir, outputroot, procflags, config):
    """
    Runs nanoaod analyzer over ROOT files in indir and outputs into a single ROOT file.
    Now supports both local and remote files via XRootD.
    """
    rootfilestoprocess = []
    is_das_path = is_valid_das_path(indir)

    if is_filelist(indir):
        print(f"READ root files from batch list:\n{indir}\n")
        with open(indir) as f:
           rootfilestoprocess = [l.strip() for l in f if l.strip() and not l.startswith("#")]
    elif is_das_path:
        print(f"COLLECT root files from DAS dataset:\n{indir}\n")
        rootfilestoprocess = get_root_file_paths(indir)

   # >>> ADDED: Handle remote EOS path with XRootD
    elif indir.startswith("/store/user/msahoo/"):
        print(f"COLLECT root files from remote EOS path:\n{indir}\n")
        #xrootd_prefix = "root://cmsxrootd.fnal.gov/"
        xrootd_prefix = "root://cmseos.fnal.gov/"
        eos_full_path = xrootd_prefix + indir

        eos_ls_cmd = f'xrdfs cmseos.fnal.gov ls -R {indir}'
        eos_files = os.popen(eos_ls_cmd).read().splitlines()

        counter = 0
        for fname in eos_files:
            if fname.endswith(".root"):
                counter += 1
                fullpath = xrootd_prefix + fname
                if counter <= procflags['nrootfiles'] and procflags['nrootfiles'] != 0:
                    rootfilestoprocess.append(fullpath)
                elif procflags['nrootfiles'] == 0:
                    rootfilestoprocess.append(fullpath)
    # <<< END ADDED

    else:
        print(f"COLLECT root files in:\n{indir}\n")
        fullnamelist = []
        if not procflags['recursive']:
            flist = os.listdir(indir)
            for fname in flist:
                fullname = os.path.join(indir, fname)
                fullnamelist.append(fullname)
        else:
            for root, dirs, flist in os.walk(indir):
                for fname in flist:
                    fullname = os.path.join(root, fname)
                    fullnamelist.append(fullname)

        counter = 0
        for fname in fullnamelist:
            if re.match('.*\.root', fname) and os.path.isfile(fname):
                counter += 1
                if counter <= procflags['nrootfiles'] and procflags['nrootfiles'] != 0:
                    rootfilestoprocess.append(fname)
                elif procflags['nrootfiles'] == 0:
                    rootfilestoprocess.append(fname)

    if procflags['nrootfiles'] > 0:
        rootfilestoprocess = rootfilestoprocess[:procflags['nrootfiles']]

    # NOTE: core chain+analyse+write logic now lives in _run_baseanalyser_on_files
    _run_baseanalyser_on_files(rootfilestoprocess, outputroot, procflags, config)


# >>> ADDED: new entry point driven entirely by the batched JSON.
# One output ROOT file is written per batch/job:
#   <outdir>/<dataset_key>_batch<N>_analyzed.root
#
# >>> ADDED: only_dataset / only_batch filter <<<
# When only_dataset is given, restrict to that dataset's batches. When
# only_batch is ALSO given, restrict further to that single batch_index,
# so exactly one job (one condor job = one batch) runs per call.
def Nanoaodprocessor_fromjson(jsonfile, outdir, procflags, config,
                               only_dataset=None, only_batch=None):
    """
    Loop over dataset/batch entries described in the JSON produced by
    get_das_files_batched.py and run the BaseAnalyser once per batch,
    writing one output ROOT file per job into outdir.

    only_dataset / only_batch: if provided, restrict to that single
    (dataset, batch_index) job instead of running every batch of every
    dataset in the JSON. This is what lets one HTCondor job process
    exactly one batch while still passing the JSON file directly.

    Honors procflags['nrootfiles'] as a per-job cap on the number of files
    used (0 = use every file in the batch), and procflags['split'] to run
    jobs in parallel with multiprocessing when NOT restricted to a single
    batch.
    """
    if not os.path.exists(outdir):
        os.makedirs(outdir)

    print(f"COLLECT batches from JSON:\n{jsonfile}\n")
    jobs = get_jobs_from_json(jsonfile)

    # >>> ADDED: apply the dataset/batch filter
    if only_dataset is not None:
        if only_batch is not None:
            target_name = f"{only_dataset}_batch{only_batch}"
            jobs = [(name, files) for name, files in jobs if name == target_name]
            if not jobs:
                print(f"ERROR: no job named '{target_name}' found in {jsonfile} "
                      f"(check dataset name / batch_index / dataset status)")
                exit(1)
        else:
            prefix = f"{only_dataset}_batch"
            jobs = [(name, files) for name, files in jobs if name.startswith(prefix)]
            if not jobs:
                print(f"ERROR: no batches found for dataset '{only_dataset}' in {jsonfile}")
                exit(1)
    # <<< END ADDED

    if procflags.get('nrootfiles', 0) > 0:
        n = procflags['nrootfiles']
        jobs = [(name, files[:n]) for name, files in jobs]

    if not jobs:
        print("No batches found in JSON (or all datasets were skipped) -- nothing to do.")
        return

    def _run_one_job(job_name, files):
        outfname = os.path.join(outdir, f"{job_name}_analyzed.root")
        if procflags.get('skipold', False) and os.path.isfile(outfname):
            print(f"{job_name}: output already exists, skipping (skipold=True)")
            return
        print(f"=== Running job {job_name}  ({len(files)} file(s)) -> {outfname} ===")
        _run_baseanalyser_on_files(files, outfname, procflags, config)

    split = procflags.get('split', 1)
    # Only worth multiprocessing when we're running more than one job in
    # this call, i.e. when NOT restricted to a single (dataset, batch).
    if split and split > 1 and len(jobs) > 1:
        running = []
        for job_name, files in jobs:
            p = Process(target=_run_one_job, args=(job_name, files))
            p.start()
            running.append(p)
            if len(running) >= split:
                for p in running:
                    p.join()
                running = []
        for p in running:
            p.join()
    else:
        for job_name, files in jobs:
            _run_one_job(job_name, files)
# <<< END ADDED


if __name__ == '__main__':
    from importlib import import_module
    from argparse import ArgumentParser

    parser = ArgumentParser(usage="%(prog)s inputDir outputDir jobconfmod [--dataset NAME --batch N]")
    parser.add_argument("indir", help="DAS path, local dir, .txt filelist, or "
                                       ".json batch file (from get_das_files_batched.py)")
    parser.add_argument("outdir")
    parser.add_argument("jobconfmod")
    # >>> ADDED: optional single-batch filter, only meaningful with a .json indir
    parser.add_argument("--dataset", default=None,
                         help="Only used with a .json indir: restrict to this dataset's batches")
    parser.add_argument("--batch", type=int, default=None,
                         help="Only used with a .json indir + --dataset: restrict to this single batch_index "
                              "(one HTCondor job = one batch)")
    # <<< END ADDED
    args = parser.parse_args()

    # Load compiled C++ libraries
    cppyy.load_reflection_info("libcorrectionlib.so")
    cppyy.load_reflection_info("libMathMore.so")
    cppyy.load_reflection_info("libnanoadrdframe.so")

    # Read configurations
    mod = import_module(args.jobconfmod)
    procflags = getattr(mod, 'procflags')
    config = getattr(mod, 'config')

    # >>> ADDED: route .json inputs to the per-batch processor, one output
    # file per job, regardless of the allinone flag. Pass through the
    # optional --dataset/--batch filter for single-batch condor jobs.
    if is_json_batchfile(args.indir):
        print("json-batch mode")
        Nanoaodprocessor_fromjson(args.indir, args.outdir, procflags, config,
                                   only_dataset=args.dataset, only_batch=args.batch)
    # <<< END ADDED
    elif not procflags['allinone']:
        print("not allinone")
        n = Nanoaodprocessor(args.indir, args.outdir, args.jobconfmod, procflags, config)
        n.process()
    else:
        print("allinone")
        Nanoaodprocessor_singledir(args.indir, args.outdir, procflags, config)
