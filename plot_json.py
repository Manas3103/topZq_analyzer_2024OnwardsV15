import ROOT
import correctionlib
import numpy as np

# ============================================================
# Input
# ============================================================

json_file = "data/JERC/2024_Summer24/jetvetomaps.json"

# Change this to the correction name printed by cset.keys()
correction_name = "Summer24Prompt24_RunBCDEFGHI_V1"

# ============================================================
# Load correctionlib JSON
# ============================================================

cset = correctionlib.CorrectionSet.from_file(json_file)

print("Available corrections:")
print(cset.keys())

corr = cset[correction_name]

# ============================================================
# Create TH2D
# ============================================================

h = ROOT.TH2D(
    "jetvetomap",
    "Jet Veto Map;#eta;#phi",
    82, -5.191, 5.191,
    72, -np.pi, np.pi
)

# ============================================================
# Fill histogram
# ============================================================

for ix in range(1, h.GetNbinsX() + 1):

    eta = h.GetXaxis().GetBinCenter(ix)

    for iy in range(1, h.GetNbinsY() + 1):

        phi = h.GetYaxis().GetBinCenter(iy)

        value = corr.evaluate(
            "jetvetomap",
            eta,
            phi
        )
        inverted_value = 100 - value
        h.SetBinContent(ix, iy, inverted_value)

# ============================================================
# Draw
# ============================================================

ROOT.gStyle.SetOptStat(0)

canvas = ROOT.TCanvas(
    "canvas",
    "Jet Veto Map",
    1000,
    800
)

h.Draw("COLZ")

# ============================================================
# Save PDF
# ============================================================

canvas.SaveAs("jetvetomap.pdf")

# ============================================================
# Save ROOT file
# ============================================================

outfile = ROOT.TFile(
    "jetvetomap.root",
    "RECREATE"
)

h.Write()
canvas.Write()

outfile.Close()

print("========================================")
print("Saved:")
print("  jetvetomap.pdf")
print("  jetvetomap.root")
print("========================================")
