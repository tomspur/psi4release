/** Standard library includes */
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>

 
/** Required PSI4 includes */
#include <psifiles.h>
#include <libciomr/libciomr.h>
#include <libpsio/psio.h> 
#include <libchkpt/chkpt.h>
#include <libpsio/psio.hpp>
#include <libchkpt/chkpt.hpp> 
#include <libiwl/iwl.hpp>
#include <libqt/qt.h>
#include <libtrans/mospace.h>
#include <libtrans/integraltransform.h>


/** Required libmints includes */
#include <libmints/mints.h>
#include <libmints/factory.h>
#include <libmints/wavefunction.h>

#include "occwave.h"
#include "defines.h"

using namespace boost;
using namespace psi;
using namespace std;

namespace psi{ namespace occwave{

void OCCWave::gfock()
{

//fprintf(outfile,"\n omp3_gfock is starting... \n"); fflush(outfile);
//===========================================================================================
//========================= RHF =============================================================
//===========================================================================================
if (reference_ == "RESTRICTED") {

	// Initialize
	GFock->zero();

        // 1e-part
	HG1->zero();
	HG1->gemm(false, false, 1.0, HmoA, g1symm, 0.0);
	GFock->add(HG1);
	Ecc_rdm = HG1->trace() + Enuc; // One-electron contribution to MP2L

        // 2e-part
	dpdbuf4 G, K, X, T, Y;
	dpdfile2 GF;
	
	psio_->open(PSIF_LIBTRANS_DPD, PSIO_OPEN_OLD);
	psio_->open(PSIF_OCC_DENSITY, PSIO_OPEN_OLD);
        psio_->open(PSIF_OCC_DPD, PSIO_OPEN_OLD); 


if (wfn_type_ != "OMP2") { 
   // Build X intermediate 
   if (twopdm_abcd_type == "DIRECT" ) {
        // With this algorithm cost changes to o2v4 + ov4 => o3v3 + o3v2, roughly v/o times faster 
 	// X_MNIC = 2\sum{E,F} t_MN^EF(1) * <IC|EF>   
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "X <OO|OV>");
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV|VV>");
        if (wfn_type_ == "OMP3") { 
             dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2_1 <OO|VV>");
        }
        else if (wfn_type_ == "OCEPA") { 
             dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2 <OO|VV>");
        }
        dpd_contract444(&T, &K, &X, 0, 0, 2.0, 0.0);
	dpd_buf4_close(&K);
	dpd_buf4_close(&T);
	dpd_buf4_close(&X);
    }
}// end if (wfn_type_ != "OMP2")  

	
	// Build Fai
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('V'), ID('O'), "GF <V|O>"); 
	
	// Fai += 4 * \sum{m,n,k} <km|na> * G_kmni
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"),
                  ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"), 0, "MO Ints <OO|OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,O]"),
                  ID("[O,O]"), ID("[O,O]"), 0, "TPDM <OO|OO>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ == "OMP2") { 
        dpdfile2 G;

        // Build Virtual-Virtual block of correlation OPDM
        dpd_file2_init(&G, PSIF_OCC_DENSITY, 0, ID('V'), ID('V'), "CORR OPDM <V|V>");
        dpd_file2_mat_init(&G);
        for(int h = 0; h < nirrep_; ++h){
            for(int i = 0 ; i < avirtpiA[h]; ++i){
                for(int j = 0 ; j < avirtpiA[h]; ++j){
                    G.matrix[h][i][j] = gamma1corr->get(h, i + occpiA[h], j + occpiA[h]);
                }
            }
        }
        dpd_file2_mat_wrt(&G);
        dpd_file2_close(&G);
 
        // Sort
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                      ID("[O,V]"), ID("[V>=V]+"), 0, "MO Ints (OV|VV)");
        dpd_buf4_sort(&K, PSIF_LIBTRANS_DPD , qprs, ID("[V,O]"), ID("[V,V]"), "MO Ints (VO|VV)");
	dpd_buf4_close(&K);
 
	// Fai += 2 * \sum{e,f} (ai|ef) * Gcorr(e,f) 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[V,O]"), ints->DPD_ID("[V,V]"),
                  ints->DPD_ID("[V,O]"), ints->DPD_ID("[V,V]"), 0, "MO Ints (VO|VV)");
        dpd_file2_init(&G, PSIF_OCC_DENSITY, 0, ID('V'), ID('V'), "CORR OPDM <V|V>");
        dpd_contract422(&K, &G, &GF, 0, 0, 2.0, 1.0);
	dpd_buf4_close(&K);
        dpd_file2_close(&G);
}// end if (wfn_type_ != "OMP2") 

