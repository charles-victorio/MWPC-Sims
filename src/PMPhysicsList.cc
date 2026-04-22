// #include "PMPhysicsList.hh"

// PMPhysicsList::PMPhysicsList() {
//     // EM Physics
//     // RegisterPhysics(new G4EmStandardPhysics());

//     G4VModularPhysicsList *physList = new FTFP_BERT;
// }

// PMPhysicsList::~PMPhysicsList() {
    
// }

// PMPhysicsList.cc
// MWPC cosmic muon simulation — Ar/CO2 75:25
//
// Physics strategy:
//   Base:  FTFP_BERT  (hadronic + standard EM as fallback outside gas)
//   EM:    G4EmStandardPhysics_option4 replaces the default EM constructor
//          option4 uses the most accurate EM models for thin absorbers and
//          correctly handles delta-ray production (important for Q_99 tail)
//   PAI:   G4PAIPhotModel applied to the gas region only
//          PAI (Photo-Absorption Ionization) gives the correct Landau-Vavilov
//          fluctuations in thin gas gaps that standard Bethe-Bloch misses.
//          Without it Q_MPV and especially Q_5 will be wrong.
//   Step:  User-defined step limit in gas so individual deposits are resolved
//          and not smeared into one big step.

#include "PMPhysicsList.hh"

// --- Geant4 headers ---------------------------------------------------------
#include "G4SystemOfUnits.hh"
#include "G4RegionStore.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"

// Base physics list
#include "FTFP_BERT.hh"

// EM replacement
#include "G4EmStandardPhysics_option4.hh"

// PAI model
#include "G4PAIPhotModel.hh"
#include "G4EmConfigurator.hh"
#include "G4LossTableManager.hh"

// Step limiter
#include "G4StepLimiterPhysics.hh"
#include "G4UserSpecialCuts.hh"

// Particles we apply PAI to
#include "G4MuonPlus.hh"
#include "G4MuonMinus.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
#include "G4PionPlus.hh"
#include "G4PionMinus.hh"
#include "G4Proton.hh"

// ----------------------------------------------------------------------------

PMPhysicsList::PMPhysicsList() : G4VModularPhysicsList() {
    SetVerboseLevel(1);

    // -------------------------------------------------------------------------
    // 1. Start from FTFP_BERT.
    //    This gives us hadronic physics (not really needed for cosmic muons but
    //    good practice) and a baseline EM constructor which we immediately
    //    replace below.
    // -------------------------------------------------------------------------
    G4VModularPhysicsList* base = new FTFP_BERT(0);  // 0 = quiet

    // Transfer all registered physics constructors from FTFP_BERT to this list
    for (G4int i = 0; ; ++i) {
        G4VPhysicsConstructor* elem = const_cast<G4VPhysicsConstructor*>(
            base->GetPhysics(i));
        if (!elem) break;
        RegisterPhysics(elem);
    }

    // -------------------------------------------------------------------------
    // 2. Replace the default EM physics with option4.
    //    option4 enables:
    //      - G4LivermoreIonisationModel for e- below 100 keV
    //      - G4MuBetheBlochModel with full delta-ray production for muons
    //      - Precise fluorescence and Auger electron emission
    //      - Urban model for multiple scattering (most accurate for thin layers)
    //    This is the right choice for a gas detector where delta rays (knock-on
    //    electrons) are responsible for the high-charge tail you measure as Q_99.
    // -------------------------------------------------------------------------
    ReplacePhysics(new G4EmStandardPhysics_option4(0));

    // -------------------------------------------------------------------------
    // 3. Add step limiter so Geant4 doesn't take one giant step across the
    //    entire 6 mm gas gap. Without this, energy deposits in thin volumes
    //    can be incorrectly attributed or lost entirely.
    //    The actual limit value is set in ConstructProcess() below.
    // -------------------------------------------------------------------------
    RegisterPhysics(new G4StepLimiterPhysics());
}

// ----------------------------------------------------------------------------

PMPhysicsList::~PMPhysicsList() {}

// ----------------------------------------------------------------------------

void PMPhysicsList::ConstructParticle()
{
    // Delegate to all registered constructors
    G4VModularPhysicsList::ConstructParticle();
}

// ----------------------------------------------------------------------------

void PMPhysicsList::ConstructProcess()
{
    // Let all registered constructors run first (builds the process tables)
    G4VModularPhysicsList::ConstructProcess();

    // Now attach PAI models and step limits on top
    AddPAIModel();
    SetStepLimits();
}

// ----------------------------------------------------------------------------

void PMPhysicsList::SetCuts()
{
    // Default production cuts for the world — 1 mm is standard
    SetCutsWithDefault();
    SetDefaultCutValue(1.0 * mm);

    // Tighter cuts in the gas region so delta rays are tracked properly.
    // A 30 µm cut means electrons must have enough energy to travel 30 µm
    // before being produced as secondaries — below that their energy is
    // deposited locally (continuous energy loss).
    // For Ar/CO2 at NTP, 30 µm corresponds to ~1 keV electrons.
    // This is a good balance: resolves the ionization structure without
    // tracking millions of very low-energy secondaries.
    G4Region* gasRegion = G4RegionStore::GetInstance()->GetRegion("GasRegion", false);
    if (gasRegion) {
        G4ProductionCuts* gasCuts = new G4ProductionCuts();
        gasCuts->SetProductionCut(30 * um, G4ProductionCuts::GetIndex("gamma"));
        gasCuts->SetProductionCut(30 * um, G4ProductionCuts::GetIndex("e-"));
        gasCuts->SetProductionCut(30 * um, G4ProductionCuts::GetIndex("e+"));
        gasCuts->SetProductionCut(30 * um, G4ProductionCuts::GetIndex("proton"));
        gasRegion->SetProductionCuts(gasCuts);
        G4cout << "[PMPhysicsList] Production cuts set to 30 µm in GasRegion" << G4endl;
    } else {
        G4cout << "[PMPhysicsList] WARNING: GasRegion not found — "
               << "using default cuts everywhere. "
               << "Make sure your DetectorConstruction creates a G4Region "
               << "named \"GasRegion\" and assigns the gas logical volume to it."
               << G4endl;
    }
}

