#ifndef HELIX_GARFSIM_PLOTTING_HH
#define HELIX_GARFSIM_PLOTTING_HH

#include <TColor.h>
#include <TROOT.h>
#include <TStyle.h>

#include "Garfield/Random.hh"
#include "Garfield/RandomEngineRoot.hh"
#include "Garfield/Sensor.hh"

namespace Garfield {

class LegacyRandomEngine : public RandomEngineRoot {
 public:
  using seed_t = RandomEngineRoot::seed_t;
  LegacyRandomEngine() { Random::SetEngine(*this); }
  explicit LegacyRandomEngine(const seed_t& seed) : RandomEngineRoot(seed) {
    Random::SetEngine(*this);
  }
  void Seed(const seed_t& seed) {
    SetSeed(seed);
    Random::SetEngine(*this);
  }
};

// Minimal plotting helper used by the HELIX macros.
class PlottingEngine {
 public:
  void SetDefaultStyle() const {
    if (!gStyle) return;
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetCanvasColor(kWhite);
    gStyle->SetPadColor(kWhite);
    gStyle->SetFrameFillColor(kWhite);
    gStyle->SetFrameLineColor(kBlack);
    gStyle->SetPalette(kBird);
    gStyle->SetNumberContours(255);
    gStyle->SetPadTopMargin(0.08);
    gStyle->SetPadBottomMargin(0.12);
    gStyle->SetPadLeftMargin(0.12);
    gStyle->SetPadRightMargin(0.1);
    gStyle->SetTitleOffset(1.2, "Y");
    gStyle->SetTitleOffset(1.2, "Z");
  }
};

inline PlottingEngine plottingEngine;
inline LegacyRandomEngine randomEngine;

}  // namespace Garfield

#endif  // HELIX_GARFSIM_PLOTTING_HH
