#include "Util.h"

/// function used to draw an error band around a RooAbsPdf -> used to draw the band after each fit around the pdf
void draw_error_band( RooAbsData *rdata,  RooAbsPdf *rpdf,  RooRealVar *rrv_number_events , RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const std::string & opt, const int & number_point, const int & number_errorband){

  std::cout<<" <<<<<<<<<<<<<<<< draw error band 1 <<<<<<<<<<<<<<<< "<<std::endl;
  TRandom3 rand(1234);
  rand.SetSeed(0);
  /// get the observables of the pdf --> mj or mlvj depends on which bands you are drawing
  RooArgSet* argset_obs = rpdf->getObservables(rdata);
  /// create an iterator on the observables
  TIterator *par=argset_obs->createIterator();
  par->Reset();  
  /// extract the RooRealVar --> in our case just one obs
  RooRealVar *rrv_x=(RooRealVar*)par->Next();
  rrv_x->Print();
  /// Get the pdf pramters 
  rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
  rrv_number_events->Print();
  /// Define min and max x range
  Double_t x_min = rrv_x->getMin();
  Double_t x_max = rrv_x->getMax();
  /// define the step to sample the function 
  Double_t delta_x = (x_max-x_min)/number_point;
  /// Original Bin width
  Double_t width_x = mplot->getFitRangeBinW();

  /// number of events  
  Double_t number_events_mean = rrv_number_events->getVal();
  Double_t number_events_sigma= rrv_number_events->getError();

  /// Create a Graph for the central background prediction 
  TGraph *bkgpred = new TGraph(number_point+1);
  for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rrv_number_events->getVal()*rpdf->getVal(*rrv_x)*width_x);
  }
  
  bkgpred->SetLineWidth(2);
  bkgpred->SetLineColor(kcolor);

  /// Set of parameters
  RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
       
  /// Build a envelope using number_errorband toys
  TGraph* syst[number_errorband];
  for(int j=0;j<number_errorband;j++){
   syst[j] = new TGraph(number_point+1);
   /// paramters value are randomized using rfres and this can be done also if they are not decorrelate
   RooArgList par_tmp = rfres->randomizePars();
   *par_pdf = par_tmp;
   Double_t number_events_tmp = rand.Gaus(number_events_mean,number_events_sigma); /// new poisson random number of events
   for(int i =0 ; i<=number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	syst[j]->SetPoint(i,x_min+delta_x*i,number_events_tmp*rpdf->getVal(*rrv_x)*width_x);
   }
  }

  /// re-take the initial parameters
  RooArgList par_tmp = rfres->floatParsFinal();
  *par_pdf = par_tmp;

  // build the uncertainty band at 68% CL 
  std::vector<double> val;
  val.resize(number_errorband);

  // Try to build and find max and minimum for each point --> not the curve but the value to do a real envelope -> take one sigma interval
  TGraph *ap = new TGraph(number_point+1);
  TGraph *am = new TGraph(number_point+1);
  ap->SetName("error_up");
  am->SetName("error_dn");

  TGraphAsymmErrors* errorband = new TGraphAsymmErrors(number_point+1);
  errorband->SetName("errorband");

  for(int i =0 ; i<= number_point ; i++){
   for(int j=0;j<number_errorband;j++){ 
    val[j]=(syst[j])->GetY()[i];
   }

   std::sort(val.begin(),val.end());

   ap->SetPoint(i,x_min+delta_x*i,val[Int_t(0.16*number_errorband)]);
   am->SetPoint(i,x_min+delta_x*i,val[Int_t(0.84*number_errorband)]);

   errorband->SetPoint(i,x_min+delta_x*i,bkgpred->GetY()[i] );
   double errYLow = val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i];
   double errYHi  = bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)];
   errorband->SetPointError(i, 0.,0.,errYLow , errYHi);

  }

  ap->SetLineWidth(2);
  ap->SetLineColor(kcolor);
  am->SetLineWidth(2);
  am->SetLineColor(kcolor);
  errorband->SetFillColor(kBlack);
  errorband->SetFillStyle(3013);

  if( TString(opt).Contains("F") ) mplot->addObject(errorband,"E3");             
  if( TString(opt).Contains("L") ){ 
      mplot->addObject(am); 
      mplot->addObject(ap); 
  }

  return ;
}

/// Draw error band giving directly the extended Pdf
void draw_error_band_extendPdf( RooAbsData *rdata,  RooAbsPdf* rpdf, RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const std::string & opt, const int & number_point,  const int & number_errorband){

 TRandom3 rand(1234);
 rand.SetSeed(0);
 std::cout<<" <<<<<<<<<<<<<<<< draw error band extended Pdf <<<<<<<<<<<<<<<< "<<std::endl;
        
 /// Take the observable for the pdf
 RooArgSet* argset_obs = rpdf->getObservables(rdata);
 TIterator *par = argset_obs->createIterator();
 par->Reset();
 RooRealVar *rrv_x = (RooRealVar*)par->Next();
 rrv_x->Print();

 /// Define the sampling
 Double_t x_min = rrv_x->getMin();
 Double_t x_max = rrv_x->getMax();
 Double_t delta_x = (x_max-x_min)/number_point;
 Double_t width_x = mplot->getFitRangeBinW();
 std::cout<<"delta_x: " << delta_x << std::endl;
 std::cout<<"width_x: " << width_x << std::endl;
 std::cout<<"number_point: " << number_point << std::endl;
 rpdf->Print("v");
        
 /// Central value for the bkg prediction 
 TGraph *bkgpred = new TGraph(number_point+1);
 TGraph* error_curve[3];
 for(int k=0;k<3;k++){
   error_curve[k] = new TGraph(number_point+1);
 }
 for(int i =0 ; i <= number_point ; i++){
   rrv_x->setVal(x_min+delta_x*i);
   error_curve[2]->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
 }

 RooAbsPdf* rpdf_copy = rpdf;

 double x0,y0;
 for(int i =0 ; i <= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
//	rpdf->Print("v");
	bkgpred->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
//	std::cout<<"PDF value: " << rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x << std::endl;

 }
 bkgpred->SetLineWidth(2);
 bkgpred->SetLineColor(kcolor);


// Take the parameters
 RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x));
 std::cout<<"WHERE AM I" << std::endl;
 par_pdf->Print("v");
 