else { 
	// Fai += 4 * \sum{e,m,f} <me|af> * G_meif 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[O,V]"), ints->DPD_ID("[V,V]"),
                  ints->DPD_ID("[O,V]"), ints->DPD_ID("[V,V]"), 0, "MO Ints <OV|VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "TPDM <OV|OV>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
}// end else 

if (wfn_type_ == "OMP2" && incore_iabc_ == 0) { 
      // Fai += 8 * \sum{e,m,f} <ma|ef> * G_mief 
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
      	IWL ERIIN(psio_.get(), PSIF_OCC_IABC, 0.0, 1, 1);
	int ilsti,nbuf,index,fi;
	double value = 0;

 do
 {
        ilsti = ERIIN.last_buffer(); 
        nbuf = ERIIN.buffer_count();
	
   fi = 0;
   for (int idx=0; idx < nbuf; idx++ )
   {

        int m = ERIIN.labels()[fi];
            m = abs(m);
        int a = ERIIN.labels()[fi+1];
        int e = ERIIN.labels()[fi+2];
        int f = ERIIN.labels()[fi+3];
        value = ERIIN.values()[idx];
        fi += 4;

        int m_pitzer = qt2pitzerA[m];
        int a_pitzer = qt2pitzerA[a];
        int e_pitzer = qt2pitzerA[e];
        int f_pitzer = qt2pitzerA[f];
  
        int hm = mosym[m_pitzer];
        int ha = mosym[a_pitzer];
        int he = mosym[e_pitzer];
        int hf = mosym[f_pitzer];
 
        int hma = ha^hm;
        int hef = he^hf;

        if (hma == hef) {
            dpd_buf4_mat_irrep_init(&G, hma);
            dpd_buf4_mat_irrep_rd(&G, hma);
            double summ = 0.0;
            int ee = e - nooA;
            int ff = f - nooA;
            for (int i=0; i < occpiA[ha]; i++) {
                 int I = occ_offA[ha] + i;
                 int i_pitzer = qt2pitzerA[I];
                 int mi = G.params->rowidx[m][I];
                 int ef = G.params->colidx[ee][ff];
                 summ = 8.0 * value * G.matrix[hma][mi][ef];  
                 int aa = pitzer2symblk[a_pitzer];
                 GFock->add(ha, aa, i, summ); 
            }
            dpd_buf4_mat_irrep_close(&G, hma);
        }

   }
        if(!ilsti)
	  ERIIN.fetch();

 } while(!ilsti);

        // Close
	dpd_buf4_close(&G);
} // end if (wfn_type_ == "OMP2" && incore_iabc_ == 0)

