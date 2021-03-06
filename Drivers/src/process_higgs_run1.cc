
//	
#ifdef STANDALONE
#include "Muon.h"
#include "Jet.h"
#include "Vertex.h"
#include "Event.h"
#include "MET.h"
#include "Constants.h"
#include "Streamer.h"
#include "MetaHiggs.h"

//	ROOT headers
#include "TFile.h"
#include "TChain.h"
#include "TString.h"
#include "TMath.h"
#include "TH1D.h"
#include "TLorentzVector.h"
#include "LumiReweightingStandAlone.h"
#include "HistogramSets.h"
#include "KalmanMuonCalibrator.h"
#include "RoccoR.h"

#include "boost/program_options.hpp"
#include <signal.h>

#define FILL_JETS(set) \
	set.hDiJetMass->Fill(dijetmass, puweight); \
	set.hDiJetdeta->Fill(TMath::Abs(deta), puweight); \
	set.hDiMuonpt->Fill(p4dimuon.Pt(), puweight); \
	set.hDiMuonMass->Fill(p4dimuon.M(), puweight); \
	set.hDiMuoneta->Fill(p4dimuon.Eta(), puweight); \
	set.hDiMuondphi->Fill(dphi, puweight); \
	set.hMuonpt->Fill(p4m1.Pt(), puweight); \
	set.hMuonpt->Fill(p4m2.Pt(), puweight); \
	set.hMuoneta->Fill(p4m1.Eta(), puweight); \
	set.hMuoneta->Fill(p4m2.Eta(), puweight); \
	set.hMuonphi->Fill(p4m1.Phi(), puweight); \
	set.hMuonphi->Fill(p4m2.Phi(), puweight);

#define FILL_NOJETS(set) \
	set.hDiMuonpt->Fill(p4dimuon.Pt(), puweight); \
	set.hDiMuonMass->Fill(p4dimuon.M(), puweight); \
	set.hDiMuoneta->Fill(p4dimuon.Eta(), puweight); \
	set.hDiMuondphi->Fill(dphi, puweight); \
	set.hMuonpt->Fill(p4m1.Pt(), puweight); \
	set.hMuonpt->Fill(p4m2.Pt(), puweight); \
	set.hMuoneta->Fill(p4m1.Eta(), puweight); \
	set.hMuoneta->Fill(p4m2.Eta(), puweight); \
	set.hMuonphi->Fill(p4m1.Phi(), puweight); \
	set.hMuonphi->Fill(p4m2.Phi(), puweight);

/*
 *	Declare/Define all the service globals
 */
std::string __inputfilename;
std::string __outputfilename;
bool __isMC;
bool __genPUMC;
std::string __puMCfilename;
std::string __puDATAfilename;
bool __continueRunning = true;
bool __kalman = false;
bool __rochester = false;
std::string __rochesterInput;
std::string __kalmanInput;

/*
 *  Define all the Constants
 */
double __muonMatchedPt = 26.;
double __muonMatchedEta = 2.4;
double __muonPt = 10.;
double __muonEta = 2.4;
double __muonIso = 0.25;
double __leadJetPt = 40.;
double __subleadJetPt = 30.;
double __metPt = 40.;
double __dijetMass_VBFTight = 650;
double __dijetdEta_VBFTight = 3.5;
double __dijetMass_ggFTight = 250.;
double __dimuonPt_ggFTight = 50.;
double __dimuonPt_01JetsTight = 10.;

std::string const NTUPLEMAKER_NAME =  "ntuplemaker_H2DiMuonMaker";

namespace po = boost::program_options;
using namespace analysis::core;
using namespace analysis::dimuon;
using namespace analysis::processing;

TH1D *hEventWeights = NULL;
DimuonSet setNoCats("NoCats");
DimuonSet set2Jets("2Jets");
DimuonSet setVBFTight("VBFTight");
DimuonSet setggFTight("ggFTight");
DimuonSet setggFLoose("ggFLoose");