// Make the envelope
 TGraph* syst[number_errorband];
 
 for(int j=0;j<number_errorband;j++){
        syst[j] = new TGraph(number_point+1);
	RooArgList par_tmp = rfres->randomizePars();
	*par_pdf = par_tmp;
	par_pdf->Print("v");
	for(int i =0 ; i <= number_point ; i++){
		rrv_x->setVal(x_min+delta_x*i); 
		syst[j]->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
/*		std::cout<<"x_min+delta_x*i: " << x_min+delta_x*i << std::endl;
		std::cout<<"rpdf->expectedEvents(*rrv_x): " << rpdf->expectedEvents(*rrv_x) << std::endl;
		std::cout<<"rpdf->getVal(*rrv_x): " << rpdf->getVal(*rrv_x) << std::endl;
		std::cout<<"width_x: " << width_x << std::endl;
		std::cout<<"SetValue: " << rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x << std::endl;*/
	}
 }

 RooArgList par_tmp = rfres->floatParsFinal();
 *par_pdf = par_tmp;

 /// now extract the error curve at 2sigma 
 std::vector<double> val;
 val.resize(number_errorband);
// std::cout<<"number_errorband: " << number_errorband << std::endl;

 TGraph *ap = new TGraph(number_point+1);
 TGraph *am = new TGraph(number_point+1);
 ap->SetName("error_up");
 am->SetName("error_dn");

 TGraphAsymmErrors* errorband = new TGraphAsymmErrors(number_point+1);
 errorband->SetName("errorband");

 for(int i =0 ; i<= number_point ; i++){
    for(int j=0;j<number_errorband;j++){
      val[j] = (syst[j])->GetY()[i];
//      std::cout<<"val[j]: " << val[j] << std::endl;
    }
    std::sort(val.begin(),val.end());
    ap->SetPoint(i,x_min+delta_x*i,val[Int_t(0.84*number_errorband)]);
    am->SetPoint(i,x_min+delta_x*i,val[Int_t(0.16*number_errorband)]);
//    std::cout<<"number_errorband: " << number_errorband << std::endl;
//    std::cout<<"Int_t(0.84*number_errorband): " << Int_t(0.84*number_errorband) << std::endl;
//    std::cout<<"val[Int_t(0.84*number_errorband)]: " << val[Int_t(0.84*number_errorband)] << endl;
//    std::cout<<"val[Int_t(0.16*number_errorband)]: " << val[Int_t(0.16*number_errorband)] << endl;
    errorband->SetPoint(i,x_min+delta_x*i,bkgpred->GetY()[i]);
    errorband->SetPointError(i,0.,0.,bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)],val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);
 }

 ap->SetLineWidth(2);
 ap->SetLineColor(kcolor);
 ap->SetMarkerSize(0.);
 am->SetLineWidth(2);
 am->SetLineColor(kcolor);
 am->SetMarkerSize(0.);
 // errorband->SetFillColor(kBlack);
 // errorband->SetFillStyle(3013);

 if( TString(opt).Contains("F") ) mplot->addObject(errorband,"E3"); 
 else if( TString(opt).Contains("L") ){ mplot->addObject(am); mplot->addObject(ap); }
  
}