else {
        // Fai += 8 * \sum{e,m,f} <ma|ef> * G_mief 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[O,V]"), ints->DPD_ID("[V,V]"),
                  ints->DPD_ID("[O,V]"), ints->DPD_ID("[V,V]"), 0, "MO Ints <OV|VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
	dpd_contract442(&K, &G, &GF, 1, 1, 8.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	dpd_file2_close(&GF);
}// end else
	
	// Build Fia
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('O'), ID('V'), "GF <O|V>");  
	
	// Fia += 4 * \sum{m,e,n} <mi|ne> * G_mane
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"),
                  ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"), 0, "MO Ints <OO|OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "TPDM <OV|OV>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
	// Fia += 8 * \sum{m,e,n} <nm|ie> * G_nmae
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"),
                  ints->DPD_ID("[O,O]"), ints->DPD_ID("[O,V]"), 0, "MO Ints <OO|OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
	dpd_contract442(&K, &G, &GF, 2, 2, 8.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
       	// Fia += \sum{m,n,c} X_mnic * (2t_mn^ac(1) - tmn^ca(1)) 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "X <OO|OV>");
        if (wfn_type_ == "OMP3") { 
           dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "Tau_1 <OO|VV>");
        }
        else if (wfn_type_ == "OCEPA") { 
           dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "Tau <OO|VV>");
        }
	dpd_contract442(&X, &T, &GF, 2, 2, 1.0, 1.0); 
	dpd_buf4_close(&X);
	dpd_buf4_close(&T);
	dpd_file2_close(&GF);
   }

   else if (twopdm_abcd_type == "COMPUTE" ) {
       	// Fia += 4 * \sum{e,f,c} <ce|fi> * G_cefa = 4 * \sum{e,f,c} <if|ce> * G_afce 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV|VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,V]"), ID("[V,V]"),
                  ID("[V,V]"), ID("[V,V]"), 0, "TPDM <VV|VV>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	dpd_file2_close(&GF);
   }
}// end if (wfn_type_ != "OMP2") 
	
	psio_->close(PSIF_LIBTRANS_DPD, 1);
        psio_->close(PSIF_OCC_DPD, 1);
	
	// Load Fai
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('V'), ID('O'), "GF <V|O>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int a = 0 ; a < virtpiA[h]; ++a){
            for(int i = 0 ; i < occpiA[h]; ++i){
                GFock->add(h, a + occpiA[h], i, GF.matrix[h][a][i]);
            }
	  }
	}
	dpd_file2_close(&GF);
	
	// Load Fia
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('O'), ID('V'), "GF <O|V>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int i = 0 ; i < occpiA[h]; ++i){
            for(int a = 0 ; a < virtpiA[h]; ++a){
                GFock->add(h, i, a + occpiA[h], GF.matrix[h][i][a]);
            }
	  }
	}
	dpd_file2_close(&GF);
	
	psio_->close(PSIF_OCC_DENSITY, 1);
	if (print_ > 2) GFock->print();
	
}// end if (reference_ == "RESTRICTED") 



//===========================================================================================
//========================= UHF =============================================================
//===========================================================================================
else if (reference_ == "UNRESTRICTED") {

/********************************************************************************************/
/************************** Initialize ******************************************************/
/********************************************************************************************/ 
	GFockA->zero();
	GFockB->zero();

/********************************************************************************************/
/************************** 1e-part *********************************************************/
/********************************************************************************************/ 
	HG1A->zero();
	HG1B->zero();
	HG1A->gemm(false, false, 1.0, HmoA, g1symmA, 0.0);
	HG1B->gemm(false, false, 1.0, HmoB, g1symmB, 0.0);
	GFockA->add(HG1A);
	GFockB->add(HG1B);
	Ecc_rdm = HG1A->trace() + HG1B->trace() + Enuc; // One-electron contribution to MP2L

/********************************************************************************************/
/************************** 2e-part *********************************************************/
/********************************************************************************************/ 
	dpdbuf4 G, K, T, L, X;
	dpdfile2 GF;
	
	psio_->open(PSIF_LIBTRANS_DPD, PSIO_OPEN_OLD);
	psio_->open(PSIF_OCC_DENSITY, PSIO_OPEN_OLD);
        psio_->open(PSIF_OCC_DPD, PSIO_OPEN_OLD); 
	
/********************************************************************************************/
/************************** Build X intermediates *******************************************/
/********************************************************************************************/ 
if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
        // With this algorithm cost changes to 3*o2v4 + 4*ov4 => 4*(o3v3 + o3v2), ~5-times faster 
 	// X_MNIC = \sum{E,F} t_MN^EF * <IC||EF> = 2\sum{E,F} t_MN^EF * <IC|EF>  
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "X <OO|OV>");
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV|VV>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2_1 <OO|VV>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2 <OO|VV>");
        }
        dpd_contract444(&T, &K, &X, 0, 0, 2.0, 0.0);
	dpd_buf4_close(&K);
	dpd_buf4_close(&T);
	dpd_buf4_close(&X);

	// X_mnic = \sum{e,f} t_mn^ef * <ic||ef> = 2\sum{e,f} t_mn^ef * <ic|ef> 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[o,v]"),
                  ID("[o,o]"), ID("[o,v]"), 0, "X <oo|ov>");
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov|vv>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "T2_1 <oo|vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "T2 <oo|vv>");
        }
        dpd_contract444(&T, &K, &X, 0, 0, 2.0, 0.0);
	dpd_buf4_close(&K);
	dpd_buf4_close(&T);
	dpd_buf4_close(&X);

        // X_MnIc = \sum{E,f} t_Mn^Ef * <Ic|Ef> 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[O,v]"),
                  ID("[O,o]"), ID("[O,v]"), 0, "X <Oo|Ov>");
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[V,v]"),
                  ID("[O,v]"), ID("[V,v]"), 0, "MO Ints <Ov|Vv>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2_1 <Oo|Vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2 <Oo|Vv>");
        }
        dpd_contract444(&T, &K, &X, 0, 0, 1.0, 0.0);
	dpd_buf4_close(&K);
	dpd_buf4_close(&T);
	dpd_buf4_close(&X);

        // X_MnCi = \sum{E,f} t_Mn^Ef * <Ci|Ef> 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,o]"),
                  ID("[O,o]"), ID("[V,o]"), 0, "X <Oo|Vo>");
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[V,o]"), ID("[V,v]"),
                  ID("[V,o]"), ID("[V,v]"), 0, "MO Ints <Vo|Vv>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2_1 <Oo|Vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2 <Oo|Vv>");
        }
        dpd_contract444(&T, &K, &X, 0, 0, 1.0, 0.0);
	dpd_buf4_close(&K);
	dpd_buf4_close(&T);
	dpd_buf4_close(&X);

  }// end main if for X
} // end if (wfn_type_ != "OMP2") { 
	
