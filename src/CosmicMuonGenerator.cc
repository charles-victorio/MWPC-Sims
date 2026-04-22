#include "CosmicMuonGenerator.hh"

#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

#include <cmath>

CosmicMuonGenerator::CosmicMuonGenerator() {
    fGun = new G4ParticleGun(1);

    // Fix energy — charge spectrum shape doesn't depend on this
    // above ~1 GeV. 4 GeV is a reasonable mean
    auto *table = G4ParticleTable::GetParticleTable();
    fGun->SetParticleDefinition(table->FindParticle("mu+")); // mu+ vs mu- doesn't affect charge deposit
    fGun->SetParticleEnergy(4*GeV);
}

void CosmicMuonGenerator::GeneratePrimaries(G4Event *event) {
    // angular sampling I(theta) ~ cos^2(theta)
    // Inverse CDF: theta = arccos(u^(1/3)), u ~ Uniform(0, 1)
    // but restrict theta beyond 70 degs
    G4double uMin = std::pow(std::cos(70.0 * deg), 3.0);
    G4double u = uMin + (1.0 - uMin) * G4UniformRand();
    G4double theta = std::acos(std::pow(u, 1.0/3.0));
    G4double phi = 2.0 * CLHEP::pi * G4UniformRand();

    // Direction vector (-z)
    G4double sinTheta = std::sin(theta);
    G4ThreeVector dir(
        sinTheta * std::cos(phi),
        sinTheta * std::sin(phi),
        -std::cos(theta)
    );
    fGun->SetParticleMomentumDirection(dir);

    // Spawn position on a plane above the detector
    // sample uniformly over your detector footprint + margin
    G4double xSpawn = (G4UniformRand() - 0.5) * 16*cm;
    G4double ySpawn = (G4UniformRand() - 0.5) * 16*cm;
    G4double zSpawn = 10*cm;

    fGun->SetParticlePosition(G4ThreeVector(xSpawn, ySpawn, zSpawn));
    fGun->GeneratePrimaryVertex(event);
}