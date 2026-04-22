#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "Randomize.hh"

class CosmicMuonGenerator : public G4VUserPrimaryGeneratorAction {
public:
    CosmicMuonGenerator();
    void GeneratePrimaries(G4Event *event) override;

private:
    G4ParticleGun *fGun;
};