/********************************************************************************************/
/************************** VO-Block ********************************************************/
/********************************************************************************************/ 
	// Build FAJ
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('V'), ID('O'), "GF <V|O>"); 
	
	// FAJ = 2 * \sum{M,N,K} <MN||KA> * G_MNKJ
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "MO Ints <OO||OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,O]"),
                  ID("[O,O]"), ID("[O,O]"), 0, "TPDM <OO|OO>");
	dpd_contract442(&K, &G, &GF, 3, 3, 2.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

        /*	
	// FAJ += 2 * \sum{E,M,F} <EF||MA> * G_MJEF = 2 * \sum{E,M,F} <MA||EF> * G_MJEF 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV||VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
	dpd_contract442(&K, &G, &GF, 1, 1, 2.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
        */

        // FAJ += 2 * \sum{E,M,F} <EF||MA> * G_MJEF = 2 * \sum{E,M,F} <MA||EF> * G_MJEF 
        //      = 4 * \sum{E,M,F} <MA|EF> * G_MJEF
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV|VV>");
        dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
        dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
        dpd_buf4_close(&G);
 
        /*	
	// FAJ += 4 * \sum{E,M,F} <EM||FA> * G_MEJF = 4 * \sum{E,M,F} <ME||AF> * G_MEJF
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV||VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "TPDM <OV|OV>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
        */
   
     	// FAJ += 4 * \sum{E,M,F} <EM||FA> * G_MEJF = 4 * \sum{E,M,F} <ME||AF> * G_MEJF
     	//      = 4 * \sum{E,M,F} <ME|AF> * G_MEJF - 4 * \sum{E,M,F} <ME|FA> * G_MEJF 
        //      = 4 * \sum{E,M,F} <ME|AF> * G_MEJF + 4 * \sum{E,M,F} <ME|FA> * G_MEFJ => 1st term
        dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "TPDM <OV|OV>");
        dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0);
        dpd_buf4_close(&G);
 
	// FAJ += 4 * \sum{E,M,F} <EM||FA> * G_MEJF = 4 * \sum{E,M,F} <ME||AF> * G_MEJF
     	//      = 4 * \sum{E,M,F} <ME|AF> * G_MEJF - 4 * \sum{E,M,F} <ME|FA> * G_MEJF 
        //      = 4 * \sum{E,M,F} <ME|AF> * G_MEJF + 4 * \sum{E,M,F} <ME|FA> * G_MEFJ => 2nd term
        dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[V,O]"),
                  ID("[O,V]"), ID("[V,O]"), 0, "TPDM <OV|VO>");
        dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0);
        dpd_buf4_close(&K);
        dpd_buf4_close(&G);

	// FAJ += 4 * \sum{m,N,k} <Nm|Ak> * G_NmJk 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,o]"),
                  ID("[O,o]"), ID("[V,o]"), 0, "MO Ints <Oo|Vo>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[O,o]"),
                  ID("[O,o]"), ID("[O,o]"), 0, "TPDM <Oo|Oo>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
	// FAJ += 4 * \sum{e,F,m} <Fe|Am> * G_JmFe = 4 * \sum{e,F,m} <Am|Fe> * G_JmFe
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[V,o]"), ID("[V,v]"),
                  ID("[V,o]"), ID("[V,v]"), 0, "MO Ints <Vo|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "TPDM <Oo|Vv>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ != "OMP2") { 
	// FAJ += 4 * \sum{E,f,m} <Em|Af> * G_EmJf  => this new  
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[V,o]"), ID("[V,v]"),
                  ID("[V,o]"), ID("[V,v]"), 0, "MO Ints <Vo|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,o]"), ID("[O,v]"),
                  ID("[V,o]"), ID("[O,v]"), 0, "TPDM <Vo|Ov>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
}// end if (wfn_type_ != "OMP2") 
	
	// FAJ += 4 * \sum{e,f,M} <Me|Af> * G_MeJf
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[V,v]"),
                  ID("[O,v]"), ID("[V,v]"), 0, "MO Ints <Ov|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,v]"), ID("[O,v]"),
                  ID("[O,v]"), ID("[O,v]"), 0, "TPDM <Ov|Ov>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

	// Close
	dpd_file2_close(&GF);
	
	
	// Build Faj
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('v'), ID('o'), "GF <v|o>"); 
	
	// Faj = 2 * \sum{m,n,k} <mn||ka> * G_mnkj
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,o]"), ID("[o,v]"),
                  ID("[o,o]"), ID("[o,v]"), 0, "MO Ints <oo||ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[o,o]"),
                  ID("[o,o]"), ID("[o,o]"), 0, "TPDM <oo|oo>");
	dpd_contract442(&K, &G, &GF, 3, 3, 2.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

        /*	
        // Faj += 2 * \sum{e,m,f} <ef||ma> * G_mjef = 2 * \sum{e,m,f} <ma||ef> * G_mjef 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov||vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "TPDM <oo|vv>");
	dpd_contract442(&K, &G, &GF, 1, 1, 2.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
        */

        // Faj += 2 * \sum{e,m,f} <ef||ma> * G_mjef = 2 * \sum{e,m,f} <ma||ef> * G_mjef 
	//      = 4 * \sum{e,m,f} <ma|ef> * G_mjef 
        dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov|vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "TPDM <oo|vv>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&G);
	
	/*
        // Faj += 4 * \sum{e,m,f} <em||fa> * G_mejf = 4 * \sum{e,m,f} <me||af> * G_mejf
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov||vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "TPDM <ov|ov>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	*/

        // Faj += 4 * \sum{e,m,f} <em||fa> * G_mejf = 4 * \sum{e,m,f} <me||af> * G_mejf
        //      = 4 * \sum{e,m,f} <me|af> * G_mejf - 4 * \sum{e,m,f} <me|fa> * G_mejf 
        //      = 4 * \sum{e,m,f} <me|af> * G_mejf + 4 * \sum{e,m,f} <me|fa> * G_mefj => 1st term
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "TPDM <ov|ov>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&G);
 
        // Faj += 4 * \sum{e,m,f} <em||fa> * G_mejf = 4 * \sum{e,m,f} <me||af> * G_mejf
        //      = 4 * \sum{e,m,f} <me|af> * G_mejf - 4 * \sum{e,m,f} <me|fa> * G_mejf 
        //      = 4 * \sum{e,m,f} <me|af> * G_mejf + 4 * \sum{e,m,f} <me|fa> * G_mefj => 2nd term
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,v]"), ID("[v,o]"),
                  ID("[o,v]"), ID("[v,o]"), 0, "TPDM <ov|vo>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

	// Faj += 4 * \sum{M,n,K} <Mn|Ka> * G_MnKj 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[O,v]"),
                  ID("[O,o]"), ID("[O,v]"), 0, "MO Ints <Oo|Ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[O,o]"),
                  ID("[O,o]"), ID("[O,o]"), 0, "TPDM <Oo|Oo>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
	// Faj += 4 * \sum{E,f,M} <Ef|Ma> * G_MjEf = 4 * \sum{E,f,M} <Ma|Ef> * G_MjEf 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[V,v]"),
                  ID("[O,v]"), ID("[V,v]"), 0, "MO Ints <Ov|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "TPDM <Oo|Vv>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ != "OMP2") { 
	// Faj += 4 * \sum{E,f,M} <Me|Fa> * G_MeFj => this new  
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[V,v]"),
                  ID("[O,v]"), ID("[V,v]"), 0, "MO Ints <Ov|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,v]"), ID("[V,o]"),
                  ID("[O,v]"), ID("[V,o]"), 0, "TPDM <Ov|Vo>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
}// end if (wfn_type_ != "OMP2")
	
	// Faj += 4 * \sum{E,F,m} <Em|Fa> * G_EmFj
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[V,o]"), ID("[V,v]"),
                  ID("[V,o]"), ID("[V,v]"), 0, "MO Ints <Vo|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,o]"), ID("[V,o]"),
                  ID("[V,o]"), ID("[V,o]"), 0, "TPDM <Vo|Vo>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

	// Close
	dpd_file2_close(&GF);
	
	
