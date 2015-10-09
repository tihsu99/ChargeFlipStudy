#include <TFile.h>
#include <TROOT.h>
#include <TH1.h>
#include <TH2.h>
#include <TSystem.h>

#include "TopLJets2015/TopAnalysis/interface/MiniEvent.h"
#include "TopLJets2015/TopAnalysis/interface/ReadTree.h"

#include "CondFormats/BTauObjects/interface/BTagCalibration.h"
#include "CondFormats/BTauObjects/interface/BTagCalibrationReader.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"

#include <vector>
#include <iostream>
#include <algorithm>

#include "TMath.h"

using namespace std;

Int_t getSecVtxBinToFill(Float_t firstVtxMass,Float_t secondVtxMass,Int_t nJets,Int_t nsvtxMassBins,Float_t minSvtxMass,Float_t maxSvtxMass)
{
  Int_t nJetsBin(nJets>4 ? 4 : nJets);
  Int_t nVtx(0); nVtx+=(firstVtxMass>0); nVtx += (secondVtxMass>0); 
  Int_t secvtxBinToFill(0);
  if(nVtx==1) secvtxBinToFill=(Int_t)nsvtxMassBins*(TMath::Max(TMath::Min(firstVtxMass,maxSvtxMass),minSvtxMass)-minSvtxMass)/maxSvtxMass+1;
  if(nVtx==2) secvtxBinToFill=(Int_t)nsvtxMassBins*(TMath::Max(TMath::Min(secondVtxMass,maxSvtxMass),minSvtxMass)-minSvtxMass)/maxSvtxMass+nsvtxMassBins+1;
  secvtxBinToFill += (nJetsBin-1)*(2*nsvtxMassBins+1);
  return secvtxBinToFill;
}

Int_t getBtagCatBinToFill(Int_t nBtags,Int_t nJets)
{
  Int_t nJetsBin(nJets>4 ? 4 : nJets);

  Int_t binToFill(nBtags>=2?2:nBtags);
  binToFill+=3*(nJetsBin-1);
  return binToFill;
}

