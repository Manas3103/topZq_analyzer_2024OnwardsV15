#!/usr/bin/env python3
"""
add_titles.py

Reads hist_config.json (branch -> {bins, xmin, xmax}) and writes a new
version where every entry also has "title" and "xlabel", derived from the
key name using a region-prefix + variable-name convention.

Run:  python3 add_titles.py hist_config.json hist_config_with_labels.json
"""

import json
import re
import sys

# --- known region prefixes, longest/most-specific first so matching is greedy ---
REGION_PREFIXES = [
    ("uuu_ThreeLRegion_", "Three-Lepton Region (#mu#mu#mu)"),
    ("uue_ThreeLRegion_", "Three-Lepton Region (#mu#mue)"),
    ("eeu_ThreeLRegion_", "Three-Lepton Region (ee#mu)"),
    ("eee_ThreeLRegion_", "Three-Lepton Region (eee)"),
    ("WZ_Region_",        "WZ Region"),
    ("ThreeLRegion_",     "Three-Lepton Region"),
    ("BaseRegion_",       "Base Region"),
]

# --- variable name -> (title fragment, xlabel) ---
VARIABLE_MAP = {
    "leadingLepton_pt":       ("Leading Lepton p_{T}",         "p_{T}^{lead lep} [GeV]"),
    "subleadingLepton_pt":    ("Subleading Lepton p_{T}",      "p_{T}^{sublead lep} [GeV]"),
    "trailingLepton_pt":      ("Trailing Lepton p_{T}",        "p_{T}^{trail lep} [GeV]"),
    "topLepton_pt":           ("Top Lepton p_{T}",             "p_{T}^{top lep} [GeV]"),
    "leadingLepton_eta":      ("Leading Lepton #eta",          "#eta^{lead lep}"),
    "subleadingLepton_eta":   ("Subleading Lepton #eta",       "#eta^{sublead lep}"),
    "trailingLepton_eta":     ("Trailing Lepton #eta",         "#eta^{trail lep}"),
    "leadingMuon_pt":         ("Leading Muon p_{T}",           "p_{T}^{lead #mu} [GeV]"),
    "subleadingMuon_pt":      ("Subleading Muon p_{T}",        "p_{T}^{sublead #mu} [GeV]"),
    "trailingMuon_pt":        ("Trailing Muon p_{T}",          "p_{T}^{trail #mu} [GeV]"),
    "leadingMuon_eta":        ("Leading Muon #eta",            "#eta^{lead #mu}"),
    "subleadingMuon_eta":     ("Subleading Muon #eta",         "#eta^{sublead #mu}"),
    "trailingMuon_eta":       ("Trailing Muon #eta",           "#eta^{trail #mu}"),
    "nJets":                  ("Jet Multiplicity",             "N_{jets}"),
    "nbJets":                 ("b-Jet Multiplicity",           "N_{b-jets}"),
    "muon_multiplicity":      ("Muon Multiplicity",            "N_{#mu}"),
    "leadingJet_pt":          ("Leading Jet p_{T}",            "p_{T}^{lead jet} [GeV]"),
    "leadingJet_eta":         ("Leading Jet #eta",             "#eta^{lead jet}"),
    "subleadingJet_pt":       ("Subleading Jet p_{T}",         "p_{T}^{sublead jet} [GeV]"),
    "subleadingJet_eta":      ("Subleading Jet #eta",          "#eta^{sublead jet}"),
    "leadingbJet_pt":         ("Leading b-Jet p_{T}",          "p_{T}^{lead b-jet} [GeV]"),
    "leadingbJet_eta":        ("Leading b-Jet #eta",           "#eta^{lead b-jet}"),
    "subleadingbJet_pt":      ("Subleading b-Jet p_{T}",       "p_{T}^{sublead b-jet} [GeV]"),
    "subleadingbJet_eta":     ("Subleading b-Jet #eta",        "#eta^{sublead b-jet}"),
    "goodMET_pt":             ("Missing Transverse Momentum",  "p_{T}^{miss} [GeV]"),
    "goodMET_phi":            ("Missing Transverse Momentum #phi", "#phi^{miss}"),
}

# --- special-case exact keys ---
SPECIAL_CASES = {
    "top_mass": {
        "title": "Reconstructed Top Quark Mass",
        "xlabel": "m_{t} [GeV]",
    },
    "BaseRegion": {
        "title": "Base Region: Event Yield",
        "xlabel": "Event Category",
    },
}


def split_region_and_var(key):
    """Return (region_label_or_None, variable_key) for a config key."""
    for prefix, label in REGION_PREFIXES:
        if key.startswith(prefix):
            return label, key[len(prefix):]
    return None, key


def make_title_xlabel(key):
    if key in SPECIAL_CASES:
        return SPECIAL_CASES[key]["title"], SPECIAL_CASES[key]["xlabel"]

    region_label, var_key = split_region_and_var(key)

    if var_key in VARIABLE_MAP:
        var_title, xlabel = VARIABLE_MAP[var_key]
    else:
        # Fallback: turn camelCase / snake_case into a readable title
        readable = re.sub(r"(?<!^)(?=[A-Z])", " ", var_key).replace("_", " ")
        var_title = readable.strip().title()
        xlabel = var_title
        print(f"Warning: no explicit mapping for variable '{var_key}' "
              f"(key '{key}') -> using generic label '{var_title}'")

    if region_label:
        title = f"{region_label}: {var_title}"
    else:
        title = var_title

    return title, xlabel


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_config.json> <output_config.json>")
        sys.exit(1)

    infile, outfile = sys.argv[1], sys.argv[2]

    with open(infile, "r") as f:
        config = json.load(f)

    new_config = {}
    for key, entry in config.items():
        title, xlabel = make_title_xlabel(key)
        new_config[key] = {
            "bins": entry["bins"],
            "xmin": entry["xmin"],
            "xmax": entry["xmax"],
            "title": title,
            "xlabel": xlabel,
        }

    with open(outfile, "w") as f:
        json.dump(new_config, f, indent=2)

    print(f"Wrote {len(new_config)} entries with title/xlabel to {outfile}")


if __name__ == "__main__":
    main()