/********************************************************************************************/
/************************** OV-Block ********************************************************/
/********************************************************************************************/ 
	// Build FIB
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('O'), ID('V'), "GF <O|V>");  
	
	// FIB = 2 * \sum{M,N,E} <MN||EI> * G_MNEB = 2 * \sum{M,N,E} <MN||IE> * G_MNBE
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "MO Ints <OO||OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "TPDM <OO|VV>");
	dpd_contract442(&K, &G, &GF, 2, 2, 2.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

        /*	
	// FIB += 2 * \sum{E,F,C} <EF||CI> * G_EFCB = 2 * \sum{E,F,C} <IC||FE> * G_BCFE 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV||VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,V]"), ID("[V,V]"),
                  ID("[V,V]"), ID("[V,V]"), 0, "TPDM <VV|VV>");
	dpd_contract442(&K, &G, &GF, 0, 0, 2.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
        */

if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
       	// FIB += 2 * \sum{E,F,C} <EF||CI> * G_EFCB = 1/4  \sum{M,N,C} X_MNIC * t_MN^BC 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "X <OO|OV>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2_1 <OO|VV>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "T2 <OO|VV>");
        }
	dpd_contract442(&X, &T, &GF, 2, 2, 0.25, 1.0); 
	dpd_buf4_close(&X);
	dpd_buf4_close(&T);
   }

   else if (twopdm_abcd_type == "COMPUTE" ) {
       	// FIB += 2 * \sum{E,F,C} <EF||CI> * G_EFCB = 2 * \sum{E,F,C} <IC||FE> * G_BCFE 
       	//      = 4 * \sum{E,F,C} <IC|FE> * G_BCFE
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[V,V]"),
                  ID("[O,V]"), ID("[V,V]"), 0, "MO Ints <OV|VV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,V]"), ID("[V,V]"),
                  ID("[V,V]"), ID("[V,V]"), 0, "TPDM <VV|VV>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
   }
}// end if (wfn_type_ != "OMP2")
	
	// FIB += 4 * \sum{M,N,E} <ME||NI> * G_MENB = 4 * \sum{M,N,E} <NI||ME> * G_NBME
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,O]"), ID("[O,V]"),
                  ID("[O,O]"), ID("[O,V]"), 0, "MO Ints <OO||OV>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "TPDM <OV|OV>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);


