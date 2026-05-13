import json
import re


# -------------------------------------------------
# Region naming
# -------------------------------------------------
REGION_MAP = {
    "ThreeLRegion": "Three-Lepton Region",
    "SR": "Signal Region",
    "WZ": "WZ Control Region",
    "ZZ": "ZZ Control Region",
    "NP1": "Nonprompt-1 Region",
    "NP2": "Nonprompt-2 Region",
    "XG": "Xγ Region",
    "ttZ4L": "ttZ (4ℓ) Region",
    "ttZ": "ttZ Region",
    "TR": "Transfer Region",
}


# -------------------------------------------------
# Flavor channel detection
# -------------------------------------------------
FLAVOR_MAP = {
    "uuu": "μμμ",
    "uue": "μμe",
    "eeu": "eeμ",
    "eee": "eee"
}


# -------------------------------------------------
# Extract flavor prefix
# -------------------------------------------------
def extract_flavor(var):

    for key in FLAVOR_MAP:
        if var.startswith(key + "_"):
            return FLAVOR_MAP[key], var[len(key)+1:]

    return None, var


# -------------------------------------------------
# Extract region
# -------------------------------------------------
def extract_region(var):

    for key in REGION_MAP:
        if key in var:
            return REGION_MAP[key]

    return None


# -------------------------------------------------
# Clean object name
# -------------------------------------------------
def extract_object_name(name):

    name = re.sub(r"^(ThreeLRegion_|SR_|WZ_|ZZ_|NP1_|NP2_|XG_|ttZ_|ttZ4L_|TR_)", "", name)
    name = re.sub(r"(_pt|_eta|_phi|_mass)", "", name)
    name = name.replace("_", " ")

    return name.capitalize()


# -------------------------------------------------
# X-axis builder
# -------------------------------------------------
def build_xtitle(var):

    if "goodMET_pt" in var:
        return "Missing transverse momentum p_{T}^{miss} [GeV]"

    if "goodMET_phi" in var:
        return "Missing transverse momentum φ"

    if "topLepton_pt" in var:
        return "Highest p_{T} lepton p_{T} [GeV]"

    if "_pt" in var:
        return extract_object_name(var) + " p_{T} [GeV]"

    if "_eta" in var:
        return extract_object_name(var) + " η"

    if "_phi" in var:
        return extract_object_name(var) + " φ"

    if "_mass" in var:
        return extract_object_name(var) + " mass [GeV]"

    if "nJets" in var:
        return "Number of jets"

    if "nbJets" in var:
        return "Number of b-jets"

    if "multiplicity" in var:
        return "Multiplicity"

    return extract_object_name(var)


# -------------------------------------------------
# Title builder
# -------------------------------------------------
def build_title(var):

    flavor, remainder = extract_flavor(var)
    region = extract_region(remainder)

    base = build_xtitle(var).replace(" [GeV]", "")

    title_parts = []

    if region:
        title_parts.append(region)

    if flavor:
        title_parts.append(flavor)

    if title_parts:
        return f"{base} ({', '.join(title_parts)})"

    return base


# -------------------------------------------------
# Conversion
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
# Run
# -------------------------------------------------
if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("input_json")
    parser.add_argument("output_json")
    args = parser.parse_args()

    convert(args.input_json, args.output_json)
