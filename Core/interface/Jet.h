#ifndef Analysis_Core_Jet_h
#define Analysis_Core_Jet_h

#ifndef STANDALONE
#include "Analysis/Core/interface/GenJet.h"
#else
#include "GenJet.h"
#endif


namespace analysis
{
	namespace core
	{
		class Jet : public Object
		{
			public:
				Jet() : Object() {}

				virtual void reset()
				{
					_px = 0;
					_py = 0;
					_pz = 0;
					_pt = 0;
					_eta = 0;
					_phi = 0;
					_mass = 0;
					_charge = 0;
					_partonFlavour = 0;
					_chf = 0;
					_nhf = 0;
					_cef = 0;
					_nef = 0;
					_muf = 0;
					_hfhf = 0;
					_hfef = 0;
					_cm = 0;
					_chm = 0;
					_nhm = 0;
					_cem = 0;
					_nem = 0;
					_mum = 0;
					_hfhm = 0;
					_hfem = 0;
					_jecf = 0;
					_jecu = 0;
					_csv = 0;
					_puid = 0;

					_genjet.reset();
					_genMatched = 0;
					_genemf = 0;
					_genhadf = 0;
					_geninvf = 0;
					_genauxf = 0;
				}
				virtual ~Jet() {}

				float _px;
				float _py;
				float _pz;
				float _pt;
				float _eta;
				float _phi;
				float _mass;
				float _charge;
				float _partonFlavour;

				float _chf;
				float _nhf;
				float _cef;
				float _nef;
				float _muf;
				float _hfhf;
				float _hfef;
				float _cm;
				float _chm;
				float _nhm;
				float _cem;
				float _nem;
				float _mum;
				float _hfhm;
				float _hfem;
				float _jecf;
				float _jecu;
				float _csv;
				float _puid;

				//	
				GenJet _genjet;
				bool _genMatched;
				float _genemf;
				float _genhadf;
				float _geninvf;
				float _genauxf;

#ifdef STANDALONE 
				ClassDef(Jet, 1)
#endif
		};

		typedef std::vector<analysis::core::Jet> Jets;
	}
}

#ifdef STANDALONE
ClassImpUnique(analysis::core::Jet, Jet)
#endif

#endif