if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
       	// FIB += 4 * \sum{e,F,c} <Fe|Ic> * G_FeBc =  \sum{M,n,C} X_MnIc * t_Mn^Bc 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[O,v]"),
                  ID("[O,o]"), ID("[O,v]"), 0, "X <Oo|Ov>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2_1 <Oo|Vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2 <Oo|Vv>");
        }
	dpd_contract442(&X, &T, &GF, 2, 2, 1.0, 1.0); 
	dpd_buf4_close(&X);
	dpd_buf4_close(&T);
   }

   else if (twopdm_abcd_type == "COMPUTE" ) {
	// FIB += 4 * \sum{e,F,c} <Fe|Ic> * G_FeBc = 4 * \sum{e,F,c} <Ic|Fe> * G_BcFe  => this is new
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[V,v]"),
                  ID("[O,v]"), ID("[V,v]"), 0, "MO Ints <Ov|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,v]"), ID("[V,v]"),
                  ID("[V,v]"), ID("[V,v]"), 0, "TPDM <Vv|Vv>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
   }
}// end if (wfn_type_ != "OMP2")
	
	// FIB += 4 * \sum{m,N,e} <Nm|Ie> * G_NmBe 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[O,v]"),
                  ID("[O,o]"), ID("[O,v]"), 0, "MO Ints <Oo|Ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "TPDM <Oo|Vv>");
	dpd_contract442(&K, &G, &GF, 2, 2, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
	// FIB += 4 * \sum{m,n,E} <Em|In> * G_EmBn = 4 * \sum{m,n,E} <In|Em> * G_BnEm
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,o]"),
                 ID("[O,o]"), ID("[V,o]"), 0, "MO Ints <Oo|Vo>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,o]"), ID("[V,o]"),
                  ID("[V,o]"), ID("[V,o]"), 0, "TPDM <Vo|Vo>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ != "OMP2") { 
	// FIB += 4 * \sum{M,n,e} <Me|In> * G_MeBn = 4 * \sum{M,n,e} <In|Me> * G_BnMe  => this is new
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[O,v]"),
                  ID("[O,o]"), ID("[O,v]"), 0, "MO Ints <Oo|Ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,o]"), ID("[O,v]"),
                  ID("[V,o]"), ID("[O,v]"), 0, "TPDM <Vo|Ov>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
}// end if (wfn_type_ != "OMP2")
	
	// Close
	dpd_file2_close(&GF);
	
	
	
	// Build Fib
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('o'), ID('v'), "GF <o|v>");  
	
	// Fib = 2 * \sum{m,n,e} <mn||ei> * G_mneb = 2 * \sum{m,n,e} <mn||ie> * G_mnbe
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,o]"), ID("[o,v]"),
                  ID("[o,o]"), ID("[o,v]"), 0, "MO Ints <oo||ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "TPDM <oo|vv>");
	dpd_contract442(&K, &G, &GF, 2, 2, 2.0, 0.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);

        /*	
	// Fib += 2 * \sum{e,f,c} <ef||ci> * G_efcb = 2 * \sum{e,f,c} <ic||fe> * G_bcfe 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov||vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[v,v]"), ID("[v,v]"),
                  ID("[v,v]"), ID("[v,v]"), 0, "TPDM <vv|vv>");
	dpd_contract442(&K, &G, &GF, 0, 0, 2.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
        */

