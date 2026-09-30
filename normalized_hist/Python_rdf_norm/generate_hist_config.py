#!/usr/bin/env python3
"""
generate_hist_config.py

Single source of truth for hist_config_with_labels.json. Replaces the old
two-step workflow (hand-typed hist_config.json -> add_titles.py) with one
script: edit the tables below, run this, get the final config that
create_hist_rdf.py reads. Nothing downstream (create_hist_rdf.py,
runjob_hist_py.sh) needs to change.

Run:  python3 generate_hist_config.py [output_path]
      (defaults to hist_config_with_labels.json in the current directory)
"""

import json
import sys

# ---------------------------------------------------------------------
# Regions: prefix used on every branch name (must match BaseAnalyser's
# defineRegionObjectBranches() outPrefix argument, and the mass-branch
# Define() names, EXACTLY -- see the note at the bottom of this file
# about renaming the mass branches to match this convention), and the
# display label used in histogram titles.
# ---------------------------------------------------------------------
# Set True to re-enable the per-channel (uuu/uue/eeu/eee) object branches
# once they're turned back on in BaseAnalyser.cpp. Currently disabled there
# intentionally, so kept here for reference rather than deleted.
ENABLE_CHANNEL_REGIONS = False

ACTIVE_REGIONS = [
    {"prefix": "BaseRegion",       "label": "Base Region"},
    {"prefix": "ThreeLRegion",     "label": "Three-Lepton Region"},
    {"prefix": "WZ_Region",        "label": "WZ Region"},
    {"prefix": "WZ_Region_m3l",    "label": "WZ Region (m3l > 100)"},
    {"prefix": "SignalRegion_tzq", "label": "tZq Signal Region"},
    {"prefix": "SignalRegion_ttz", "label": "ttZ Signal Region"},
    {"prefix": "X_gamma_Region",   "label": "X+#gamma* Control Region"},
    {"prefix": "NP_1_Region",      "label": "Non-Prompt Control Region (no OSSF)"},
    {"prefix": "NP_2_Region",      "label": "Non-Prompt Control Region (off-Z)"},
    {"prefix": "baseRegion_4L",    "label": "Four-Lepton Region"},
    {"prefix": "ZZ_Region",        "label": "ZZ Region"},
    {"prefix": "ttZ_Region",       "label": "ttZ Region (4L)"},
]

# Currently disabled in BaseAnalyser.cpp -- kept here, excluded unless
# ENABLE_CHANNEL_REGIONS is flipped on above.
CHANNEL_REGIONS = [
    {"prefix": "uuu_ThreeLRegion", "label": "Three-Lepton Region (#mu#mu#mu)"},
    {"prefix": "uue_ThreeLRegion", "label": "Three-Lepton Region (#mu#mue)"},
    {"prefix": "eeu_ThreeLRegion", "label": "Three-Lepton Region (ee#mu)"},
    {"prefix": "eee_ThreeLRegion", "label": "Three-Lepton Region (eee)"},
]

REGIONS = ACTIVE_REGIONS + (CHANNEL_REGIONS if ENABLE_CHANNEL_REGIONS else [])