DimuonSet set01Jets("01Jets");
DimuonSet set01JetsTight("01JetsTight");
DimuonSet set01JetsLoose("01JetsLoose");
DimuonSet set01JetsBB("01JetsBB");
DimuonSet set01JetsBO("01JetsBO");
DimuonSet set01JetsBE("01JetsBE");
DimuonSet set01JetsOO("01JetsOO");
DimuonSet set01JetsOE("01JetsOE");
DimuonSet set01JetsEE("01JetsEE");
DimuonSet set01JetsTightBB("01JetsTightBB");
DimuonSet set01JetsTightBO("01JetsTightBO");
DimuonSet set01JetsTightBE("01JetsTightBE");
DimuonSet set01JetsTightOO("01JetsTightOO");
DimuonSet set01JetsTightOE("01JetsTightOE");
DimuonSet set01JetsTightEE("01JetsTightEE");
DimuonSet set01JetsLooseBB("01JetsLooseBB");
DimuonSet set01JetsLooseBO("01JetsLooseBO");
DimuonSet set01JetsLooseBE("01JetsLooseBE");
DimuonSet set01JetsLooseOO("01JetsLooseOO");
DimuonSet set01JetsLooseOE("01JetsLooseOE");
DimuonSet set01JetsLooseEE("01JetsLooseEE");

bool isBarrel(Muon const& m)
{
	return TMath::Abs(m._eta)<0.8;
}

bool isOverlap(Muon const& m)
{
	return TMath::Abs(m._eta)>=0.8 && TMath::Abs(m._eta)<1.6;
}

bool isEndcap(Muon const& m)
{
	return TMath::Abs(m._eta)>=1.6 && TMath::Abs(m._eta)<2.1;
}

bool passVertex(Vertices* v)
{
	if (v->size()==0)
		return false;
	int n=0;
	for (Vertices::const_iterator it=v->begin();
		it!=v->end() && n<20; ++it)
	{
		if (TMath::Abs(it->_z)<24 &&
			it->_ndf>4)
			return true;
		n++;
	}

	return false;
}

// pass individual muon object definition
bool passMuon(Muon const& m)
{
    double muonIsolation = (m._sumChargedHadronPtR04 + std::max(0.,
        m._sumNeutralHadronEtR04 + m._sumPhotonEtR04 - 0.5*m._sumPUPtR04)) / m._pt;

	if (m._isGlobal && m._isTracker &&
		m._pt>10 && TMath::Abs(m._eta)<2.4 &&
		m._isMedium && 
        muonIsolation < 0.25)
		return true;
	return false;
}

bool passHLTMatch(std::vector<Muon> const& muons)
{
    for (std::vector<Muon>::const_iterator it=muons.begin();
        it!=muons.end(); ++it)
        if ((it->_isHLTMatched[1] || it->_isHLTMatched[0]) &&
            it->_pt>26)
            return true;

    return false;
}

bool passMuons(std::vector<Muon> const& muons) 
{
    if (muons.size() < 2) 
        return false;

    for (std::vector<Muon>::const_iterator it=muons.begin();
        it!=muons.end(); ++it)
        if (it->_pt>26)
            return true;

    return false;
}

bool passMuonHLT(Muon const& m)
{
	if ((m._isHLTMatched[1] || m._isHLTMatched[0]) &&
		m._pt>__muonMatchedPt && TMath::Abs(m._eta)<__muonMatchedEta)
		return true;

	return false;
}

bool passMuons(Muon const& m1, Muon const& m2)
{
	if (m1._charge!=m2._charge &&
		passMuon(m1) && passMuon(m2))
		if (passMuonHLT(m1) || passMuonHLT(m2))
			return true;

	return false;
}

float jetMuondR(float jeta,float jphi, float meta, float mphi)
{
	TLorentzVector p4j,p4m;
	p4j.SetPtEtaPhiM(10, jeta, jphi, 0);
	p4m.SetPtEtaPhiM(10, meta, mphi, 0);
	return p4j.DeltaR(p4m);
}