/// Variation of the previous method using a workspace -> used to draw the band for the final extrapolation -> take in input decorrelated parameters
std::vector<TGraphAsymmErrors*>* draw_error_band_ws(RooAbsData* rdata, RooAbsPdf *rpdf,  const std::string & xaxis_name ,  RooRealVar* rrv_number_events, RooArgList *paras,  RooWorkspace *ws, RooPlot *mplot, const int & kcolor, const std::string & opt, const int & number_point, const int & narrow_factor,const int & number_errorband, const int & ispull){

  TRandom3 rand(1234);
  rand.SetSeed(0);  

  /// get observables , pdf and number of events
  std::cout<<" <<<<<<<<<<<<<<<< draw error band 2 <<<<<<<<<<<<<<<< "<<std::endl;
  RooRealVar *rrv_x = ws->var(xaxis_name.c_str());
  rpdf->Print("v");
  rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
  rrv_number_events->Print();

  /// Define the sampling of the input pdf
  Double_t x_min = rrv_x->getMin(); 
  Double_t x_max = rrv_x->getMax();
  Double_t delta_x = (x_max-x_min)/(number_point);
  Double_t width_x = mplot->getFitRangeBinW();

  Double_t number_events_mean  = rrv_number_events->getVal();
  Double_t number_events_sigma = rrv_number_events->getError();

  std::cout<<" <<<<<<<<<< central bkg prediction "<<std::endl;
  /// TGraph for the central bkg prediction 
  TGraph *bkgpred=new TGraph(number_point+1);
  for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rrv_number_events->getVal()*rpdf->getVal(*rrv_x)*width_x);
  }
  
  bkgpred->SetLineWidth(2);
  bkgpred->SetLineColor(kcolor);

  /// Define the curve in each toy and fill them  not using the randomized par but vaying them by hand -> to be decorrelated
  std::cout<<" <<<<<<<<<< making the envelope "<<std::endl;
  for(int ipara = 0; ipara<paras->getSize();ipara++){
    std::cout<<" name "<<paras->at(ipara)->GetName()<<std::endl;
  }
         
  TGraph* syst[number_errorband];
  for(int j=0;j<number_errorband;j++){
	for(Int_t ipara=0;ipara<paras->getSize();ipara++){
          ws->var(paras->at(ipara)->GetName())->setConstant(0);           
          ws->var(paras->at(ipara)->GetName())->setVal( rand.Gaus(0.,ws->var(paras->at(ipara)->GetName())->getError()) );
	}

	Double_t number_events_tmp = rand.Gaus(number_events_mean,number_events_sigma);
	syst[j]=new TGraph(number_point+1);
	for(int i =0 ; i<=number_point ; i++){
		rrv_x->setVal(x_min+delta_x*i); 
		syst[j]->SetPoint(i,x_min+delta_x*i,number_events_tmp*rpdf->getVal(*rrv_x)*width_x);
	}
   }

   /// Now look for the envelop at 2sigma CL
   std::vector<double> val;
   val.resize(number_errorband);

   TGraphAsymmErrors* errorband = new TGraphAsymmErrors(number_point+1);
   errorband->SetName("errorband");

   TGraph *ap = new TGraph(number_point+1);
   TGraph *am = new TGraph(number_point+1);
   ap->SetName("error_up");
   am->SetName("error_dn");

   std::vector<TGraphAsymmErrors*>* errorband_pull = new std::vector<TGraphAsymmErrors*>();
   errorband_pull->push_back(new TGraphAsymmErrors(number_point+1));
   errorband_pull->push_back(new TGraphAsymmErrors(number_point+1));
   errorband_pull->push_back(new TGraphAsymmErrors(number_point+1)); 
   errorband_pull->at(0)->SetName("errorband_pull");
   errorband_pull->at(1)->SetName("errorband_pull_up");
   errorband_pull->at(2)->SetName("errorband_pull_dn");

   std::cout<<" making min and max "<<std::endl;
 
   TH1D* hdata = (TH1D*)rdata->createHistogram(rrv_x->GetName());
   hdata->Rebin(narrow_factor);
   const double alpha = 1 - 0.6827;
   
   for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
	std::sort(val.begin(),val.end());
	ap->SetPoint(i,x_min+delta_x*i,val[Int_t(0.84*number_errorband)]);
	am->SetPoint(i,x_min+delta_x*i,val[Int_t(0.16*number_errorband)]);
	errorband->SetPoint(i,x_min+delta_x*i,bkgpred->GetY()[i] );
	errorband->SetPointError(i, 0.,0., bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)],val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);

	double errYLow   = bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)];
	double errYHi    = val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i];                
        int N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*i));
        if (i == number_point) N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*(i-1)));

	double errData_dw =  (N==0) ? 0  : (ROOT::Math::gamma_quantile(alpha/2,N,1.));
	double errData_up =  ROOT::Math::gamma_quantile_c(alpha/2,N+1,1) ;         
        errData_dw = N - errData_dw ;
        errData_up = errData_up - N ;

        if( errData_dw < 1E-6) errData_dw = 1;
        if( errData_up < 1E-6) errData_up = 1;  

        if( ispull == 1){

	 errorband_pull->at(0)->SetPoint(i,x_min+delta_x*i+width_x/2,0.0);
	 errorband_pull->at(0)->SetPointError(i,width_x/2,width_x/2,errYLow/errData_dw, errYHi/errData_up);

         errorband_pull->at(1)->SetPoint(i,x_min+delta_x*i,0+errYHi/errData_up);
       	 errorband_pull->at(2)->SetPoint(i,x_min+delta_x*i,0-errYLow/errData_dw);

	}
        else{

       	 errorband_pull->at(0)->SetPoint(i,x_min+delta_x*i+width_x/2,1.0);
	 errorband_pull->at(0)->SetPointError(i,width_x/2,width_x/2,errYLow/bkgpred->GetY()[i],errYHi/bkgpred->GetY()[i]);

         errorband_pull->at(1)->SetPoint(i,x_min+delta_x*i,1.0+errYHi/bkgpred->GetY()[i]);
 	 errorband_pull->at(2)->SetPoint(i,x_min+delta_x*i,1.0-errYLow/bkgpred->GetY()[i]);
	}
    }

    ap->SetLineWidth(2);
    ap->SetLineColor(kcolor);
    am->SetLineWidth(2);
    am->SetLineColor(kcolor);
    errorband->SetFillColor(kBlack);
    errorband->SetFillStyle(3013);
    if( TString(opt).Contains("F") ) mplot->addObject(errorband,"E3");
    if( TString(opt).Contains("L") ){ mplot->addObject(am); mplot->addObject(ap); }
   
    errorband_pull->at(0)->SetFillColor(kYellow);
    errorband_pull->at(0)->SetLineColor(kYellow);
    errorband_pull->at(0)->SetLineWidth(2);
    errorband_pull->at(0)->SetFillStyle(3001);

    errorband_pull->at(1)->SetFillColor(kGray+1);
    errorband_pull->at(1)->SetLineColor(kGray+1);
    errorband_pull->at(1)->SetLineWidth(2);
    errorband_pull->at(1)->SetLineStyle(7);

    errorband_pull->at(2)->SetFillColor(kGray+1);
    errorband_pull->at(2)->SetLineColor(kGray+1);
    errorband_pull->at(2)->SetLineWidth(2);
    errorband_pull->at(2)->SetLineStyle(7);

    return errorband_pull ; 
}