# ---------------------------------------------------------------------
# Common fields: the fixed set defineRegionObjectBranches() produces for
# EVERY region, one entry per field. Must stay in sync with
# regionBranchSpecs() in utility.cpp -- field names here are exactly
# what's appended after "<prefix>_".
# ---------------------------------------------------------------------
COMMON_FIELDS = [
    {"field": "leadingLepton_pt",     "bins": 25, "xmin": 0,    "xmax": 250, "title": "Leading Lepton p_{T}",            "xlabel": "p_{T}^{lead lep} [GeV]"},
    {"field": "subleadingLepton_pt",  "bins": 20, "xmin": 0,    "xmax": 150, "title": "Subleading Lepton p_{T}",         "xlabel": "p_{T}^{sublead lep} [GeV]"},
    {"field": "trailingLepton_pt",    "bins": 20, "xmin": 0,    "xmax": 100, "title": "Trailing Lepton p_{T}",           "xlabel": "p_{T}^{trail lep} [GeV]"},
    {"field": "leadingLepton_eta",    "bins": 30, "xmin": -3,   "xmax": 3,   "title": "Leading Lepton #eta",             "xlabel": "#eta^{lead lep}"},
    {"field": "subleadingLepton_eta", "bins": 30, "xmin": -3,   "xmax": 3,   "title": "Subleading Lepton #eta",          "xlabel": "#eta^{sublead lep}"},
    {"field": "trailingLepton_eta",   "bins": 30, "xmin": -3,   "xmax": 3,   "title": "Trailing Lepton #eta",            "xlabel": "#eta^{trail lep}"},

    {"field": "leadingJet_pt",        "bins": 20, "xmin": 0,    "xmax": 400, "title": "Leading Jet p_{T}",               "xlabel": "p_{T}^{lead jet} [GeV]"},
    {"field": "subleadingJet_pt",     "bins": 10, "xmin": 0,    "xmax": 200, "title": "Subleading Jet p_{T}",            "xlabel": "p_{T}^{sublead jet} [GeV]"},
    {"field": "leadingbJet_pt",       "bins": 20, "xmin": 0,    "xmax": 400, "title": "Leading b-Jet p_{T}",             "xlabel": "p_{T}^{lead b-jet} [GeV]"},
    {"field": "subleadingbJet_pt",    "bins": 10, "xmin": 0,    "xmax": 200, "title": "Subleading b-Jet p_{T}",          "xlabel": "p_{T}^{sublead b-jet} [GeV]"},
    {"field": "leadingJet_eta",       "bins": 30, "xmin": -5,   "xmax": 5,   "title": "Leading Jet #eta",                "xlabel": "#eta^{lead jet}"},
    {"field": "subleadingJet_eta",    "bins": 30, "xmin": -5,   "xmax": 5,   "title": "Subleading Jet #eta",             "xlabel": "#eta^{sublead jet}"},
    {"field": "leadingbJet_eta",      "bins": 30, "xmin": -5,   "xmax": 5,   "title": "Leading b-Jet #eta",              "xlabel": "#eta^{lead b-jet}"},
    {"field": "subleadingbJet_eta",   "bins": 30, "xmin": -5,   "xmax": 5,   "title": "Subleading b-Jet #eta",           "xlabel": "#eta^{sublead b-jet}"},
    {"field": "leadingJet_phi",       "bins": 32, "xmin": -3.2, "xmax": 3.2, "title": "Leading Jet #phi",                "xlabel": "#phi^{lead jet}"},
    {"field": "subleadingJet_phi",    "bins": 32, "xmin": -3.2, "xmax": 3.2, "title": "Subleading Jet #phi",             "xlabel": "#phi^{sublead jet}"},
    {"field": "leadingbJet_phi",      "bins": 32, "xmin": -3.2, "xmax": 3.2, "title": "Leading b-Jet #phi",              "xlabel": "#phi^{lead b-jet}"},
    {"field": "subleadingbJet_phi",   "bins": 32, "xmin": -3.2, "xmax": 3.2, "title": "Subleading b-Jet #phi",           "xlabel": "#phi^{sublead b-jet}"},

    {"field": "nJets",              "bins": 10, "xmin": 0, "xmax": 10, "title": "Jet Multiplicity",   "xlabel": "N_{jets}"},
    {"field": "nbJets",             "bins": 6,  "xmin": 0, "xmax": 6,  "title": "b-Jet Multiplicity", "xlabel": "N_{b-jets}"},
    {"field": "muon_multiplicity",  "bins": 6,  "xmin": 0, "xmax": 6,  "title": "Muon Multiplicity",  "xlabel": "N_{#mu}"},

    {"field": "goodMET_pt",   "bins": 20, "xmin": 0,    "xmax": 300, "title": "Missing Transverse Momentum",      "xlabel": "p_{T}^{miss} [GeV]"},
    {"field": "goodMET_phi",  "bins": 15, "xmin": -3.2, "xmax": 3.2, "title": "Missing Transverse Momentum #phi", "xlabel": "#phi^{miss}"},
    {"field": "PuppiMET_pt",  "bins": 20, "xmin": 0,    "xmax": 300, "title": "Missing Transverse Momentum",      "xlabel": "p_{T}^{miss} [GeV]"},
    {"field": "PuppiMET_phi", "bins": 15, "xmin": -3.2, "xmax": 3.2, "title": "Missing Transverse Momentum #phi", "xlabel": "#phi^{miss}"},
]