void categorize(Jets* jets, Muon const& mu1, Muon const&  mu2, 
	MET const& met, Event const& event, float puweight=1.)
{
	TLorentzVector p4m1, p4m2;
	p4m1.SetPtEtaPhiM(mu1._pt, mu1._eta, 
		mu1._phi, PDG_MASS_Mu);
	p4m2.SetPtEtaPhiM(mu2._pt, mu2._eta, 
		mu2._phi, PDG_MASS_Mu);
	TLorentzVector p4dimuon = p4m1 + p4m2;

	//	Fill the No Categorization Set
	double dphi = p4m1.DeltaPhi(p4m2);
	setNoCats.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
	setNoCats.hDiMuonMass->Fill(p4dimuon.M(), puweight);
	setNoCats.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
	setNoCats.hDiMuondphi->Fill(dphi, puweight);
	setNoCats.hMuonpt->Fill(p4m1.Pt(), puweight);
	setNoCats.hMuonpt->Fill(p4m2.Pt(), puweight);
	setNoCats.hMuoneta->Fill(p4m1.Eta(), puweight);
	setNoCats.hMuoneta->Fill(p4m2.Eta(), puweight);
	setNoCats.hMuonphi->Fill(p4m1.Phi(), puweight);
	setNoCats.hMuonphi->Fill(p4m2.Phi(), puweight);
	
	std::vector<TLorentzVector> p4jets;
	for (Jets::const_iterator it=jets->begin(); it!=jets->end(); ++it)
	{
		if (it->_pt>30 && TMath::Abs(it->_eta)<4.7)
		{
			if (jetMuondR(it->_eta, it->_phi, mu1._eta, mu1._phi)>0.4 && 
				jetMuondR(it->_eta, it->_phi, mu2._eta, mu2._phi)>0.4)
			{
				TLorentzVector p4;
				p4.SetPtEtaPhiM(it->_pt, it->_eta, it->_phi, it->_mass);
				p4jets.push_back(p4);
			}
		}
	}

	bool isPreSelected = false;
    bool vbf = false;
    bool ggFT = false;
    bool ggFL = false;
    std::pair<int, int> pVBF;
    std::pair<int, int> pGGFT;
    std::pair<int, int> pGGFL;
	if (p4jets.size()>=2)
	{
        for (int i=0; i<p4jets.size(); i++)
            for (int j=i+1; j<p4jets.size(); j++)
            {
                TLorentzVector p4lead = p4jets[i];
                TLorentzVector p4sub = p4jets[j];
		        TLorentzVector dijet = p4lead + p4sub;

		        float deta = p4lead.Eta() - p4sub.Eta();
		        float dijetmass = dijet.M();
			
		        if (p4lead.Pt()>40 && met._pt<40)
		        {
			        isPreSelected = true;

			        //	categorize
			        if (dijetmass>650 && TMath::Abs(deta)>3.5)
			        {
                        vbf = true;
                        pVBF = std::make_pair(i, j);
			        }
                    else if (dijetmass>250 && p4dimuon.Pt()>50)
			        {
                        ggFT = true;
                        pGGFT = std::make_pair(i, j);
                    }
			        else
			        {	
                        ggFL = true;
                        pGGFL = std::make_pair(i, j);
			        }
                }
            }
	}
    if (isPreSelected) 
    {
        float deta = 0;
        float dijetmass = 0;
        if (vbf)
        {
            TLorentzVector p4lead = p4jets[pVBF.first];
            TLorentzVector p4sub = p4jets[pVBF.second];
		    TLorentzVector dijet = p4lead + p4sub;
		    deta = p4lead.Eta() - p4sub.Eta();
		    dijetmass = dijet.M();
        }
        else if (ggFT) 
        {
            TLorentzVector p4lead = p4jets[pGGFT.first];
            TLorentzVector p4sub = p4jets[pGGFT.second];
		    TLorentzVector dijet = p4lead + p4sub;
		    deta = p4lead.Eta() - p4sub.Eta();
		    dijetmass = dijet.M();
        }
        else 
        {
            TLorentzVector p4lead = p4jets[pGGFL.first];
            TLorentzVector p4sub = p4jets[pGGFL.second];
		    TLorentzVector dijet = p4lead + p4sub;
		    deta = p4lead.Eta() - p4sub.Eta();
		    dijetmass = dijet.M();
        }
		set2Jets.hDiJetMass->Fill(dijetmass, puweight);
		set2Jets.hDiJetdeta->Fill(TMath::Abs(deta), puweight);
		set2Jets.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
	    set2Jets.hDiMuonMass->Fill(p4dimuon.M(), puweight);
		set2Jets.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
		set2Jets.hDiMuondphi->Fill(dphi, puweight);
	    set2Jets.hMuonpt->Fill(p4m1.Pt(), puweight);
		set2Jets.hMuonpt->Fill(p4m2.Pt(), puweight);
	    set2Jets.hMuoneta->Fill(p4m1.Eta(), puweight);
		set2Jets.hMuoneta->Fill(p4m2.Eta(), puweight);
		set2Jets.hMuonphi->Fill(p4m1.Phi(), puweight);
	    set2Jets.hMuonphi->Fill(p4m2.Phi(), puweight);
        if (vbf)
        {
			setVBFTight.hDiJetMass->Fill(dijetmass, puweight);
		    setVBFTight.hDiJetdeta->Fill(TMath::Abs(deta), puweight);
			setVBFTight.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
			setVBFTight.hDiMuonMass->Fill(p4dimuon.M(), puweight);
			setVBFTight.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
			setVBFTight.hDiMuondphi->Fill(dphi, puweight);
			setVBFTight.hMuonpt->Fill(p4m1.Pt(), puweight);
			setVBFTight.hMuonpt->Fill(p4m2.Pt(), puweight);
			setVBFTight.hMuoneta->Fill(p4m1.Eta(), puweight);
			setVBFTight.hMuoneta->Fill(p4m2.Eta(), puweight);
		    setVBFTight.hMuonphi->Fill(p4m1.Phi(), puweight);
			setVBFTight.hMuonphi->Fill(p4m2.Phi(), puweight);
            return;
        }
        else if (ggFT)
        {
			setggFTight.hDiJetMass->Fill(dijetmass, puweight);
			setggFTight.hDiJetdeta->Fill(TMath::Abs(deta), puweight);
			setggFTight.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
			setggFTight.hDiMuonMass->Fill(p4dimuon.M(), puweight);
			setggFTight.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
		    setggFTight.hDiMuondphi->Fill(dphi, puweight);
		    setggFTight.hMuonpt->Fill(p4m1.Pt(), puweight);
		    setggFTight.hMuonpt->Fill(p4m2.Pt(), puweight);
			setggFTight.hMuoneta->Fill(p4m1.Eta(), puweight);
			setggFTight.hMuoneta->Fill(p4m2.Eta(), puweight);
			setggFTight.hMuonphi->Fill(p4m1.Phi(), puweight);
		    setggFTight.hMuonphi->Fill(p4m2.Phi(), puweight);
            return;
        }
        else if (ggFL)
        {
			setggFLoose.hDiJetMass->Fill(dijetmass, puweight);
			setggFLoose.hDiJetdeta->Fill(TMath::Abs(deta), puweight);
			setggFLoose.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
			setggFLoose.hDiMuonMass->Fill(p4dimuon.M(), puweight);
			setggFLoose.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
			setggFLoose.hDiMuondphi->Fill(dphi, puweight);
			setggFLoose.hMuonpt->Fill(p4m1.Pt(), puweight);
			setggFLoose.hMuonpt->Fill(p4m2.Pt(), puweight);
			setggFLoose.hMuoneta->Fill(p4m1.Eta(), puweight);
			setggFLoose.hMuoneta->Fill(p4m2.Eta(), puweight);
		    setggFLoose.hMuonphi->Fill(p4m1.Phi(), puweight);
			setggFLoose.hMuonphi->Fill(p4m2.Phi(), puweight);
            return;
        }
    }
	if (!isPreSelected)
	{
		set01Jets.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
		set01Jets.hDiMuonMass->Fill(p4dimuon.M(), puweight);
		set01Jets.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
		set01Jets.hDiMuondphi->Fill(dphi, puweight);
		set01Jets.hMuonpt->Fill(p4m1.Pt(), puweight);
		set01Jets.hMuonpt->Fill(p4m2.Pt(), puweight);
		set01Jets.hMuoneta->Fill(p4m1.Eta(), puweight);
		set01Jets.hMuoneta->Fill(p4m2.Eta(), puweight);
		set01Jets.hMuonphi->Fill(p4m1.Phi(), puweight);
		set01Jets.hMuonphi->Fill(p4m2.Phi(), puweight);
		if (isBarrel(mu1) && isBarrel(mu2))
		{
			//	BB
			FILL_NOJETS(set01JetsBB);
		}
		else if ((isBarrel(mu1) && isOverlap(mu2)) ||
			(isBarrel(mu2) && isOverlap(mu1)))
		{
			//	BO
			FILL_NOJETS(set01JetsBO);
		}
		else if ((isBarrel(mu1) & isEndcap(mu2)) || 
			(isBarrel(mu2) && isEndcap(mu1)))
		{
			//	BE
			FILL_NOJETS(set01JetsBE);
		}
		else if (isOverlap(mu1) && isOverlap(mu2))
		{
			//	OO
			FILL_NOJETS(set01JetsOO);
		}
		else if ((isOverlap(mu1) && isEndcap(mu2)) ||
			(isOverlap(mu2) && isEndcap(mu1)))
		{
			//	OE
			FILL_NOJETS(set01JetsOE);
		}
		else if (isEndcap(mu1) && isEndcap(mu2))
		{
			//	EE
			FILL_NOJETS(set01JetsEE);
		}

		//	separate loose vs tight
		if (p4dimuon.Pt() >= 25.)
		{
			//	01Jet Tight
			set01JetsTight.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
			set01JetsTight.hDiMuonMass->Fill(p4dimuon.M(), puweight);
			set01JetsTight.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
			set01JetsTight.hDiMuondphi->Fill(dphi, puweight);
			set01JetsTight.hMuonpt->Fill(p4m1.Pt(), puweight);
			set01JetsTight.hMuonpt->Fill(p4m2.Pt(), puweight);
			set01JetsTight.hMuoneta->Fill(p4m1.Eta(), puweight);
			set01JetsTight.hMuoneta->Fill(p4m2.Eta(), puweight);
			set01JetsTight.hMuonphi->Fill(p4m1.Phi(), puweight);
			set01JetsTight.hMuonphi->Fill(p4m2.Phi(), puweight);
			if (isBarrel(mu1) && isBarrel(mu2))
			{
				//	BB
				FILL_NOJETS(set01JetsTightBB);
			}
			else if ((isBarrel(mu1) && isOverlap(mu2)) ||
				(isBarrel(mu2) && isOverlap(mu1)))
			{
				//	BO
				FILL_NOJETS(set01JetsTightBO);
			}
			else if ((isBarrel(mu1) & isEndcap(mu2)) || 
				(isBarrel(mu2) && isEndcap(mu1)))
			{
				//	BE
				FILL_NOJETS(set01JetsTightBE);
			}
			else if (isOverlap(mu1) && isOverlap(mu2))
			{
				//	OO
				FILL_NOJETS(set01JetsTightOO);
			}
			else if ((isOverlap(mu1) && isEndcap(mu2)) ||
				(isOverlap(mu2) && isEndcap(mu1)))
			{
				//	OE
				FILL_NOJETS(set01JetsTightOE);
			}
			else if (isEndcap(mu1) && isEndcap(mu2))
			{
				//	EE
				FILL_NOJETS(set01JetsTightEE);
			}
			return;
		}
		else
		{
			//	01Jet Loose
			set01JetsLoose.hDiMuonpt->Fill(p4dimuon.Pt(), puweight);
			set01JetsLoose.hDiMuonMass->Fill(p4dimuon.M(), puweight);
			set01JetsLoose.hDiMuoneta->Fill(p4dimuon.Eta(), puweight);
			set01JetsLoose.hDiMuondphi->Fill(dphi, puweight);
			set01JetsLoose.hMuonpt->Fill(p4m1.Pt(), puweight);
			set01JetsLoose.hMuonpt->Fill(p4m2.Pt(), puweight);
			set01JetsLoose.hMuoneta->Fill(p4m1.Eta(), puweight);
			set01JetsLoose.hMuoneta->Fill(p4m2.Eta(), puweight);
			set01JetsLoose.hMuonphi->Fill(p4m1.Phi(), puweight);
			set01JetsLoose.hMuonphi->Fill(p4m2.Phi(), puweight);
			if (isBarrel(mu1) && isBarrel(mu2))
			{
				//	BB
				FILL_NOJETS(set01JetsLooseBB);
			}
			else if ((isBarrel(mu1) && isOverlap(mu2)) ||
				(isBarrel(mu2) && isOverlap(mu1)))
			{
				//	BO
				FILL_NOJETS(set01JetsLooseBO);
			}
			else if ((isBarrel(mu1) & isEndcap(mu2)) || 
				(isBarrel(mu2) && isEndcap(mu1)))
			{
				//	BE
				FILL_NOJETS(set01JetsLooseBE);
			}
			else if (isOverlap(mu1) && isOverlap(mu2))
			{
				//	OO
				FILL_NOJETS(set01JetsLooseOO);
			}
			else if ((isOverlap(mu1) && isEndcap(mu2)) ||
				(isOverlap(mu2) && isEndcap(mu1)))
			{
				//	OE
				FILL_NOJETS(set01JetsLooseOE);
			}
			else if (isEndcap(mu1) && isEndcap(mu2))
			{
				//	EE
				FILL_NOJETS(set01JetsLooseEE);
			}
			return;
		}
	}
	
	return;
}