/// Error band creator for a Decorellated Pdf starting from Workspace
void draw_error_band_Decor( std::string pdf_name, std::string xaxis_name,  RooArgList *paras,  RooWorkspace *ws,  RooRealVar *rrv_shape_scale , RooPlot *mplot, const int & kcolor, const std::string & opt, const int & number_point, const int & number_errorband){

 TRandom3 rand(1234);
 rand.SetSeed(0);
 std::cout<<" <<<<<<<<<<<<<<<< draw error band Decorrelated <<<<<<<<<<<<<<<< "<<std::endl;
 RooRealVar *rrv_x=ws->var(xaxis_name.c_str());
 rrv_x->Print();
 Double_t x_min=rrv_x->getMin();
 Double_t x_max=rrv_x->getMax();
 Double_t delta_x=(x_max-x_min)/number_point;
 Double_t width_x=mplot->getFitRangeBinW();

 /// factor to scale the normalized shape
 Double_t shape_scale = rrv_shape_scale->getVal();
 Double_t shape_scale_error = rrv_shape_scale->getError();

 /// Central bkg prediction 
 TGraph *bkgpred=new TGraph(number_point+1);
 for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,shape_scale*ws->pdf(pdf_name.c_str())->getVal());
 }
 bkgpred->SetLineWidth(2);
 bkgpred->SetLineColor(kcolor+3);

 /// error band -> each parameter can be randomly gaus generated
 TGraph* syst[number_errorband];
 for(int j=0;j<number_errorband;j++){
	for(Int_t ipara=0;ipara<paras->getSize();ipara++){
	  ws->var(paras->at(ipara)->GetName())->setVal(rand.Gaus(0.,1.));
	}

  /// Change the scaling value
  Double_t shape_scale_tmp = rand.Gaus(shape_scale,shape_scale_error);
  syst[j] = new TGraph(number_point+1);
  for(int i =0 ; i<=number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	syst[j]->SetPoint(i,x_min+delta_x*i,shape_scale_tmp*ws->pdf(pdf_name.c_str())->getVal()*width_x);
  }
 }

 std::vector<double> val;
 val.resize(number_errorband);

 TGraph *ap=new TGraph(number_point+1);
 TGraph *am=new TGraph(number_point+1);
 TGraphAsymmErrors* errorband=new TGraphAsymmErrors(number_point+1);
 ap->SetName("error_up");
 am->SetName("error_dn");
 errorband->SetName("errorband");

 for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
	std::sort(val.begin(),val.end());
	ap->SetPoint(i,x_min+delta_x*i,val[Int_t(0.84*number_errorband)]);
	am->SetPoint(i,x_min+delta_x*i,val[Int_t(0.16*number_errorband)]);
	errorband->SetPoint(i,x_min+delta_x*i,bkgpred->GetY()[i] );
	errorband->SetPointError(i,0.,0.,bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)],val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);
 }

 ap->SetLineWidth(2);
 ap->SetLineColor(kcolor);
 am->SetLineWidth(2);
 am->SetLineColor(kcolor);
 errorband->SetFillColor(kBlack);
 errorband->SetFillStyle(3013);
 errorband->SetName("Uncertainty");

 if( TString(opt).Contains("F") ){
  mplot->addObject(errorband,"E3"); 
  mplot->addObject(bkgpred);
 }
 if( TString(opt).Contains("L") ){
    mplot->addObject(am); mplot->addObject(ap); 
    mplot->addObject(bkgpred);
  }

  for(Int_t ipara=0;ipara<paras->getSize();ipara++){
    ws->var(paras->at(ipara)->GetName())->setVal(0.);
  }

} 

/// Just the shape and don't touch the normalization 
void draw_error_band_shape_Decor( std::string pdf_name, std::string xaxis_name,  RooArgList* paras,  RooWorkspace* ws, double sigma , RooPlot *mplot, const int & kcolor,const std::string & opt, const int & fillstyle, const std::string & uncertainty_title, const int & number_point, const int & number_errorband){

 TRandom3 rand(1234);
 rand.SetSeed(0); 
 std::cout<<" <<<<<<<<<<<<<<<< draw error band Decorrelated shape <<<<<<<<<<<<<<<< "<<std::endl;

 /// take the observable
 RooRealVar *rrv_x = ws->var(xaxis_name.c_str());
 rrv_x->Print();
 Double_t x_min = rrv_x->getMin();
 Double_t x_max = rrv_x->getMax();
 Double_t delta_x = (x_max-x_min)/number_point;
 Double_t width_x = mplot->getFitRangeBinW();
 
 /// bkg prediction central value
 TGraph *bkgpred=new TGraph(number_point+1);
 for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,ws->pdf(pdf_name.c_str())->getVal(*rrv_x)*width_x );
 }
 bkgpred->SetLineWidth(2);
 bkgpred->SetLineColor(kcolor+3);

 // make the envelope
 TGraph* syst[number_errorband];
 for(int j = 0; j < number_errorband;j++){
	for(Int_t ipara = 0;ipara<paras->getSize();ipara++){
	  ws->var(paras->at(ipara)->GetName())->setVal(rand.Gaus(0.,sigma)); // choose how many sigma on the parameters you wamt
	}
	syst[j]=new TGraph(number_point+1);
	for(int i =0 ; i <= number_point ; i++){
		rrv_x->setVal(x_min+delta_x*i); 
		syst[j]->SetPoint(i,x_min+delta_x*i,ws->pdf(pdf_name.c_str())->getVal(*rrv_x)*width_x);
	}
 }

 std::vector<double> val;
 val.resize(number_errorband);
 TGraph *ap=new TGraph(number_point+1);
 TGraph *am=new TGraph(number_point+1);
 TGraphAsymmErrors* errorband=new TGraphAsymmErrors(number_point+1);
 ap->SetName("error_up");
 am->SetName("error_dn");
 errorband->SetName("errorband");

 for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
	std::sort(val.begin(),val.end());
	ap->SetPoint(i, x_min+delta_x*i,val[Int_t(0.16*number_errorband)]);
	am->SetPoint(i, x_min+delta_x*i,val[Int_t(0.84*number_errorband)]);
	errorband->SetPoint(i,x_min+delta_x*i,bkgpred->GetY()[i]);
	errorband->SetPointError(i,0.,0.,bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)],val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);
 }

 ap->SetLineWidth(2);
 ap->SetLineColor(kcolor);
 am->SetLineWidth(2);
 am->SetLineColor(kcolor);
 errorband->SetFillStyle(fillstyle);
 errorband->SetFillColor(kcolor);
 errorband->SetName(Form("%s %g#sigma",uncertainty_title.c_str(),sigma));

 mplot->addObject(errorband,"E3");
 // if( TString(opt).Contains("F") ){ mplot->addObject(errorband,"E3"); }
 if( TString(opt).Contains("L") ){ mplot->addObject(am); mplot->addObject(ap); }

 for(Int_t ipara=0;ipara<paras->getSize();ipara++)
   ws->var(paras->at(ipara)->GetName())->setVal(0.);

} 


