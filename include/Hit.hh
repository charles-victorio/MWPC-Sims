#pragma once

#if 0

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4SystemOfUnits.hh"

class Hit : public G4VHit {
public:
    Hit() = default;
    ~Hit() = default;

    void AddEdep(G4double e) { fEdep += e; }
    G4double GetEdep() const { return fEdep; }

    void Reset() { fEdep = 0.0; }

private:
    G4double fEdep = 0.0; // [MeV]
};

using HitsCollection = G4THitsCollection<Hit>;

#endif