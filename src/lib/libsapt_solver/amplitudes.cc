#include "sapt2.h"

namespace psi { namespace sapt {

void SAPT2::amplitudes()
{
  tOVOV(PSIF_SAPT_AA_DF_INTS,"AR RI Integrals",foccA_,noccA_,nvirA_,evalsA_,
    PSIF_SAPT_AA_DF_INTS,"AR RI Integrals",foccA_,noccA_,nvirA_,evalsA_,
    PSIF_SAPT_AMPS,"tARAR Amplitudes");
  tOVOV(PSIF_SAPT_BB_DF_INTS,"BS RI Integrals",foccB_,noccB_,nvirB_,evalsB_,
    PSIF_SAPT_BB_DF_INTS,"BS RI Integrals",foccB_,noccB_,nvirB_,evalsB_,
    PSIF_SAPT_AMPS,"tBSBS Amplitudes");

  tOVOV(PSIF_SAPT_AA_DF_INTS,"AR RI Integrals",foccA_,noccA_,nvirA_,evalsA_,
    PSIF_SAPT_BB_DF_INTS,"BS RI Integrals",foccB_,noccB_,nvirB_,evalsB_,
    PSIF_SAPT_AMPS,"tARBS Amplitudes");

  pOOpVV(PSIF_SAPT_AMPS,"tARAR Amplitudes","tARAR Amplitudes",aoccA_,nvirA_,
    PSIF_SAPT_AMPS,"pAA Density Matrix","pRR Density Matrix");
  pOOpVV(PSIF_SAPT_AMPS,"tBSBS Amplitudes","tBSBS Amplitudes",aoccB_,nvirB_,
    PSIF_SAPT_AMPS,"pBB Density Matrix","pSS Density Matrix");

  theta(PSIF_SAPT_AMPS,"tARAR Amplitudes",'N',true,aoccA_,nvirA_,aoccA_,nvirA_,
    "AR RI Integrals",PSIF_SAPT_AMPS,"Theta AR Intermediates");
  theta(PSIF_SAPT_AMPS,"tBSBS Amplitudes",'N',true,aoccB_,nvirB_,aoccB_,nvirB_,
    "BS RI Integrals",PSIF_SAPT_AMPS,"Theta BS Intermediates");

  theta(PSIF_SAPT_AMPS,"tARBS Amplitudes",'N',false,aoccA_,nvirA_,
    aoccB_,nvirB_,"BS RI Integrals",PSIF_SAPT_AMPS,"T AR Intermediates");
  theta(PSIF_SAPT_AMPS,"tARBS Amplitudes",'T',false,aoccA_,nvirA_,
    aoccB_,nvirB_,"AR RI Integrals",PSIF_SAPT_AMPS,"T BS Intermediates");

  Y2(PSIF_SAPT_AA_DF_INTS,"AA RI Integrals","AR RI Integrals",
    "RR RI Integrals",PSIF_SAPT_AMPS,"pAA Density Matrix","pRR Density Matrix",
    "Theta AR Intermediates",foccA_,noccA_,nvirA_,evalsA_,PSIF_SAPT_AMPS,
    "Y2 AR Amplitudes","T2 AR Amplitudes");
  Y2(PSIF_SAPT_BB_DF_INTS,"BB RI Integrals","BS RI Integrals",
    "SS RI Integrals",PSIF_SAPT_AMPS,"pBB Density Matrix","pSS Density Matrix",
    "Theta BS Intermediates",foccB_,noccB_,nvirB_,evalsB_,PSIF_SAPT_AMPS,
    "Y2 BS Amplitudes","T2 BS Amplitudes");
}

void SAPT2::tOVOV(int intfileA, const char *ARlabel, int foccA, int noccA,
  int nvirA, double *evalsA, int intfileB, const char *BSlabel, int foccB,
  int noccB, int nvirB, double *evalsB, int ampout, const char *amplabel)
{
  int aoccA = noccA - foccA;
  int aoccB = noccB - foccB;

  double **B_p_AR = get_DF_ints(intfileA,ARlabel,foccA,noccA,0,nvirA);
  double **B_p_BS = get_DF_ints(intfileB,BSlabel,foccB,noccB,0,nvirB);
  double **tARBS = block_matrix(aoccA*nvirA,aoccB*nvirB);

  C_DGEMM('N','T',aoccA*nvirA,aoccB*nvirB,ndf_,1.0,B_p_AR[0],ndf_+3,
    B_p_BS[0],ndf_+3,0.0,tARBS[0],aoccB*nvirB);

  for (int a=0, ar=0; a<aoccA; a++){
    for (int r=0; r<nvirA; r++, ar++){
      for (int b=0, bs=0; b<aoccB; b++){
        for (int s=0; s<nvirB; s++, bs++){
          tARBS[ar][bs] /= evalsA[a+foccA]+evalsB[b+foccB]-evalsA[r+noccA]-
            evalsB[s+noccB];
  }}}}

  psio_->write_entry(ampout,amplabel,(char *) tARBS[0],sizeof(double)*aoccA*
    nvirA*aoccB*nvirB); 

  free_block(B_p_AR);
  free_block(B_p_BS);
  free_block(tARBS);
}

void SAPT2::pOOpVV(int ampfile, const char *amplabel, const char *thetalabel,
  int aoccA, int nvirA, int ampout, const char *OOlabel, const char *VVlabel)
{
  double **tARAR = block_matrix(aoccA*nvirA,aoccA*nvirA);
  psio_->read_entry(ampfile,amplabel,(char *) tARAR[0],sizeof(double)*aoccA*
    nvirA*aoccA*nvirA);

  double **thetaARAR = block_matrix(aoccA*nvirA,aoccA*nvirA);
  psio_->read_entry(ampfile,thetalabel,(char *) thetaARAR[0],
    sizeof(double)*aoccA*nvirA*aoccA*nvirA);
  antisym(thetaARAR,aoccA,nvirA);

  double **pOO = block_matrix(aoccA,aoccA);
  double **pVV = block_matrix(nvirA,nvirA);

  C_DGEMM('N','T',aoccA,aoccA,nvirA*aoccA*nvirA,1.0,tARAR[0],nvirA*aoccA*nvirA,
    thetaARAR[0],nvirA*aoccA*nvirA,0.0,pOO[0],aoccA);

  C_DGEMM('T','N',nvirA,nvirA,aoccA*nvirA*aoccA,1.0,tARAR[0],nvirA,
    thetaARAR[0],nvirA,0.0,pVV[0],nvirA);

  psio_->write_entry(ampout,OOlabel,(char *) pOO[0],
    sizeof(double)*aoccA*aoccA);
  psio_->write_entry(ampout,VVlabel,(char *) pVV[0],
    sizeof(double)*nvirA*nvirA);

  free_block(tARAR);
  free_block(thetaARAR);
  free_block(pOO);
  free_block(pVV);
}

void SAPT2::theta(int ampfile, const char *amplabel, const char trans, 
  bool antisymmetrized, int aoccA, int nvirA, int aoccB, int nvirB, 
  const char *OVlabel, int ampout, const char *thetalabel)
{
  double **tARBS = block_matrix(aoccA*nvirA,aoccB*nvirB);
  psio_->read_entry(ampfile,amplabel,(char *) tARBS[0],sizeof(double)*aoccA*
    nvirA*aoccB*nvirB);
  
  if (antisymmetrized)
    antisym(tARBS,aoccA,nvirA);

  double **B_p_OV; 
  if (OVlabel == "AR RI Integrals") {
    B_p_OV = get_DF_ints(PSIF_SAPT_AA_DF_INTS,"AR RI Integrals",
      foccA_,noccA_,0,nvirA_);

    for (int a=foccA_, ar=0; a<noccA_; a++){
      for (int r=0; r<nvirA_; r++, ar++){
        B_p_OV[ar][ndf_+1] = vBAA_[a][r+noccA_]/(double) NB_;
      }
    }
  }
  else if (OVlabel == "BS RI Integrals") {
    B_p_OV = get_DF_ints(PSIF_SAPT_BB_DF_INTS,"BS RI Integrals",
      foccB_,noccB_,0,nvirB_);

    for (int b=foccB_, bs=0; b<noccB_; b++){
      for (int s=0; s<nvirB_; s++, bs++){
        B_p_OV[bs][ndf_] = vABB_[b][s+noccB_]/(double) NA_;
      }
    }
  }
  else 
    throw PsiException("Those integrals do not exist",__FILE__,__LINE__);

  if (trans == 'N' || trans == 'n') {
    double **T_p_OV = block_matrix(aoccA*nvirA,ndf_+3);

    C_DGEMM('N','N',aoccA*nvirA,ndf_+3,aoccB*nvirB,1.0,tARBS[0],aoccB*nvirB,
      B_p_OV[0],ndf_+3,0.0,T_p_OV[0],ndf_+3);

    psio_->write_entry(ampout,thetalabel,(char *) T_p_OV[0],
      sizeof(double)*aoccA*nvirA*(ndf_+3));

    free_block(T_p_OV);
  }
  else if (trans == 'T' || trans == 't') {
    double **T_p_OV = block_matrix(aoccB*nvirB,ndf_+3);

    C_DGEMM('T','N',aoccB*nvirB,ndf_+3,aoccA*nvirA,1.0,tARBS[0],aoccB*nvirB,
      B_p_OV[0],ndf_+3,0.0,T_p_OV[0],ndf_+3);

    psio_->write_entry(ampout,thetalabel,(char *) T_p_OV[0],
      sizeof(double)*aoccB*nvirB*(ndf_+3));

    free_block(T_p_OV);
  }
  else 
    throw PsiException("You want me to do what to that matrix?",
       __FILE__,__LINE__);

  free_block(tARBS);
  free_block(B_p_OV);
}

void SAPT2::Y2(int intfile, const char *AAlabel, const char *ARlabel, 
  const char *RRlabel, int ampfile, const char *pAAlabel, const char *pRRlabel,
  const char *thetalabel, int foccA, int noccA, int nvirA, double *evals, 
  int ampout, const char *Ylabel, const char *tlabel)
{
  int aoccA = noccA - foccA;

  double **yAR = block_matrix(aoccA,nvirA);
  double **tAR = block_matrix(aoccA,nvirA);

  Y2_3(yAR,intfile,AAlabel,RRlabel,ampfile,thetalabel,foccA,noccA,nvirA);

  C_DCOPY(aoccA*nvirA,yAR[0],1,tAR[0],1);

  for (int a=0; a<aoccA; a++) {
    for (int r=0; r<nvirA; r++) {
      tAR[a][r] /= evals[a+foccA] - evals[r];
  }}

  psio_->write_entry(ampfile,tlabel,(char *) tAR[0],
    sizeof(double)*aoccA*nvirA);

  free_block(tAR);

  Y2_1(yAR,intfile,ARlabel,RRlabel,ampfile,pRRlabel,foccA,noccA,nvirA);
  Y2_2(yAR,intfile,AAlabel,ARlabel,ampfile,pAAlabel,foccA,noccA,nvirA);

  psio_->write_entry(ampout,Ylabel,(char *) yAR[0],
    sizeof(double)*aoccA*nvirA);

  free_block(yAR);
}

void SAPT2::Y2_1(double **yAR, int intfile, const char *ARlabel, 
  const char *RRlabel, int ampfile, const char *pRRlabel, int foccA, 
  int noccA, int nvirA)
{
  int aoccA = noccA - foccA;

  double **pRR = block_matrix(nvirA,nvirA);
  psio_->read_entry(ampfile,pRRlabel,(char *) pRR[0],
    sizeof(double)*nvirA*nvirA);

  double **B_p_AR = get_DF_ints(intfile,ARlabel,foccA,noccA,0,nvirA);
  double **B_p_RR = get_DF_ints(intfile,RRlabel,0,nvirA,0,nvirA);

  double *X = init_array(ndf_+3);
  double **C_p_AR = block_matrix(aoccA*nvirA,ndf_+3);

  C_DGEMV('t',nvirA*nvirA,ndf_+3,1.0,B_p_RR[0],ndf_+3,pRR[0],1,0.0,X,1);

  for (int a=0; a<aoccA; a++) {
    C_DGEMM('T','N',nvirA,ndf_+3,nvirA,1.0,pRR[0],nvirA,B_p_AR[a*nvirA],ndf_+3,
      0.0,C_p_AR[a*nvirA],ndf_+3); 
  }

  C_DGEMV('n',aoccA*nvirA,ndf_+3,2.0,B_p_AR[0],ndf_+3,X,1,1.0,yAR[0],1);

  C_DGEMM('N','T',aoccA,nvirA,nvirA*(ndf_+3),-1.0,C_p_AR[0],nvirA*(ndf_+3),
    B_p_RR[0],nvirA*(ndf_+3),1.0,yAR[0],nvirA);

  free(X);
  free_block(pRR);
  free_block(B_p_AR);
  free_block(C_p_AR);
  free_block(B_p_RR);
}

void SAPT2::Y2_2(double **yAR, int intfile, const char *AAlabel,
  const char *ARlabel, int ampfile, const char *pAAlabel, int foccA,
  int noccA, int nvirA)
{
  int aoccA = noccA - foccA;

  double **pAA = block_matrix(aoccA,aoccA);
  psio_->read_entry(ampfile,pAAlabel,(char *) pAA[0],
    sizeof(double)*aoccA*aoccA);

  double **B_p_AA = get_DF_ints(intfile,AAlabel,foccA,noccA,foccA,noccA);
  double **B_p_AR = get_DF_ints(intfile,ARlabel,foccA,noccA,0,nvirA);

  double *X = init_array(ndf_+3);
  double **C_p_AA = block_matrix(aoccA*aoccA,ndf_+3);

  C_DGEMV('t',aoccA*aoccA,ndf_+3,1.0,B_p_AA[0],ndf_+3,pAA[0],1,0.0,X,1);

  C_DGEMM('N','N',aoccA,aoccA*(ndf_+3),aoccA,1.0,pAA[0],aoccA,B_p_AA[0],
    aoccA*(ndf_+3),0.0,C_p_AA[0],aoccA*(ndf_+3));

  C_DGEMV('n',aoccA*nvirA,ndf_+3,-2.0,B_p_AR[0],ndf_+3,X,1,1.0,yAR[0],1);

  for (int a=0; a<aoccA; a++) {
    C_DGEMM('N','T',aoccA,nvirA,ndf_+3,1.0,C_p_AA[a*aoccA],ndf_+3,
      B_p_AR[a*nvirA],ndf_+3,1.0,yAR[0],nvirA);
  }

  free(X);
  free_block(pAA);
  free_block(B_p_AA);
  free_block(C_p_AA);
  free_block(B_p_AR);
}

void SAPT2::Y2_3(double **yAR, int intfile, const char *AAlabel, 
  const char *RRlabel, int ampfile, const char *thetalabel, int foccA, 
  int noccA, int nvirA)
{
  int aoccA = noccA - foccA;

  double **T_p_AR = block_matrix(aoccA*nvirA,ndf_+3);
  psio_->read_entry(ampfile,thetalabel,(char *) T_p_AR[0],
    sizeof(double)*aoccA*nvirA*(ndf_+3));

  double **B_p_AA = get_DF_ints(intfile,AAlabel,foccA,noccA,foccA,noccA);
  double **B_p_RR = get_DF_ints(intfile,RRlabel,0,nvirA,0,nvirA);

  for (int a=0; a<aoccA; a++) {
    C_DGEMM('N','T',aoccA,nvirA,ndf_+3,-1.0,B_p_AA[a*aoccA],ndf_+3,
      T_p_AR[a*nvirA],ndf_+3,1.0,yAR[0],nvirA);
  }

  C_DGEMM('N','T',aoccA,nvirA,nvirA*(ndf_+3),1.0,T_p_AR[0],nvirA*(ndf_+3),
    B_p_RR[0],nvirA*(ndf_+3),1.0,yAR[0],nvirA);

  free_block(B_p_AA);
  free_block(T_p_AR);
  free_block(B_p_RR);
}

}}