/// draw the error band on pull plots 
void draw_error_band_extendPdf_pull( RooAbsData *rdata, RooAbsPdf *rpdf, RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const int & number_point, const int & number_errorband){

 std::cout<<" <<<<<<<<<<<<<<<< draw error band extend Pdf Pull <<<<<<<<<<<<<<<< "<<std::endl;
 TRandom3 rand(1234);
 rand.SetSeed(0);
 /// get the observables of the pdf --> mj or mlvj depends on which bands you are drawing
 RooArgSet* argset_obs = rpdf->getObservables(rdata);
 /// create an iterator on the observables
 TIterator *par=argset_obs->createIterator();
 par->Reset();
 /// extract the RooRealVar --> in our case just one obs
 RooRealVar *rrv_x=(RooRealVar*)par->Next();
 rrv_x->Print();
 /// Get the pdf pramters 
 rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
 /// Define min and max x range
 Double_t x_min = rrv_x->getMin();
 Double_t x_max = rrv_x->getMax();
 /// define the step to sample the function 
 Double_t delta_x = (x_max-x_min)/number_point;
 /// Original Bin width
 Double_t width_x = mplot->getFitRangeBinW();

 /// Create a Graph for the central background prediction 
 TGraph *bkgpred = new TGraph(number_point+1);
 for(int i =0 ; i<= number_point ; i++){
 	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
 }
 bkgpred->SetLineWidth(2);
 bkgpred->SetLineColor(kcolor);

 /// Set of parameters
 RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
       
 /// Build a envelope using number_errorband toys
 TGraph* syst[number_errorband];
 for(int j=0;j<number_errorband;j++){
   syst[j] = new TGraph(number_point+1);
   /// paramters value are randomized using rfres and this can be done also if they are not decorrelate
   RooArgList par_tmp = rfres->randomizePars();
   *par_pdf = par_tmp;
   for(int i =0 ; i<=number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	syst[j]->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
   }
 }

 /// re-take the initial parameters
 RooArgList par_tmp = rfres->floatParsFinal();
 *par_pdf = par_tmp;
 // build the uncertainty band at 68% CL 
 std::vector<double> val;
 val.resize(number_errorband);

 // Try to build and find max and minimum for each point --> not the curve but the value to do a real envelope -> take one sigma interval
 TGraphAsymmErrors* errorband_pull = new TGraphAsymmErrors(number_point+1);
 errorband_pull->SetName("errorband_pull");

 TGraphAsymmErrors* errorband_pull_dn = new TGraphAsymmErrors(number_point+1);
 errorband_pull_dn->SetName("errorband_pull_dn");

 TGraphAsymmErrors* errorband_pull_up = new TGraphAsymmErrors(number_point+1);
 errorband_pull_up->SetName("errorband_pull_up");

 TH1D* hdata = (TH1D*)rdata->createHistogram(rrv_x->GetName());
 const double alpha = 1 - 0.6827;
 
 for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
	std::sort(val.begin(),val.end());

	double errYLow   = (bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)]);
	double errYHi    = (val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);
        int N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*i));

        if (i == number_point) N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*(i-1)));
	double errData_dw =  (N==0) ? 0  : (ROOT::Math::gamma_quantile(alpha/2,N,1.));
        errData_dw = N - errData_dw ;
	double errData_up =  ROOT::Math::gamma_quantile_c(alpha/2,N+1,1) ;         
        errData_up = errData_up-N ;

        if( errData_dw < 1E-6) errData_dw = 1;
        if( errData_up < 1E-6) errData_up = 1;  

      	errorband_pull->SetPoint(i, x_min+delta_x*i+width_x/2,  0.0 );
	errorband_pull->SetPointError(i,width_x/2,width_x/2,errYLow/errData_dw, errYHi/errData_up);         

      	errorband_pull_up->SetPoint(i,x_min+delta_x*i,1.0+errYHi/bkgpred->GetY()[i]);
      	errorband_pull_dn->SetPoint(i,x_min+delta_x*i,1.0-errYLow/bkgpred->GetY()[i]);
 }

 errorband_pull->SetFillColor(kYellow);
 errorband_pull->SetLineColor(kYellow);
 errorband_pull->SetLineWidth(2);
 errorband_pull->SetFillStyle(3001);

 errorband_pull_up->SetFillColor(kGray+1);
 errorband_pull_up->SetLineColor(kGray+1);
 errorband_pull_up->SetLineWidth(2);
 errorband_pull_up->SetLineStyle(7);

 errorband_pull_dn->SetFillColor(kGray+1);
 errorband_pull_dn->SetLineColor(kGray+1);
 errorband_pull_dn->SetLineWidth(2);
 errorband_pull_dn->SetLineStyle(7);
  
 mplot->addObject(errorband_pull,"E2");             
 mplot->addObject(errorband_pull_up,"L");             
 mplot->addObject(errorband_pull_dn,"L");             

}