float sampleinfo(std::string const& inputname)
{
	Streamer s(inputname, NTUPLEMAKER_NAME+"/Meta");
	s.chainup();

	using namespace analysis::dimuon;
	MetaHiggs *meta=NULL;
	s._chain->SetBranchAddress("Meta", &meta);

	long long int numEvents = 0;
	long long int numEventsWeighted = 0;
	for (int i=0; i<s._chain->GetEntries(); i++)
	{
		s._chain->GetEntry(i);
		numEvents+=meta->_nEventsProcessed;
		numEventsWeighted+=meta->_sumEventWeights;
	}
	std::cout 
		<< "#events processed total = " << numEvents << std::endl
		<< "#events weighted total = " << numEventsWeighted << std::endl;

	return numEventsWeighted;
}

void generatePUMC()
{
    std::cout << "### Generate PU MC file...." << std::endl;
	TFile *pufile = new TFile(__puMCfilename.c_str(), "recreate");
	TH1D *h = new TH1D("pileup", "pileup", 50, 0, 50);

	Streamer s(__inputfilename, NTUPLEMAKER_NAME+"/Events");
	s.chainup();

	EventAuxiliary *aux=NULL;
	s._chain->SetBranchAddress("EventAuxiliary", &aux);
	int numEvents = s._chain->GetEntries();
	for (uint32_t i=0; i<numEvents; i++)
	{
		s._chain->GetEntry(i);
		h->Fill(aux->_nPU, aux->_genWeight);
	}

	pufile->Write();
	pufile->Close();
}