// ----------------------------------------------------------------------------

void PMPhysicsList::AddPAIModel()
{
    // -------------------------------------------------------------------------
    // PAI (Photo-Absorption Ionization) model
    //
    // The PAI model uses measured photoabsorption cross-sections to compute
    // ionization fluctuations in thin absorbers. It gives the correct Landau
    // distribution shape — in particular the low-charge side (Q_5) which
    // standard Bethe-Bloch overestimates.
    //
    // G4PAIPhotModel is the recommended variant: it includes both ionization
    // and Cherenkov-photon production contributions to energy loss, giving
    // better agreement with measurements in noble gas mixtures.
    //
    // We apply it to all charged particle types that a cosmic muon can produce
    // in the gas: the muon itself, knock-on electrons (delta rays), and
    // positrons from pair production (rare but present).
    //
    // The model is region-restricted to GasRegion — outside the gas it falls
    // back to the standard option4 models registered above.
    // -------------------------------------------------------------------------

    G4EmConfigurator* config = G4LossTableManager::Instance()->EmConfigurator();

    G4Region* gasRegion = G4RegionStore::GetInstance()->GetRegion("GasRegion", false);

    if (!gasRegion) {
        G4cout << "[PMPhysicsList] WARNING: GasRegion not found — "
               << "PAI model NOT applied. Charge spectrum will be incorrect."
               << G4endl;
        return;
    }

    // Energy range over which PAI is active.
    // Lower bound: 100 eV — below this the particle is below tracking threshold
    // Upper bound: 100 TeV — effectively no upper limit for cosmic muons
    G4double eMin = 100 * eV;
    G4double eMax = 100 * TeV;

    // Particle list: everything that can deposit ionization in the gas.
    // Pions and protons are included for completeness (hadronic showers from
    // muon-nuclear interactions, rare but possible at high muon energies).
    struct ParticlePAI {
        const G4String particleName;
        const G4String processName;   // must match the ionisation process name
    };

    std::vector<ParticlePAI> particles = {
        { "mu+",    "muIoni"  },
        { "mu-",    "muIoni"  },
        { "e-",     "eIoni"   },
        { "e+",     "eIoni"   },
        { "pi+",    "hIoni"   },
        { "pi-",    "hIoni"   },
        { "proton", "hIoni"   },
    };


    // maybe this can be  done lgobally idk
    G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition *muon = particleTable->FindParticle("mu+");

    for (const auto& p : particles) {
        // Each particle gets its own PAIPhotModel instance — they are not shared
        G4PAIPhotModel* paiModel = new G4PAIPhotModel(muon, p.particleName + "_PAI");

        config->SetExtraEmModel(
            p.particleName,   // particle name
            p.processName,    // ionisation process to attach to
            paiModel,         // the model
            "GasRegion",      // region name (must match your G4Region name)
            eMin,             // model active from
            eMax,             // model active to
            new G4PAIPhotModel(muon, p.particleName + "_PAI_fluct")  // fluctuation model
        );

        G4cout << "[PMPhysicsList] PAIPhotModel attached to "
               << p.particleName << " / " << p.processName
               << " in GasRegion" << G4endl;
    }
}

// ----------------------------------------------------------------------------

void PMPhysicsList::SetStepLimits()
{
    // -------------------------------------------------------------------------
    // Step size limit in the gas volume.
    //
    // Without a step limit, Geant4 may traverse the entire 6 mm gap in one
    // step, which:
    //   (a) prevents PAI from sampling fluctuations at the right scale
    //   (b) prevents correct attribution of delta-ray production positions
    //
    // A limit of 0.5 mm gives ~12 steps across the gap — enough to resolve
    // the ionization structure without excessive CPU overhead.
    // For very precise cluster shape studies you could go to 0.1 mm (60 steps)
    // but for the charge spectrum this is unnecessary.
    //
    // G4StepLimiterPhysics (registered in constructor) adds the G4StepLimiter
    // process to all charged particles. The actual limit is set via
    // G4UserLimits attached to the logical volume in DetectorConstruction.
    // Here we just confirm the value in the output so it is visible in the log.
    // -------------------------------------------------------------------------

    G4cout << "[PMPhysicsList] Step limit: set 0.5 mm on GasRegion logical volume "
           << "via G4UserLimits in DetectorConstruction." << G4endl;
    G4cout << "[PMPhysicsList]   e.g.:  gasLV->SetUserLimits("
           << "new G4UserLimits(0.5*mm));" << G4endl;

    // NOTE: The G4UserLimits object must be attached to the logical volume in
    // DetectorConstruction, not here. This is a Geant4 design constraint —
    // logical volumes are not accessible from the physics list at construction
    // time. Add this line to your DetectorConstruction::Construct():
    //
    //   #include "G4UserLimits.hh"
    //   gasLogicalVolume->SetUserLimits(new G4UserLimits(0.5*mm));
    //
    // The G4StepLimiterPhysics constructor registered above will pick it up
    // automatically during run initialization.
}