void draw_error_band_extendPdf_ratio( RooAbsData *rdata, RooAbsPdf *rpdf, RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const int & number_point, const int & number_errorband){

 std::cout<<" <<<<<<<<<<<<<<<< draw error band extend Pdf Pull <<<<<<<<<<<<<<<< "<<std::endl;
 TRandom3 rand(1234);
 rand.SetSeed(0);

 /// get the observables of the pdf --> mj or mlvj depends on which bands you are drawing
 RooArgSet* argset_obs = rpdf->getObservables(rdata);
 /// create an iterator on the observables
 TIterator *par=argset_obs->createIterator();
 par->Reset();
 /// extract the RooRealVar --> in our case just one obs
 RooRealVar *rrv_x=(RooRealVar*)par->Next();
 rrv_x->Print();
 /// Get the pdf pramters
 rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
 /// Define min and max x range
 Double_t x_min = rrv_x->getMin();
 Double_t x_max = rrv_x->getMax();
 /// define the step to sample the function 
 Double_t delta_x = (x_max-x_min)/number_point;
 /// Original Bin width
 Double_t width_x = mplot->getFitRangeBinW();


 /// Create a Graph for the central background prediction 
 TGraph *bkgpred = new TGraph(number_point+1);
 for(int i =0 ; i<= number_point ; i++){
 	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
 }
 bkgpred->SetLineWidth(2);
 bkgpred->SetLineColor(kcolor);

 /// Set of parameters
 RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
       
 /// Build a envelope using number_errorband toys
 TGraph* syst[number_errorband];
 for(int j=0;j<number_errorband;j++){
   syst[j] = new TGraph(number_point+1);
   /// paramters value are randomized using rfres and this can be done also if they are not decorrelate
   RooArgList par_tmp = rfres->randomizePars();
   *par_pdf = par_tmp;
   for(int i =0 ; i<=number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	syst[j]->SetPoint(i,x_min+delta_x*i,rpdf->expectedEvents(*rrv_x)*rpdf->getVal(*rrv_x)*width_x);
   }
 }

 /// re-take the initial parameters
 RooArgList par_tmp = rfres->floatParsFinal();
 *par_pdf = par_tmp;
 
 // build the uncertainty band at 68% CL 
 std::vector<double> val;
 val.resize(number_errorband);

 // Try to build and find max and minimum for each point --> not the curve but the value to do a real envelope -> take one sigma interval
 TGraphAsymmErrors* errorband_ratio = new TGraphAsymmErrors(number_point+1);
 errorband_ratio->SetName("errorband_pull");
 TGraphAsymmErrors* errorband_ratio_dn = new TGraphAsymmErrors(number_point+1);
 errorband_ratio_dn->SetName("errorband_pull_dn");
 TGraphAsymmErrors* errorband_ratio_up = new TGraphAsymmErrors(number_point+1);
 errorband_ratio_up->SetName("errorband_pull_up");
  
 for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
        std::sort(val.begin(),val.end());
	double errYLow   = bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)];
	double errYHi    = val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i];
      	errorband_ratio->SetPoint(i,x_min+delta_x*i+width_x/2,1.0);
	errorband_ratio->SetPointError(i,width_x/2,width_x/2,errYLow/bkgpred->GetY()[i],errYHi/bkgpred->GetY()[i]);

      	errorband_ratio_up->SetPoint(i,x_min+delta_x*i,1.0 + errYHi/bkgpred->GetY()[i]);
      	errorband_ratio_dn->SetPoint(i,x_min+delta_x*i,1.0 - errYLow/bkgpred->GetY()[i]);

  }

  errorband_ratio->SetFillColor(kYellow);
  errorband_ratio->SetLineColor(kYellow);
  errorband_ratio->SetLineWidth(2);
  errorband_ratio->SetFillStyle(3001);

  errorband_ratio_up->SetFillColor(kGray+1);
  errorband_ratio_up->SetLineColor(kGray+1);
  errorband_ratio_up->SetLineWidth(2);
  errorband_ratio_up->SetLineStyle(7);

  errorband_ratio_dn->SetFillColor(kGray+1);
  errorband_ratio_dn->SetLineColor(kGray+1);
  errorband_ratio_dn->SetLineWidth(2);
  errorband_ratio_dn->SetLineStyle(7);
  
  mplot->addObject(errorband_ratio,"E2");             
  mplot->addObject(errorband_ratio_up,"L");             
  mplot->addObject(errorband_ratio_dn,"L");             
 
}


void draw_error_band_pull( RooAbsData *rdata,  RooAbsPdf *rpdf,  RooRealVar *rrv_number_events , RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const int & number_point, const int & number_errorband){

  std::cout<<" <<<<<<<<<<<<<<<< draw error band for the pull plot <<<<<<<<<<<<<<<< "<<std::endl;
  TRandom3 rand(1234);
  rand.SetSeed(0);

  /// get the observables of the pdf --> mj or mlvj depends on which bands you are drawing
  RooArgSet* argset_obs = rpdf->getObservables(rdata);
  /// create an iterator on the observables
  TIterator *par=argset_obs->createIterator();
  par->Reset();
  /// extract the RooRealVar --> in our case just one obs
  RooRealVar *rrv_x=(RooRealVar*)par->Next();
  rrv_x->Print();
  /// Get the pdf pramters 
  rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
  rrv_number_events->Print();
  /// Define min and max x range
  Double_t x_min = rrv_x->getMin();
  Double_t x_max = rrv_x->getMax();
  /// define the step to sample the function 
  Double_t delta_x = (x_max-x_min)/number_point;
  /// Original Bin width
  Double_t width_x = mplot->getFitRangeBinW();

  /// number of events  
  Double_t number_events_mean = rrv_number_events->getVal();
  Double_t number_events_sigma= rrv_number_events->getError();

  /// Create a Graph for the central background prediction 
  TGraph *bkgpred = new TGraph(number_point+1);
  for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rrv_number_events->getVal()*rpdf->getVal(*rrv_x)*width_x);
  }
  bkgpred->SetLineWidth(2);
  bkgpred->SetLineColor(kcolor);

  /// Set of parameters
  RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
       
  /// Build a envelope using number_errorband toys
  TGraph* syst[number_errorband];
  for(int j=0;j<number_errorband;j++){
	syst[j] = new TGraph(number_point+1);
        /// paramters value are randomized using rfres and this can be done also if they are not decorrelate
	RooArgList par_tmp = rfres->randomizePars();
	*par_pdf = par_tmp;
	Double_t number_events_tmp = rand.Gaus(number_events_mean,number_events_sigma); /// new poisson random number of events
	for(int i =0 ; i<=number_point ; i++){
		rrv_x->setVal(x_min+delta_x*i); 
		syst[j]->SetPoint(i,x_min+delta_x*i,number_events_tmp*rpdf->getVal(*rrv_x)*width_x);
	}
  }

  /// re-take the initial parameters
  RooArgList par_tmp = rfres->floatParsFinal();
  *par_pdf = par_tmp;

  // build the uncertainty band at 68% CL 
  std::vector<double> val;
  val.resize(number_errorband);

  // Try to build and find max and minimum for each point --> not the curve but the value to do a real envelope -> take one sigma interval
  TGraphAsymmErrors* errorband_pull = new TGraphAsymmErrors(number_point+1);
  errorband_pull->SetName("errorband_pull");
  TGraphAsymmErrors* errorband_pull_dn = new TGraphAsymmErrors(number_point+1);
  errorband_pull_dn->SetName("errorband_pull_dn");
  TGraphAsymmErrors* errorband_pull_up = new TGraphAsymmErrors(number_point+1);
  errorband_pull_up->SetName("errorband_pull_up");

  TH1D* hdata = (TH1D*)rdata->createHistogram(rrv_x->GetName());
  const double alpha = 1 - 0.6827;

  for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}
        std::sort(val.begin(),val.end());
	double errYLow   = (bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)]);
	double errYHi    = (val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);
        int N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*i));
        if (i == number_point) N = hdata->GetBinContent(hdata->FindBin(x_min+delta_x*(i-1)));
	double errData_dw =  (N==0) ? 0  : (ROOT::Math::gamma_quantile(alpha/2,N,1.));
        errData_dw = N - errData_dw ;
	double errData_up =  ROOT::Math::gamma_quantile_c(alpha/2,N+1,1) ;         
        errData_up = errData_up - N ;
        if( errData_dw < 1E-6) errData_dw = 1;
        if( errData_up < 1E-6) errData_up = 1;  
      	errorband_pull->SetPoint(i,x_min+delta_x*i+width_x/2,0.0);
	errorband_pull->SetPointError(i,width_x/2,width_x/2,errYLow/errData_dw,errYHi/errData_up);

      	errorband_pull_up->SetPoint(i,x_min+delta_x*i,0+errYHi/errData_up);
      	errorband_pull_dn->SetPoint(i,x_min+delta_x*i,0+errYLow/errData_dw);
  
  }

  errorband_pull->SetFillColor(kYellow);
  errorband_pull->SetLineColor(kYellow);
  errorband_pull->SetLineWidth(2);
  errorband_pull->SetFillStyle(3001);

  errorband_pull_up->SetFillColor(kGray+1);
  errorband_pull_up->SetLineColor(kGray+1);
  errorband_pull_up->SetLineWidth(2);
  errorband_pull_up->SetLineStyle(7);

  errorband_pull_dn->SetFillColor(kGray+1);
  errorband_pull_dn->SetLineColor(kGray+1);
  errorband_pull_dn->SetLineWidth(2);
  errorband_pull_dn->SetLineStyle(7);
  
  mplot->addObject(errorband_pull,"E2");             
  mplot->addObject(errorband_pull_up,"L");             
  mplot->addObject(errorband_pull_dn,"L");             

  return;
}

