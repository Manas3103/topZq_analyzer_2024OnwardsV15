import json
import re


# -------------------------------------------------
# Region naming standardization
# -------------------------------------------------
REGION_MAP = {
    "SR": "Signal Region",
    "WZ": "WZ Control Region",
    "ZZ": "ZZ Control Region",
    "NP1": "Nonprompt-1 Region",
    "NP2": "Nonprompt-2 Region",
    "XG": "Xγ Region",
    "X_gamma": "Xγ Region",
    "ttZ4L": "ttZ (4ℓ) Region",
    "ttZ": "ttZ Region",
    "TR": "Transfer Region",
}


# -------------------------------------------------
# Helper: clean object name
# -------------------------------------------------
def extract_object_name(name):
    name = re.sub(r"^(SR_|WZ_|ZZ_|NP1_|NP2_|XG_|X_gamma_|ttZ_|ttZ4L_|TR_)", "", name)
    name = re.sub(r"(_pt|_eta|_phi|_mass)", "", name)
    return prettify(name)


def prettify(name):
    name = name.replace("_", " ")
    return name[:1].upper() + name[1:]


# -------------------------------------------------
# Build x-axis title
# -------------------------------------------------
def build_xtitle(var):

    if "goodMET_pt" in var:
        return "Missing transverse momentum p_{T}^{miss} [GeV]"

    if "goodMET_phi" in var:
        return "Missing transverse momentum φ"

    if "transversMass" in var:
        return "Transverse mass M_{T} [GeV]"

    if "_pt" in var:
        return extract_object_name(var) + " p_{T} [GeV]"

    if "_eta" in var:
        return extract_object_name(var) + " η"

    if "_phi" in var:
        return extract_object_name(var) + " φ"

    if "_mass" in var:
        return extract_object_name(var) + " mass [GeV]"

    if "ncleanjet" in var:
        return "Number of clean jets"

    if "ncleanbjet" in var:
        return "Number of clean b-jets"

    if "multiplicity" in var:
        return "Multiplicity"

    return prettify(var)


# -------------------------------------------------
# Build histogram title
# -------------------------------------------------
def build_title(var):

    region_label = ""
    for key in REGION_MAP:
        if var.startswith(key + "_"):
            region_label = REGION_MAP[key]
            break

    base = build_xtitle(var).replace(" [GeV]", "")

    if region_label:
        return f"{base} ({region_label})"

    return base


# -------------------------------------------------
# Main conversion
# -------------------------------------------------
def convert(input_file, output_file):

    with open(input_file) as f:
        old_config = json.load(f)

    new_config = {}

    for var, cfg in old_config.items():

        updated = cfg.copy()
        updated["xtitle"] = build_xtitle(var)
        updated["ytitle"] = "Events"
        updated["title"] = build_title(var)

        new_config[var] = updated

    with open(output_file, "w") as f:
        json.dump(new_config, f, indent=2)

    print(f"Converted JSON written to: {output_file}")


# -------------------------------------------------
# Run from command line
# -------------------------------------------------
if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("input_json", help="Old JSON file")
    parser.add_argument("output_json", help="Output JSON file")
    args = parser.parse_args()

    convert(args.input_json, args.output_json)
