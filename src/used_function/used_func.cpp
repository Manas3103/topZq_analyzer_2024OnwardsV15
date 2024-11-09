///// ***********These are all the functions that are compatible with the code but not complete codes********************


//########################################################################################################################
//===============THIS CODE IS TO FIND Z BOSON MASS ONLY USING MUONS AND WHICH CONTAINS 2 MUONS EXACTLY====================
//########################################################################################################################
void BaseAnalyser::calculateZBosonMass() {
    constexpr double Z_BOSON_MASS = 91.1876;
    constexpr double MASS_WINDOW = 15.0;

    cout << "Calculating effective mass of two muons within Z boson mass window" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    // Filter events with exactly two trailing muons
    _rlm = _rlm.Filter("NtrailingMuons == 2", "Events with exactly two trailing muons");

    // Calculate the invariant mass of the two-muon system if they are OSSF
    _rlm = _rlm.Define("zboson_m", [Z_BOSON_MASS, MASS_WINDOW](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& muons_4vec, 
                                                              const ROOT::VecOps::RVec<int>& trailingMuons_charge) {
        if (muons_4vec.size() == 2 && trailingMuons_charge.size() == 2) {
            // Check if the muons form an opposite-sign, same-flavor (OSSF) pair
            if (trailingMuons_charge[0] != trailingMuons_charge[1]) {
                double effective_mass = (muons_4vec[0] + muons_4vec[1]).M();
                // Check if the effective mass is within the Z boson mass window
                if (std::abs(effective_mass - Z_BOSON_MASS) <= MASS_WINDOW) {
                    return effective_mass;
                }
            }
        }
        return -1.0;  // Return a negative value to indicate it didn't meet the criteria
    }, {"trailingMuons_4vecs", "trailingMuons_charge"});
    // Filter events where zboson_m has a valid mass
    _rlm = _rlm.Filter("zboson_m > 0", "Events with OSSF pair within Z boson mass window");
}


//########################################################################################################################
//===============THIS CODE IS TO FIND Z BOSON MASS ONLY USING MUONS AND WHICH CONTAINS 2 AND 3  MUONS EXACTLY=============
//########################################################################################################################



void BaseAnalyser::calculateZBosonMass() {
    constexpr double Z_BOSON_MASS = 91.1876;
    constexpr double MASS_WINDOW = 15.0;

    cout << "Calculating effective mass of muon pairs within Z boson mass window" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    // Filter events with exactly two or three trailing muons
    _rlm = _rlm.Filter("NtrailingMuons == 2 || NtrailingMuons == 3", "Events with two or three trailing muons");

    // Calculate the invariant mass for Z boson candidates
    _rlm = _rlm.Define("zboson_m", [Z_BOSON_MASS, MASS_WINDOW](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& muons_4vec,
                                                               const ROOT::VecOps::RVec<int>& trailingMuons_charge) {
        double closest_mass = -1.0;

        if (muons_4vec.size() == 2) {
            // Case 1: Exactly two muons
            if (trailingMuons_charge[0] != trailingMuons_charge[1]) {  // Check OSSF condition
                double effective_mass = (muons_4vec[0] + muons_4vec[1]).M();
                if (std::abs(effective_mass - Z_BOSON_MASS) <= MASS_WINDOW) {
                    closest_mass = effective_mass;
                }
            }
        } else if (muons_4vec.size() == 3) {
            // Case 2: Exactly three muons
            std::vector<double> masses;

            // Check all possible OSSF pairs (0-1, 0-2, 1-2)
            if (trailingMuons_charge[0] != trailingMuons_charge[1]) {
                masses.push_back((muons_4vec[0] + muons_4vec[1]).M());
            }
            if (trailingMuons_charge[0] != trailingMuons_charge[2]) {
                masses.push_back((muons_4vec[0] + muons_4vec[2]).M());
            }
            if (trailingMuons_charge[1] != trailingMuons_charge[2]) {
                masses.push_back((muons_4vec[1] + muons_4vec[2]).M());
            }

            // Filter masses within the Z boson window and find the closest
            double min_diff = MASS_WINDOW;
            for (double mass : masses) {
                double diff = std::abs(mass - Z_BOSON_MASS);
                if (diff <= MASS_WINDOW && diff < min_diff) {
                    min_diff = diff;
                    closest_mass = mass;
                }
            }
        }

        return closest_mass;
    }, {"trailingMuons_4vecs", "trailingMuons_charge"});

    // Filter events where zboson_m has a valid mass
    _rlm = _rlm.Filter("zboson_m > 0", "Events within Z boson mass window");
}