void draw_error_band_ratio(RooAbsData* rdata, RooAbsPdf *rpdf,  RooRealVar *rrv_number_events , RooFitResult *rfres, RooPlot *mplot, const int & kcolor, const int & number_point, const int & number_errorband){

  std::cout<<" <<<<<<<<<<<<<<<< draw error band for the pull plot <<<<<<<<<<<<<<<< "<<std::endl;
  TRandom3 rand(1234);
  rand.SetSeed(0);
  /// get the observables of the pdf --> mj or mlvj depends on which bands you are drawing
  RooArgSet* argset_obs = rpdf->getObservables(rdata);
  /// create an iterator on the observables
  TIterator *par=argset_obs->createIterator();
  par->Reset();
  /// extract the RooRealVar --> in our case just one obs
  RooRealVar *rrv_x=(RooRealVar*)par->Next();
  rrv_x->Print();
  /// Get the pdf pramters 
  rpdf->getParameters(RooArgSet(*rrv_x))->Print("v");
  rrv_number_events->Print();
  /// Define min and max x range
  Double_t x_min = rrv_x->getMin();
  Double_t x_max = rrv_x->getMax();
  /// define the step to sample the function 
  Double_t delta_x = (x_max-x_min)/number_point;
  /// Original Bin width
  Double_t width_x = mplot->getFitRangeBinW();

  /// number of events  
  Double_t number_events_mean = rrv_number_events->getVal();
  Double_t number_events_sigma= rrv_number_events->getError();
  /// Create a Graph for the central background prediction 

  TGraph *bkgpred = new TGraph(number_point+1);
  for(int i =0 ; i<= number_point ; i++){
	rrv_x->setVal(x_min+delta_x*i); 
	bkgpred->SetPoint(i,x_min+delta_x*i,rrv_number_events->getVal()*rpdf->getVal(*rrv_x)*width_x);
  }
  bkgpred->SetLineWidth(2);
  bkgpred->SetLineColor(kcolor);
  /// Set of parameters
  RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
       
  /// Build a envelope using number_errorband toys
  TGraph* syst[number_errorband];
  for(int j=0;j<number_errorband;j++){
	syst[j] = new TGraph(number_point+1);
        /// paramters value are randomized using rfres and this can be done also if they are not decorrelate
	RooArgList par_tmp = rfres->randomizePars();
	*par_pdf = par_tmp;
	Double_t number_events_tmp = rand.Gaus(number_events_mean,number_events_sigma); /// new poisson random number of events
	for(int i =0 ; i<=number_point ; i++){
		rrv_x->setVal(x_min+delta_x*i); 
		syst[j]->SetPoint(i,x_min+delta_x*i,number_events_tmp*rpdf->getVal(*rrv_x)*width_x);
	}
  }

  /// re-take the initial parameters
  RooArgList par_tmp = rfres->floatParsFinal();
  *par_pdf = par_tmp;

  // build the uncertainty band at 68% CL 
  std::vector<double> val;
  val.resize(number_errorband);

  // Try to build and find max and minimum for each point --> not the curve but the value to do a real envelope -> take one sigma interval
  TGraphAsymmErrors* errorband_ratio = new TGraphAsymmErrors(number_point+1);
  errorband_ratio->SetName("errorband_pull");
  TGraphAsymmErrors* errorband_ratio_dn = new TGraphAsymmErrors(number_point+1);
  errorband_ratio_dn->SetName("errorband_pull_dn");
  TGraphAsymmErrors* errorband_ratio_up = new TGraphAsymmErrors(number_point+1);
  errorband_ratio_up->SetName("errorband_pull_up");
  
  for(int i =0 ; i<= number_point ; i++){
	for(int j=0;j<number_errorband;j++){
		val[j]=(syst[j])->GetY()[i];
	}

        std::sort(val.begin(),val.end());

	double errYLow   = bkgpred->GetY()[i]-val[Int_t(0.16*number_errorband)];
	double errYHi    = (val[Int_t(0.84*number_errorband)]-bkgpred->GetY()[i]);

      	errorband_ratio->SetPoint(i,x_min+delta_x*i+width_x/2,1.0);
	errorband_ratio->SetPointError(i,width_x/2,width_x/2,errYLow/bkgpred->GetY()[i],errYHi/bkgpred->GetY()[i]);

      	errorband_ratio_up->SetPoint(i,x_min+delta_x*i,1.0+errYHi/bkgpred->GetY()[i]);
      	errorband_ratio_dn->SetPoint(i,x_min+delta_x*i,1.0+errYLow/bkgpred->GetY()[i]);
  
  }

  errorband_ratio->SetFillColor(kYellow);
  errorband_ratio->SetLineColor(kYellow);
  errorband_ratio->SetLineWidth(2);
  errorband_ratio->SetFillStyle(3001);

  errorband_ratio_up->SetFillColor(kGray+1);
  errorband_ratio_up->SetLineColor(kGray+1);
  errorband_ratio_up->SetLineWidth(2);
  errorband_ratio_up->SetLineStyle(7);

  errorband_ratio_dn->SetFillColor(kGray+1);
  errorband_ratio_dn->SetLineColor(kGray+1);
  errorband_ratio_dn->SetLineWidth(2);
  errorband_ratio_dn->SetLineStyle(7);
  
  mplot->addObject(errorband_ratio,"E2");             
  mplot->addObject(errorband_ratio_up,"L");             
  mplot->addObject(errorband_ratio_dn,"L");             

  return;
}