# ---------------------------------------------------------------------
# Extra/exceptional branches: not part of the common set, only defined
# (and only meaningful) in specific regions. Each entry is one literal
# branch: "<region>_<suffix>". These suffixes are copied EXACTLY from a
# TTree::Print() dump of the actual analysed tree -- they are shortened
# abbreviations (zMass, topMass, WbosonMT, mass, m3l, bestZmass, Z1mass,
# Z2mass, leftoverMass), not the full C++ variable names, and they are
# NOT uniform across regions (e.g. BaseRegion has no "_zMass" at all).
# Do not assume a new region follows this same suffix convention --
# check TTree::Print() before adding one.
# ---------------------------------------------------------------------
EXTRA_BRANCHES = [
    # ---- ThreeLRegion ----
    {"region": "ThreeLRegion", "suffix": "zMass",    "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Z Boson Mass", "xlabel": "m_{ll} [GeV]"},
    {"region": "ThreeLRegion", "suffix": "topMass",  "bins": 50, "xmin": 5,  "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "ThreeLRegion", "suffix": "WbosonMT", "bins": 30, "xmin": 0,  "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},

    # ---- WZ_Region ----
    {"region": "WZ_Region", "suffix": "zMass",    "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Z Boson Mass", "xlabel": "m_{ll} [GeV]"},
    {"region": "WZ_Region", "suffix": "topMass",  "bins": 50, "xmin": 5,  "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "WZ_Region", "suffix": "WbosonMT", "bins": 30, "xmin": 0,  "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},
    # ASSUMPTION: this branch is stored under the "WZ_Region" prefix but
    # is actually the Z mass for the m3l>100 variant (WZ_Region_m3l has
    # no "_zMass" branch of its own). Confirm/correct if wrong.
    {"region": "WZ_Region", "suffix": "zMass_3l", "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Z Boson Mass (m3l > 100 cut)", "xlabel": "m_{ll} [GeV]"},

    # ---- WZ_Region_m3l ----
    {"region": "WZ_Region_m3l", "suffix": "topMass",  "bins": 50, "xmin": 5, "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "WZ_Region_m3l", "suffix": "WbosonMT", "bins": 30, "xmin": 0, "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},

    # ---- BaseRegion (no zMass branch here) ----
    {"region": "BaseRegion", "suffix": "topMass",  "bins": 50, "xmin": 5, "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "BaseRegion", "suffix": "WbosonMT", "bins": 30, "xmin": 0, "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},

    # ---- SignalRegion_tzq ----
    {"region": "SignalRegion_tzq", "suffix": "zMass",    "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Z Boson Mass", "xlabel": "m_{ll} [GeV]"},
    {"region": "SignalRegion_tzq", "suffix": "topMass",  "bins": 50, "xmin": 5,  "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "SignalRegion_tzq", "suffix": "WbosonMT", "bins": 30, "xmin": 0,  "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},

    # ---- SignalRegion_ttz ----
    {"region": "SignalRegion_ttz", "suffix": "zMass",    "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Z Boson Mass", "xlabel": "m_{ll} [GeV]"},
    {"region": "SignalRegion_ttz", "suffix": "topMass",  "bins": 50, "xmin": 5,  "xmax": 700,
     "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
    {"region": "SignalRegion_ttz", "suffix": "WbosonMT", "bins": 30, "xmin": 0,  "xmax": 200,
     "title": "W Boson Transverse Mass", "xlabel": "M_{T}^{W} [GeV]"},

    # ---- off-Z control regions ----
    {"region": "X_gamma_Region", "suffix": "mass", "bins": 30, "xmin": 0, "xmax": 120,
     "title": "Non-Z OSSF Pair Mass", "xlabel": "m_{ll}^{non-Z} [GeV]"},
    {"region": "NP_1_Region", "suffix": "m3l", "bins": 40, "xmin": 0, "xmax": 500,
     "title": "Three-Lepton Invariant Mass", "xlabel": "m_{3l} [GeV]"},
    {"region": "NP_2_Region", "suffix": "mass", "bins": 30, "xmin": 0, "xmax": 120,
     "title": "Non-Z OSSF Pair Mass", "xlabel": "m_{ll}^{non-Z} [GeV]"},

    # ---- 4-lepton regions ----
    {"region": "baseRegion_4L", "suffix": "bestZmass", "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Leading Z Candidate Mass (Z1)", "xlabel": "m_{Z1} [GeV]"},
    {"region": "ZZ_Region", "suffix": "Z1mass", "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Leading Z Candidate Mass (Z1)", "xlabel": "m_{Z1} [GeV]"},
    {"region": "ZZ_Region", "suffix": "Z2mass", "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Subleading Z Candidate Mass (Z2)", "xlabel": "m_{Z2} [GeV]"},
    {"region": "ttZ_Region", "suffix": "bestZmass", "bins": 40, "xmin": 70, "xmax": 110,
     "title": "Leading Z Candidate Mass (Z1)", "xlabel": "m_{Z1} [GeV]"},
    {"region": "ttZ_Region", "suffix": "leftoverMass", "bins": 30, "xmin": 0, "xmax": 300,
     "title": "Leftover Lepton Pair Mass", "xlabel": "m_{ll}^{leftover} [GeV]"},
]

# ---------------------------------------------------------------------
# Ungated global branches: computed once, not gated by any region flag.
# Kept for quick sanity checks against raw (pre-region-cut)
# distributions -- these existed unprefixed in the old hand-written
# config.
# ---------------------------------------------------------------------
GLOBAL_BRANCHES = [
    {"field": "leadingLepton_pt",    "bins": 25, "xmin": 0, "xmax": 250, "title": "Leading Lepton p_{T}",         "xlabel": "p_{T}^{lead lep} [GeV]"},
    {"field": "subleadingLepton_pt", "bins": 20, "xmin": 0, "xmax": 150, "title": "Subleading Lepton p_{T}",      "xlabel": "p_{T}^{sublead lep} [GeV]"},
    {"field": "trailingLepton_pt",   "bins": 20, "xmin": 0, "xmax": 100, "title": "Trailing Lepton p_{T}",        "xlabel": "p_{T}^{trail lep} [GeV]"},
    {"field": "top_mass",            "bins": 50, "xmin": 5, "xmax": 700, "title": "Reconstructed Top Quark Mass", "xlabel": "m_{t} [GeV]"},
]

# ---------------------------------------------------------------------
# Special-case / literal entries that don't fit the region x field
# pattern.
# ---------------------------------------------------------------------
SPECIAL_ENTRIES = {
    "BaseRegion": {
        "bins": 1, "xmin": 0.5, "xmax": 1.5,
        "title": "Base Region: Event Yield", "xlabel": "Event Category",
    },
}

HIST2D_ENTRIES = {
    "ThreeLRegion_Region_leadJet_eta_phi": {
        "xbranch": "ThreeLRegion_leadingJet_eta",
        "ybranch": "ThreeLRegion_leadingJet_phi",
        "xbins": 104, "xmin": -5.2, "xmax": 5.2,
        "ybins": 64,  "ymin": -3.2, "ymax": 3.2,
        "title": "Three Lepton Region: Leading Jet #eta vs #phi",
        "xlabel": "#eta", "ylabel": "#phi",
    },
}


def build_config():
    config = {}
    region_by_prefix = {r["prefix"]: r for r in REGIONS}

    # region x common-field cross product
    for region in REGIONS:
        for f in COMMON_FIELDS:
            key = f"{region['prefix']}_{f['field']}"
            config[key] = {
                "bins": f["bins"], "xmin": f["xmin"], "xmax": f["xmax"],
                "title": f"{region['label']}: {f['title']}",
                "xlabel": f["xlabel"],
            }

    # exceptional branches: one literal "<region>_<suffix>" branch each
    for extra in EXTRA_BRANCHES:
        region = region_by_prefix.get(extra["region"])
        if region is None:
            print(f"Warning: extra branch suffix '{extra['suffix']}' lists "
                  f"unknown region prefix '{extra['region']}', skipping "
                  f"(region disabled via ENABLE_CHANNEL_REGIONS, or a typo).")
            continue
        key = f"{extra['region']}_{extra['suffix']}"
        config[key] = {
            "bins": extra["bins"], "xmin": extra["xmin"], "xmax": extra["xmax"],
            "title": f"{region['label']}: {extra['title']}",
            "xlabel": extra["xlabel"],
        }

    # ungated global branches, unprefixed
    for g in GLOBAL_BRANCHES:
        config[g["field"]] = {
            "bins": g["bins"], "xmin": g["xmin"], "xmax": g["xmax"],
            "title": g["title"], "xlabel": g["xlabel"],
        }

    # special-case literal entries
    for key, entry in SPECIAL_ENTRIES.items():
        config[key] = entry

    # 2D histograms section
    config["hist2d"] = HIST2D_ENTRIES

    return config


def main():
    outfile = sys.argv[1] if len(sys.argv) > 1 else "hist_config_with_labels.json"
    config = build_config()
    with open(outfile, "w") as f:
        json.dump(config, f, indent=2)
    n_1d = len([k for k in config if k != "hist2d"])
    n_2d = len(config.get("hist2d", {}))
    print(f"Wrote {n_1d} 1D entries and {n_2d} 2D entries to {outfile}")


if __name__ == "__main__":
    main()