void process()
{
	//	out ...
	TFile *outroot = new TFile(__outputfilename.c_str(), "recreate");
    hEventWeights = new TH1D("eventWeights", "eventWeights", 1, 0, 1);
	setNoCats.init();
	set2Jets.init();
	set01Jets.init();
	setVBFTight.init();
	setggFTight.init();
	setggFLoose.init();
	set01JetsTight.init();
	set01JetsLoose.init();
	set01JetsBB.init();
	set01JetsBO.init();
	set01JetsBE.init();
	set01JetsOO.init();
	set01JetsOE.init();
	set01JetsEE.init();
	set01JetsTightBB.init();
	set01JetsTightBO.init();
	set01JetsTightBE.init();
	set01JetsTightOO.init();
	set01JetsTightOE.init();
	set01JetsTightEE.init();
	set01JetsLooseBB.init();
	set01JetsLooseBO.init();
	set01JetsLooseBE.init();
	set01JetsLooseOO.init();
	set01JetsLooseOE.init();
	set01JetsLooseEE.init();

	//	get the total events, etc...
	long long int numEventsWeighted = sampleinfo(__inputfilename);
    hEventWeights->Fill(0.5, numEventsWeighted);

	//	generate the MC Pileup histogram
	if (__genPUMC && __isMC)
		generatePUMC();

	Streamer streamer(__inputfilename, NTUPLEMAKER_NAME+"/Events");
	streamer.chainup();

    Muons *muons=NULL;
	Muons muons1;
	Muons muons2;
	Jets *jets=NULL;
	Vertices *vertices=NULL;
	Event *event=NULL;
	EventAuxiliary *aux=NULL;
	MET *met=NULL;
	streamer._chain->SetBranchAddress("Muons", &muons);
	streamer._chain->SetBranchAddress("Jets", &jets);
	streamer._chain->SetBranchAddress("Vertices", &vertices);
	streamer._chain->SetBranchAddress("Event", &event);
	streamer._chain->SetBranchAddress("EventAuxiliary", &aux);
	streamer._chain->SetBranchAddress("MET", &met);

	//	init the PU reweighter
	reweight::LumiReWeighting *weighter = NULL;
	if (__isMC)
	{
		std::cout << "mcPU=" << __puMCfilename << "  dataPU="
			<< __puDATAfilename << std::endl;
		TString mc_pileupfile = __puMCfilename.c_str();
		TString data_pileupfile = __puDATAfilename.c_str();
		weighter = new reweight::LumiReWeighting(mc_pileupfile.Data(), 
            data_pileupfile.Data(), "pileup", "pileup");
	}

    // init the Kalman Calibration Instance
    KalmanMuonCalibrator *kalman = NULL;
    std::cout << "Hello World" << std::endl;
    std::cout << __kalmanInput  << "  " << __kalman << std::endl;
    if (__kalman) {
        kalman = new KalmanMuonCalibrator(__kalmanInput);
    }
    RoccoR *rochester = NULL;
    std::cout << "Hello World" << std::endl;
    std::cout << __rochesterInput << std::endl;
    if (__rochester) 
        rochester = new RoccoR(__rochesterInput);

	//	Main Loop
	uint32_t numEntries = streamer._chain->GetEntries();
	for (uint32_t i=0; i<numEntries && __continueRunning; i++)
	{
        muons1.clear(); muons2.clear();
		streamer._chain->GetEntry(i);
		if (i%1000==0)
			std::cout << "### Event " << i << " / " << numEntries
				<< std::endl;

		float puweight = __isMC ? weighter->weight(aux->_nPU)*aux->_genWeight :
			1.;

		//
		//	Selections
		//
        // pass vertex
		if (!passVertex(vertices))
			continue;
        // pass HLT 
		if (!(aux->_hasHLTFired[0] || aux->_hasHLTFired[1]))
			continue;

        // apply the corrections
        for (Muons::iterator it=muons->begin(); it!=muons->end(); ++it) {
            if (__kalman) {
                double pt_corrected = kalman->getCorrectedPt(it->_pt, it->_eta, 
                    it->_phi, it->_charge);
                double pt_smeared = kalman->smear(pt_corrected, it->_eta);
                if (__isMC)
                    it->_pt = pt_smeared;
                else
                    it->_pt = pt_corrected;
            }

            if (__rochester) {
                double sf = 1.0;
                if (__isMC)
                    sf = rochester->kScaleAndSmearMC(it->_charge, 
                        it->_pt, it->_eta, it->_phi, it->_nTLs, gRandom->Rndm(),
                        gRandom->Rndm(), 0, 0);
                else 
                    sf = rochester->kScaleDT(it->_charge, it->_pt,
                        it->_eta, it->_phi, 0, 0);
                it->_pt*=sf;
            }
        }

        // select the muons that pass the initial cuts
        Muons selectedMuons;
        for (analysis::core::Muons::const_iterator it=muons->begin();
            it!=muons->end(); ++it)
            if (passMuon(*it))
                selectedMuons.push_back(*it);

        // should have at least 1 muon that matches HLT
        if (!passHLTMatch(selectedMuons))
            continue;

        // at least 2 selected muons, at least 1 muon with pt>=26GeV
        if (!passMuons(selectedMuons))
            continue;

        // select the dimuon pairs
        std::vector<std::pair<Muon, Muon> > muonPairs;
        for (analysis::core::Muons::const_iterator it=muons->begin();
            it!=muons->end(); ++it)
            for (analysis::core::Muons::const_iterator jt=(it+1);
                jt!=muons->end(); ++jt)
            {
                if (it->_charge!=jt->_charge)
                    muonPairs.push_back(std::make_pair(
                        it->_pt>jt->_pt ? *it : *jt, 
                        it->_pt>jt->_pt ? *jt : *it));
		    }

        if (muonPairs.size()==0)
            continue;
        else if (muonPairs.size()==1)
            categorize(jets, muonPairs[0].first, muonPairs[0].second,
                *met, *event);
        else {
            std::pair<Muon, Muon> candidate;
            candidate.first._pt = 0; candidate.second._pt = 0;
            for (std::vector<std::pair<Muon, Muon> >::const_iterator it=
            muonPairs.begin(); it!=muonPairs.end(); ++it)
                if (it->first._pt > candidate.first._pt)
                    candidate = (*it);
                else if (it->first._pt == candidate.first._pt) 
                    if (it->second._pt > candidate.second._pt)
                        candidate = (*it);
            categorize(jets, candidate.first, candidate.second,
                *met, *event);
        }
	}

	outroot->Write();
	outroot->Close();

	return;
}