if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
       	// Fib += 2 * \sum{e,f,c} <ef||ci> * G_efcb = 1/4  \sum{m,n,c} X_mnic * t_mn^bc 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[o,o]"), ID("[o,v]"),
                  ID("[o,o]"), ID("[o,v]"), 0, "X <oo|ov>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "T2_1 <oo|vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "T2 <oo|vv>");
        }
	dpd_contract442(&X, &T, &GF, 2, 2, 0.25, 1.0); 
	dpd_buf4_close(&X);
	dpd_buf4_close(&T);
   }

   else if (twopdm_abcd_type == "COMPUTE" ) {
        // Fib += 2 * \sum{e,f,c} <ef||ci> * G_efcb = 2 * \sum{e,f,c} <ic||fe> * G_bcfe 
        //      = 4 * \sum{e,f,c} <ic|fe> * G_bcfe
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[v,v]"),
                  ID("[o,v]"), ID("[v,v]"), 0, "MO Ints <ov|vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[v,v]"), ID("[v,v]"),
                  ID("[v,v]"), ID("[v,v]"), 0, "TPDM <vv|vv>");
	dpd_contract442(&K, &G, &GF, 0, 0, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
   }
}// end if (wfn_type_ != "OMP2")
 
	
	// Fib += 4 * \sum{m,n,e} <me||ni> * G_menb = 4 * \sum{m,n,e} <ni||me> * G_nbme
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[o,o]"), ID("[o,v]"),
                  ID("[o,o]"), ID("[o,v]"), 0, "MO Ints <oo||ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "TPDM <ov|ov>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ != "OMP2") { 
   if (twopdm_abcd_type == "DIRECT" ) {
       	// Fib += 4 * \sum{E,f,C} <Ef|Ci> * G_EfCb =  \sum{M,n,C} X_MnCi * t_Mn^Cb 
        dpd_buf4_init(&X, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,o]"),
                  ID("[O,o]"), ID("[V,o]"), 0, "X <Oo|Vo>");
        if (wfn_type_ == "OMP3") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2_1 <Oo|Vv>");
        }
        else if (wfn_type_ == "OCEPA") { 
            dpd_buf4_init(&T, PSIF_OCC_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "T2 <Oo|Vv>");
        }
	dpd_contract442(&X, &T, &GF, 3, 3, 1.0, 1.0); 
	dpd_buf4_close(&X);
	dpd_buf4_close(&T);
   }

   else if (twopdm_abcd_type == "COMPUTE" ) {
	// Fib += 4 * \sum{E,f,C} <Ef|Ci> * G_EfCb = 4 * \sum{E,f,C} <Ci|Ef> * G_CbEf  => this is new
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[V,o]"), ID("[V,v]"),
                  ID("[V,o]"), ID("[V,v]"), 0, "MO Ints <Vo|Vv>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[V,v]"), ID("[V,v]"),
                  ID("[V,v]"), ID("[V,v]"), 0, "TPDM <Vv|Vv>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
   }
}// end if (wfn_type_ != "OMP2")
		
	// Fib += 4 * \sum{M,n,E} <Mn|Ei> * G_MnEb 
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,o]"),
                  ID("[O,o]"), ID("[V,o]"), 0, "MO Ints <Oo|Vo>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "TPDM <Oo|Vv>");
	dpd_contract442(&K, &G, &GF, 3, 3, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
	// Fib += 4 * \sum{M,N,e} <Me|Ni> * G_MeNb = 4 * \sum{M,N,e} <Ni|Me> * G_NbMe
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[O,v]"),
                 ID("[O,o]"), ID("[O,v]"), 0, "MO Ints <Oo|Ov>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,v]"), ID("[O,v]"),
                  ID("[O,v]"), ID("[O,v]"), 0, "TPDM <Ov|Ov>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
	