void ReadTree(TString filename,
	      TString outname,
	      Int_t channelSelection, 
	      Int_t chargeSelection, 
	      Float_t norm, 
	      Bool_t isTTbar,
	      FlavourSplitting flavourSplitting,
	      GenWeightMode genWgtMode)
{
  //book histograms
  std::map<TString, TH1 *> allPlots;

  for(Int_t ij=1; ij<=4; ij++)
    {
      TString tag(Form("%dj",ij));
      allPlots["lpt_"+tag]  = new TH1F("lpt_"+tag,";Transverse momentum [GeV];Events" ,20,0.,300.);
      allPlots["leta_"+tag] = new TH1F("leta_"+tag,";Pseudo-rapidity;Events" ,12,0.,3.);
      allPlots["jpt_"+tag]  = new TH1F("jpt_"+tag,";Transverse momentum [GeV];Events" ,20,0.,300.);
      allPlots["jeta_"+tag] = new TH1F("jeta_"+tag,";Pseudo-rapidity;Events" ,12,0.,3.);
      allPlots["ht_"+tag]   = new TH1F("ht_"+tag,";H_{T} [GeV];Events",40,0,800);
      allPlots["csv_"+tag]  = new TH1F("csv_"+tag,";CSV discriminator;Events",100,0,1.0);
      allPlots["nvtx_"+tag] = new TH1F("nvtx_"+tag,";Vertex multiplicity;Events" ,50,0.,50.);
      allPlots["met_"+tag]  = new TH1F("metpt_"+tag,";Missing transverse energy [GeV];Events" ,20,0.,300.);
      allPlots["metphi_"+tag] = new TH1F("metphi_" + tag,";MET #phi [rad];Events" ,50,-3.2,3.2);
      allPlots["mt_"+tag] = new TH1F("mt_"+tag,";Transverse Mass [GeV];Events" ,100,0.,200.);
      allPlots["secvtxmass_"+tag] = new TH1F("secvtxmass_"+tag,";SecVtx Mass [GeV];Events" ,10,0.,5.);
      allPlots["secvtx3dsig_"+tag] = new TH1F("secvtx3dsig_"+tag,";SecVtx 3D sig;Events" ,10,0.,100.);
    }

  Int_t nsvtxMassBins=allPlots["secvtxmass_4j"]->GetXaxis()->GetNbins(); 
  Float_t maxSvtxMass=allPlots["secvtxmass_4j"]->GetXaxis()->GetXmax();
  Float_t minSvtxMass=allPlots["secvtxmass_4j"]->GetXaxis()->GetXmin();
  allPlots["catcountSecVtx"] = new TH1F("catcountSecVtx",";Category;Events",4*(2*nsvtxMassBins+1),0.,4*(2*nsvtxMassBins+1));
  allPlots["catcount"] = new TH1F("catcount",";Category;Events" ,12,0.,12.);
  allPlots["catcount"]->GetXaxis()->SetBinLabel(1, "1j,=0b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(2, "1j,=1b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(3, "");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(4, "2j,=0b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(5, "2j,=1b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(6, "2j,#geq2b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(7, "3j,=0b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(8, "3j,=1b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(9, "3j,#geq2b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(10,"4j,=0b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(11,"4j,=1b");
  allPlots["catcount"]->GetXaxis()->SetBinLabel(12,"4j,#geq2b");
  TString systs[]={"qcdScaleDown","qcdScaleUp","hdampScaleDown","hdampScaleUp","jesDown","jesUp","jerDown","jerUp","beffDown","beffUp","mistagDown","mistagUp"};
  for(size_t i=0; i<sizeof(systs)/sizeof(TString); i++)
    {
      allPlots["catcount_"+systs[i]]       = (TH1 *)allPlots["catcount"]->Clone("catcount_"+systs[i]);
      allPlots["catcountSecVtx_"+systs[i]] = (TH1 *)allPlots["catcountSecVtx"]->Clone("catcountSecVtx_"+systs[i]);
    }

  for (auto& it : allPlots) { it.second->Sumw2(); it.second->SetDirectory(0); }

  //jet uncertainty parameterization
  TString jecUncUrl("${CMSSW_BASE}/src/TopLJets2015/TopAnalysis/data/Summer15_50nsV5_DATA_Uncertainty_AK4PFchs.txt");
  gSystem->ExpandPathName(jecUncUrl);
  JetCorrectionUncertainty *jecUnc = new JetCorrectionUncertainty(jecUncUrl.Data());
  
  // setup calibration readers
  TString btagUncUrl("${CMSSW_BASE}/src/TopLJets2015/TopAnalysis/data/CSVv2.csv");
  gSystem->ExpandPathName(btagUncUrl);
  /*
  BTagCalibration calib("csvv2", btagUncUrl.Data());
  BTagCalibrationReader btvreader(&calib,               // calibration instance
				  BTagEntry::OP_LOOSE,  // operating point
				  "comb",               // measurement type
				  "central");           // systematics type
  BTagCalibrationReader btvreader_up(&calib, BTagEntry::OP_LOOSE, "comb", "up");  // sys up
  BTagCalibrationReader btvreader_do(&calib, BTagEntry::OP_LOOSE, "comb", "down");  // sys down
  */

  //read tree from file
  MiniEvent_t ev;
  TFile *f = TFile::Open(filename);
  TTree *t = (TTree*)f->Get("analysis/data");
  attachToMiniEventTree(t,ev);

  //loop over events
  Int_t nentries(t->GetEntriesFast());
  cout << "...producing " << outname << " from " << nentries << " events" << endl;
  for (Int_t i=0;i<t->GetEntriesFast();i++)
    {
      t->GetEntry(i);
      
      //select according to the lepton id/charge
      if(channelSelection!=0 && abs(ev.l_id)!=abs(channelSelection)) continue;
      if(chargeSelection!=0 &&  ev.l_charge!=chargeSelection) continue;

      //apply trigger requirement
      if(abs(ev.l_id) == 13 && ((ev.muTrigger>>0)&0x1)==0) continue;
      if(abs(ev.l_id) == 11 && ((ev.elTrigger>>0)&0x1)==0) continue;
      
      //select jets
      Int_t nudsgJets(0),ncJets(0), nbJets(0);
      uint32_t nJets(0), nJetsJESLo(0),nJetsJESHi(0), nJetsJERLo(0), nJetsJERHi(0);
      uint32_t nBtags(0), nBtagsBeffLo(0), nBtagsBeffHi(0), nBtagsMistagLo(0),nBtagsMistagHi(0);
      Float_t htsum(0),firstVtxMass(0.),firstVtxLxySig(-9999.),secondVtxMass(0.),secondVtxLxySig(-9999.);
      std::vector<int> selJetsIdx;
      for (int k=0; k<ev.nj;k++)
	{
	  //check kinematics
	  float jet_pt  = ev.j_pt[k], jet_eta = ev.j_eta[k], csv = ev.j_csv[k];    
	  if(fabs(jet_eta) > 2.5) continue;
	  if(jet_pt > 30)
	    {
	      nJets ++;
	      selJetsIdx.push_back(k);
	      htsum += jet_pt;
	      bool isBTagged(csv>0.890);
	      if(ev.j_vtx3DSig[k]>firstVtxLxySig)
		{
		  secondVtxLxySig=firstVtxLxySig;
		  secondVtxMass=firstVtxMass;
		  firstVtxLxySig=ev.j_vtx3DSig[k];
		  firstVtxMass=ev.j_vtxmass[k];
		}
	      else if(ev.j_vtx3DSig[k]>secondVtxLxySig)
		{
		  secondVtxLxySig=ev.j_vtx3DSig[k];
		  secondVtxMass=ev.j_vtxmass[k];
		}

	      //BTagEntry::JetFlavor btagFlav( BTagEntry::FLAV_UDSG  );
	      if(abs(ev.j_hadflav[k])==4)      { /*btagFlav=BTagEntry::FLAV_C;*/ ncJets++; }
	      else if(abs(ev.j_hadflav[k])==5) { /*btagFlav=BTagEntry::FLAV_B;*/ nbJets++; }
	      else nudsgJets++;


	      //readout the b-tagging scale factors for this jet
	      /*
		BTagEntry::JetFlavor btagFlav( BTagEntry::FLAV_UDSG  );
		if(abs(ev.j_hadflav[k])==4) btagFlav=BTagEntry::FLAV_C;
		if(abs(ev.j_hadflav[k])==5) btagFlav=BTagEntry::FLAV_B;	
		float jetBtagSF(1.0), jetBtagSFUp(1.0), jetBtagSFDown(1.0);
		if (jet_pt < 1000.) {
		jetBtagSF = btvreader.eval(btagFlav, jet_eta, jet_pt);
		jetBtagSFUp = btvreader_up.eval(btagFlav, jet_eta, jet_pt);
		jetBtagSFDown = btvreader_do.eval(btagFlav, jet_eta, jet_pt);
		}
	      */
	
	      nBtags += isBTagged;
	      nBtagsBeffLo += isBTagged;
	      nBtagsBeffHi += isBTagged;
	      nBtagsMistagLo += isBTagged;
	      nBtagsMistagHi += isBTagged;
	    }

	  //jet energy scale variations
	  jecUnc->setJetEta(fabs(jet_eta));
	  jecUnc->setJetPt(jet_pt);
	  double unc = jecUnc->getUncertainty(true);    
	  if((jet_pt)*(1+unc)>30) nJetsJESHi++;
	  if((jet_pt)*(1-unc)>30) nJetsJESLo++;
	  
	  //jet energy resolution
	  float JERCor_UP(1.0),JERCor_DOWN(1.0);
	  
	  //NEEDS GEN JETS IN THE TREE!!!!!!!!!!
	  //float genJet_pt(ev.genj_pt[k]);
	  //if(genJet_pt>0)
	  //{
	  //	JERCor = getJERfactor(jet_pt, jet_eta, genJet_pt);
	  //	JERCor_UP = getJERfactor_up(jet_pt, jet_eta, genJet_pt);
	  //  JERCor_DOWN = getJERfactor_down(jet_pt, jet_eta, genJet_pt);
	  // }
	  if(JERCor_UP*jet_pt>30) nJetsJERHi++;
	  if(JERCor_DOWN*jet_pt>30) nJetsJERLo++;
	}

      //check if flavour splitting was required
      if(flavourSplitting!=FlavourSplitting::NOFLAVOURSPLITTING)
      {
      	if(flavourSplitting==FlavourSplitting::BSPLITTING)         { if(nbJets==0)    continue; }
      	else if(flavourSplitting==FlavourSplitting::CSPLITTING)    { if(ncJets==0 || nbJets!=0)    continue; }
      	else if(flavourSplitting==FlavourSplitting::UDSGSPLITTING) { if(nudsgJets==0 || ncJets!=0 || nbJets!=0) continue; }
      }
	
      //generator level weights to apply
      float wgt(norm),wgtQCDScaleLo(norm),wgtQCDScaleHi(norm),wgthdampScaleLo(norm),wgthdampScaleHi(norm);
      if(genWgtMode==FULLWEIGHT) wgt *= ev.ttbar_w[0];
      if(genWgtMode==ONLYSIGN)   wgt *= (ev.ttbar_w[0]>0 ? +1.0 : -1.0)*norm;
      if(isTTbar)
	{
	  wgtQCDScaleLo   = wgt*ev.ttbar_w[9]/ev.ttbar_w[0];
	  wgtQCDScaleHi   = wgt*ev.ttbar_w[5]/ev.ttbar_w[0];
	  wgthdampScaleLo = wgt*ev.ttbar_w[ev.ttbar_nw-17]/ev.ttbar_w[0];
	  wgthdampScaleHi = wgt*ev.ttbar_w[ev.ttbar_nw-9]/ev.ttbar_w[0];
	}
      
      //main histogram for xsec extraction
      int binToFill=getBtagCatBinToFill(nBtags,nJets);
      int secvtxBinToFill=getSecVtxBinToFill(firstVtxMass,secondVtxMass,nJets,nsvtxMassBins, minSvtxMass,maxSvtxMass);
      if(nJets>=1)
	{
	  allPlots["catcountSecVtx"]->Fill(secvtxBinToFill,wgt);
	  allPlots["catcountSecVtx_qcdScaleDown"]->Fill(secvtxBinToFill,wgtQCDScaleLo);
	  allPlots["catcountSecVtx_qcdScaleUp"]->Fill(secvtxBinToFill,wgtQCDScaleHi);
	  allPlots["catcountSecVtx_hdampScaleDown"]->Fill(secvtxBinToFill,wgthdampScaleLo);
	  allPlots["catcountSecVtx_hdampScaleUp"]->Fill(secvtxBinToFill,wgthdampScaleHi);
	  allPlots["catcount"]->Fill(binToFill,wgt);
	  allPlots["catcount_qcdScaleDown"]->Fill(binToFill,wgtQCDScaleLo);
	  allPlots["catcount_qcdScaleUp"]->Fill(binToFill,wgtQCDScaleHi);
	  allPlots["catcount_hdampScaleDown"]->Fill(binToFill,wgthdampScaleLo);
	  allPlots["catcount_hdampScaleUp"]->Fill(binToFill,wgthdampScaleHi);
	}

      binToFill=getBtagCatBinToFill(nBtags,nJetsJESHi);
      secvtxBinToFill=getSecVtxBinToFill(firstVtxMass,secondVtxMass,nJetsJESHi,nsvtxMassBins, minSvtxMass,maxSvtxMass);
      if(nJetsJESHi>=1)
	{
	  allPlots["catcount_jesUp"]->Fill(binToFill,wgt);
	  allPlots["catcountSecVtx_jesUp"]->Fill(secvtxBinToFill,wgt);
	}

      binToFill=getBtagCatBinToFill(nBtags,nJetsJESLo);
      secvtxBinToFill=getSecVtxBinToFill(firstVtxMass,secondVtxMass,nJetsJESLo,nsvtxMassBins, minSvtxMass,maxSvtxMass);
      if(nJetsJESLo>=1) 
	{
	  allPlots["catcount_jesDown"]->Fill(binToFill,wgt);
	  allPlots["catcountSecVtx_jesDown"]->Fill(secvtxBinToFill,wgt);
	}

      binToFill=getBtagCatBinToFill(nBtags,nJetsJERHi);
      secvtxBinToFill=getSecVtxBinToFill(firstVtxMass,secondVtxMass,nJetsJERHi,nsvtxMassBins, minSvtxMass,maxSvtxMass);
      if(nJetsJERHi>=1) 
	{
	  allPlots["catcount_jerUp"]->Fill(binToFill,wgt);
	  allPlots["catcountSecVtx_jerUp"]->Fill(secvtxBinToFill,wgt);
	}

      binToFill=getBtagCatBinToFill(nBtags,nJetsJERLo);
      secvtxBinToFill=getSecVtxBinToFill(firstVtxMass,secondVtxMass,nJetsJERLo,nsvtxMassBins, minSvtxMass,maxSvtxMass);
      if(nJetsJERHi>=1) 
	{
	  allPlots["catcount_jerDown"]->Fill(binToFill,wgt);
	  allPlots["catcountSecVtx_jerDown"]->Fill(secvtxBinToFill,wgt);
	}
  
      binToFill=getBtagCatBinToFill(nBtagsBeffLo,nJets);
      if(nJets>=1) allPlots["catcount_beffDown"]->Fill(binToFill,wgt); 

      binToFill=getBtagCatBinToFill(nBtagsBeffHi,nJets);
      if(nJets>=1) allPlots["catcount_beffUp"]->Fill(binToFill,wgt); 
      
      binToFill=getBtagCatBinToFill(nBtagsMistagLo,nJets);
      if(nJets>=1) allPlots["catcount_mistagDown"]->Fill(binToFill,wgt); 

      binToFill=getBtagCatBinToFill(nBtagsMistagHi,nJets);
      if(nJets>=1) allPlots["catcount_mistagUp"]->Fill(binToFill,wgt); 

      //control histograms for the nominal selection only
      if(nJets<1) continue;
      
      TString tag("");
      if(nJets>4) tag="4";
      else tag += nJets;
      tag+="j";
      allPlots["lpt_"+tag]->Fill(ev.l_pt,wgt);
      allPlots["leta_"+tag]->Fill(ev.l_eta,wgt);
      allPlots["jpt_"+tag]->Fill(ev.j_pt[ selJetsIdx[0] ],wgt);
      allPlots["jeta_"+tag]->Fill(fabs(ev.j_eta[ selJetsIdx[0] ]),wgt);
      allPlots["csv_"+tag]->Fill(ev.j_csv[ selJetsIdx[0] ],wgt);
      allPlots["ht_"+tag]->Fill(htsum,wgt);
      allPlots["nvtx_"+tag]->Fill(ev.nvtx,wgt);
      allPlots["met_"+tag]->Fill(ev.met_pt,wgt);
      allPlots["metphi_"+tag]->Fill(ev.met_phi,wgt);
      allPlots["mt_"+tag]->Fill(ev.mt,wgt);
      if(firstVtxMass>0) 
	{
	  allPlots["secvtxmass_"+tag]->Fill(firstVtxMass,wgt);
	  allPlots["secvtx3dsig_"+tag]->Fill(firstVtxLxySig,wgt);
	}
    }

  //close input file
  f->Close();

  //save histos to file  
  TString selPrefix("");  
  if(flavourSplitting!=NOFLAVOURSPLITTING) selPrefix=Form("%d_",flavourSplitting);
  TString baseName=gSystem->BaseName(outname); 
  TString dirName=gSystem->DirName(outname);
  TFile *fOut=TFile::Open(dirName+"/"+selPrefix+baseName,"RECREATE");
  for (auto& it : allPlots)  { it.second->SetDirectory(fOut); it.second->Write(); }
  fOut->Close();
}