void sigHandler(int sig)
{
	cout << "### Signal: " << sig << " caughter. Exiting..." << endl;
	__continueRunning = false;
}

void printCuts()
{
    std::cout << "Cuts:" << std::endl
        << "_muonMatchedPt = " << __muonMatchedPt << std::endl
        << "_muonMatchedEta = " << __muonMatchedEta << std::endl
        << "_muonPt = " << __muonPt << std::endl
        << "_muonEta = " << __muonEta << std::endl
        << "_muonIso = " << __muonIso << std::endl
        << "_leadJetPt = " << __leadJetPt << std::endl
        << "_subleadJetPt = " << __subleadJetPt << std::endl
        << "_metPt = " << __metPt << std::endl
        << "_dijetMass_VBFTight = " << __dijetMass_VBFTight << std::endl
        << "_dijetdEta_VBFTight = " << __dijetdEta_VBFTight << std::endl
        << "_dijetMass_ggFTight = " << __dijetMass_ggFTight << std::endl
        << "_dimuonPt_ggFTight = " << __dimuonPt_ggFTight << std::endl
        << "_dimuonPt_01JetsTight = " << __dimuonPt_01JetsTight << std::endl;
}

int main(int argc, char** argv)
{
	/*
	 *	Register signals
	 */
	signal(SIGABRT, &sigHandler);
	signal(SIGTERM, &sigHandler);
	signal(SIGINT, &sigHandler);

	std::string none = "None";
	bool genPUMC = false;

	/*
	 *	Pare Options
	 */
	po::options_description desc("Allowed Program Options");
	desc.add_options()
		("help", "produce help messages")
		("input", po::value<std::string>(), "a file specifying all the ROOT files to process")
		("isMC", po::value<bool>(), "type of data: DATA vs MC")
		("output", po::value<std::string>(), "output file name")
		("genPUMC", po::value<bool>(&genPUMC)->default_value(false), "true if should generate the MC PU file")
		("puMC", po::value<std::string>()->default_value("None"), "MC PU Reweight file")
		("puDATA", po::value<std::string>()->default_value("None"), "DATA PU Reweight file")
        ("kalman", po::value<std::string>()->default_value(none), "Path to a file with Kalman correcvtions")
        ("rochester", po::value<std::string>()->default_value(none), "Path to a file with Rochester Corrections")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help") || argc<2)
	{
		std::cout << desc << std::endl;
		return 1;
	}

	//	Assign globals
	__inputfilename = vm["input"].as<std::string>();
	__isMC = vm["isMC"].as<bool>();
	__outputfilename = vm["output"].as<std::string>();
	__genPUMC = vm["genPUMC"].as<bool>();
	__puMCfilename = vm["puMC"].as<std::string>();
	__puDATAfilename = vm["puDATA"].as<std::string>();
    __kalmanInput = vm["kalman"].as<std::string>();
    __rochesterInput = vm["rochester"].as<std::string>();
    std::cout << __kalmanInput << std::endl;
    std::cout << __rochesterInput << std::endl;
    std::cout << none << std::endl;
    if (__kalmanInput != none)
        __kalman = true;
    if (__rochesterInput != none)
        __rochester = true;
    printCuts();

	//	start processing
	process();
	return 0;
}

#endif