//########################################################################################################################
//===============THIS CODE IS TO FIND Z BOSON MASS ONLY USING LEPTONS AND WHICH CONTAINS 2 AND 3  MUONS EXACTLY===========


//++++++++++++++++++++++++++++++++++++++++++++++++++++THIS CODE IS GIVING SOME ERROR++++++++++++++++++++++++++++++++++++++
//########################################################################################################################



void BaseAnalyser::calculateZBosonMass() {
    constexpr double Z_BOSON_MASS = 91.1876;
    constexpr double MASS_WINDOW = 15.0;

    cout << "Calculating effective mass of muon and electron pairs within Z boson mass window" << endl;
    if (debug) {
        std::cout << "================================//=================================" << std::endl;
        std::cout << "Line : " << __LINE__ << " Function : " << __FUNCTION__ << std::endl;
        std::cout << "================================//=================================" << std::endl;
    }

    // Filter events with exactly 2 or 3 trailing muons and exactly 2 or 3 trailing electrons
    _rlm = _rlm.Filter("NtrailingMuons == 2 || NtrailingElectrons == 2 || NtrailingMuons == 3 || NtrailingElectrons == 3",
                       "Events with exactly 2 or 3 trailing muons and electrons");

    // Define zboson_m based on trailing muons and electrons
    _rlm = _rlm.Define("zboson_m", [Z_BOSON_MASS, MASS_WINDOW](const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& trailingMuons_4vecs,
                                                               const ROOT::VecOps::RVec<int>& trailingMuons_charge,
                                                               const std::vector<ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>>& trailingElectrons_4Vecs,
                                                               const ROOT::VecOps::RVec<int>& trailingElectrons_charge) {
        // Collect OSSF pairs and their invariant masses
        std::vector<double> ossf_masses;

        // Case 1: 2 trailing muons and 2 trailing electrons
        if (trailingMuons_4vecs.size() == 2 || trailingElectrons_4Vecs.size() == 2) {
            if (trailingMuons_charge[0] != trailingMuons_charge[1]) {
                ossf_masses.push_back((trailingMuons_4vecs[0] + trailingMuons_4vecs[1]).M());
            }
            if (trailingElectrons_charge[0] != trailingElectrons_charge[1]) {
                ossf_masses.push_back((trailingElectrons_4Vecs[0] + trailingElectrons_4Vecs[1]).M());
            }
        }

        // Case 2: 3 trailing muons and 3 trailing electrons
        if (trailingMuons_4vecs.size() == 3 || trailingElectrons_4Vecs.size() == 3) {
            // Check all possible OSSF pairs for muons
            if (trailingMuons_charge[0] != trailingMuons_charge[1]) {
                ossf_masses.push_back((trailingMuons_4vecs[0] + trailingMuons_4vecs[1]).M());
            }
            if (trailingMuons_charge[0] != trailingMuons_charge[2]) {
                ossf_masses.push_back((trailingMuons_4vecs[0] + trailingMuons_4vecs[2]).M());
            }
            if (trailingMuons_charge[1] != trailingMuons_charge[2]) {
                ossf_masses.push_back((trailingMuons_4vecs[1] + trailingMuons_4vecs[2]).M());
            }

            // Check all possible OSSF pairs for electrons
            if (trailingElectrons_charge[0] != trailingElectrons_charge[1]) {
                ossf_masses.push_back((trailingElectrons_4Vecs[0] + trailingElectrons_4Vecs[1]).M());
            }
            if (trailingElectrons_charge[0] != trailingElectrons_charge[2]) {
                ossf_masses.push_back((trailingElectrons_4Vecs[0] + trailingElectrons_4Vecs[2]).M());
            }
            if (trailingElectrons_charge[1] != trailingElectrons_charge[2]) {
                ossf_masses.push_back((trailingElectrons_4Vecs[1] + trailingElectrons_4Vecs[2]).M());
            }
        }

        // Find the mass closest to the Z boson mass within the window
        double closest_mass = -1.0;
        double min_diff = MASS_WINDOW;

        for (const auto& mass : ossf_masses) {
            double diff = std::abs(mass - Z_BOSON_MASS);
            if (diff <= MASS_WINDOW && diff < min_diff) {
                min_diff = diff;
                closest_mass = mass;
            }
        }

        return closest_mass;
    }, {"trailingMuons_4vecs", "trailingMuons_charge", "trailingElectrons_4Vecs", "trailingElectrons_charge"});

    // Filter to keep only events with valid Z boson masses within the mass window
    _rlm = _rlm.Filter("zboson_m > 0", "Events with invariant mass close to Z boson mass");
}