if (wfn_type_ != "OMP2") { 
	// Fib += 4 * \sum{m,N,E} <Em|Ni> * G_EmNb = 4 * \sum{m,N,E} <Ni|Em> * G_NbEm   => this is new
	dpd_buf4_init(&K, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,o]"),
                   ID("[O,o]"), ID("[V,o]"), 0, "MO Ints <Oo|Vo>");
	dpd_buf4_init(&G, PSIF_OCC_DENSITY, 0, ID("[O,v]"), ID("[V,o]"),
                  ID("[O,v]"), ID("[V,o]"), 0, "TPDM <Ov|Vo>");
	dpd_contract442(&K, &G, &GF, 1, 1, 4.0, 1.0); 
	dpd_buf4_close(&K);
	dpd_buf4_close(&G);
}// end if (wfn_type_ != "OMP2")  
	
	// Close
	dpd_file2_close(&GF);
	psio_->close(PSIF_LIBTRANS_DPD, 1);
        psio_->close(PSIF_OCC_DPD, 1);

/********************************************************************************************/
/************************** Load dpd_file2 to SharedMatrix (GFock) **************************/
/********************************************************************************************/ 
	// Load FAI
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('V'), ID('O'), "GF <V|O>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int a = 0 ; a < virtpiA[h]; ++a){
            for(int i = 0 ; i < occpiA[h]; ++i){
                GFockA->add(h, a + occpiA[h], i, GF.matrix[h][a][i]);
            }
	  }
	}
	dpd_file2_close(&GF);
	
	// Load Fai
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('v'), ID('o'), "GF <v|o>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int a = 0 ; a < virtpiB[h]; ++a){
            for(int i = 0 ; i < occpiB[h]; ++i){
                GFockB->add(h, a + occpiB[h], i, GF.matrix[h][a][i]);
            }
	  }
	}
	dpd_file2_close(&GF);
	
	// Load FIA
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('O'), ID('V'), "GF <O|V>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int i = 0 ; i < occpiA[h]; ++i){
            for(int a = 0 ; a < virtpiA[h]; ++a){
                GFockA->add(h, i, a + occpiA[h], GF.matrix[h][i][a]);
            }
	  }
	}
	dpd_file2_close(&GF);
	
	// Load Fia
	dpd_file2_init(&GF, PSIF_OCC_DENSITY, 0, ID('o'), ID('v'), "GF <o|v>");  
	dpd_file2_mat_init(&GF);
	dpd_file2_mat_rd(&GF);
	for(int h = 0; h < nirrep_; ++h){
	  for(int i = 0 ; i < occpiB[h]; ++i){
            for(int a = 0 ; a < virtpiB[h]; ++a){
                GFockB->add(h, i, a + occpiB[h], GF.matrix[h][i][a]);
            }
	  }
	}
	dpd_file2_close(&GF);
	psio_->close(PSIF_OCC_DENSITY, 1);

        // Print
	if (print_ > 1) {
	  GFockA->print();
	  GFockB->print();
	}
	
}// end if (reference_ == "UNRESTRICTED") 
//fprintf(outfile,"\n omp3_gfock done. \n"); fflush(outfile);

} // End main
}} // End Namespaces