/// Calculate the error when intgrating a pdf in a range -> take the error not as a single fit result but from toys randomizing the parameters
double Calc_error_extendPdf( RooAbsData* rdata,  RooExtendPdf* rpdf, RooFitResult *rfres, const std::string & range, const int & calc_times){

 TRandom3 rand(1234);
 rand.SetSeed(); 
 std::cout<<" <<<<<<<<<<<<<<<  Calc_error_extendPdf <<<<<<<<<<<<<<<< "<<std::endl;
 /// Get the observable on the x-axis
 RooArgSet* argset_obs = rpdf->getObservables(rdata);
 TIterator *par = argset_obs->createIterator();
 par->Reset();
 RooRealVar *rrv_x=(RooRealVar*)par->Next();
 rrv_x->Print();

 /// Create integral of the whole function and in a given range
 RooAbsReal* fullInt   = rpdf->createIntegral(*rrv_x,*rrv_x);
 RooAbsReal* signalInt = rpdf->createIntegral(*rrv_x,*rrv_x,range.c_str());
 double fullInt_var    = fullInt->getVal();
 double signalInt_var  = signalInt->getVal()/fullInt_var;

 double signal_number_media = signalInt_var*rpdf->expectedEvents(*rrv_x);

 /// Take the parameters and randomize them within uncertainty and do a lot of toys
 RooArgSet* par_pdf  = rpdf->getParameters(RooArgSet(*rrv_x)) ;
 par_pdf->Print("v");

 std::vector<double> val;
 val.resize(calc_times);
 for(int j=0;j<calc_times;j++){
	RooArgList par_tmp = rfres->randomizePars();
	*par_pdf = par_tmp;

        fullInt_var = fullInt->getVal();
        signalInt_var = signalInt->getVal()/fullInt_var;
        signal_number_media = signalInt_var*rpdf->expectedEvents(*rrv_x);
        val[j] = signal_number_media;
 }

 RooArgList par_tmp = rfres->floatParsFinal();
 *par_pdf = par_tmp;

 std::sort(val.begin(),val.end());
 return (val[Int_t(0.84*calc_times)]-val[Int_t(0.16*calc_times)])/2.; /// return a doble value 

}

/// useful for couting analysis
double Calc_error(std::string rpdfname, std::string xaxis_name ,  RooArgList* paras,  RooWorkspace *ws, const std::string & range, const int & calc_times){

 TRandom3 rand(1234);
 rand.SetSeed(0);
 // Get the observable
 std::cout<<" <<<<<<<<<<<<<<<  Calc_error <<<<<<<<<<<<<<<< "<<std::endl;
 RooRealVar *rrv_x=ws->var(xaxis_name.c_str()); 
 rrv_x->Print();
 RooAbsPdf*rpdf=ws->pdf(rpdfname.c_str());

 RooAbsReal* fullInt = rpdf->createIntegral(*rrv_x,*rrv_x);
 RooAbsReal* signalInt = rpdf->createIntegral(*rrv_x,*rrv_x,range.c_str());
 double fullInt_var = fullInt->getVal();
 double signalInt_var = signalInt->getVal()/fullInt_var;

 double signal_number_media=signalInt_var;

 std::vector<double> val;
 val.resize(calc_times);
 for(int j=0;j<calc_times;j++){
    for(Int_t ipara=0;ipara<paras->getSize();ipara++){
      ws->var(paras->at(ipara)->GetName())->setConstant(0);
      ws->var(paras->at(ipara)->GetName())->setVal( rand.Gaus(0.,ws->var(paras->at(ipara)->GetName())->getError()) );
     }

     fullInt_var=fullInt->getVal();
     signalInt_var=signalInt->getVal()/fullInt_var;

     signal_number_media=signalInt_var;
     val[j]=signal_number_media;
 }
 
for(Int_t ipara=0;ipara<paras->getSize();ipara++){ ws->var(paras->at(ipara)->GetName())->setVal(0.); }

std::sort(val.begin(),val.end());
double number_error=(val[Int_t(0.84*calc_times)]-val[Int_t(0.16*calc_times)])/2./signal_number_media;
return number_error;
}
