/* Copyright (C) 2012,2013 IBM Corp.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#if defined(__unix__) || defined(__unix) || defined(unix)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <NTL/ZZ.h>
NTL_CLIENT
#include "EncryptedArray.h"
//#include "EvalMap.h"
#include "AltEvalMap.h"
#include "powerful.h"

//#define DEBUG_PRINTOUT
#ifdef DEBUG_PRINTOUT
FHESecKey* dbgKey=NULL;
EncryptedArray* dbgEa=NULL;
ZZX dbg_ptxt;
Vec<ZZ> ptxt_pwr;
#endif

#define FLAG_PRINT_ZZX  1
#define FLAG_PRINT_POLY 2
#define FLAG_PRINT_VEC  4
void decryptAndPrint(ostream& s, const Ctxt& ctxt, const FHESecKey& sk,
		     const EncryptedArray& ea, long flags=0);
template<class T> ostream& printVec(ostream& s, const Vec<T>& v, long nCoeffs=40);
ostream& printZZX(ostream& s, const ZZX& poly, long nCoeffs=40);
void baseRep(Vec<long>& rep, long nDigits, ZZ num, long base=2);

static long mValues[][13] = { 
//{ p, phi(m),  m,    d, m1,  m2, m3,   g1,    g2,    g3,ord1,ord2,ord3}
  {  2,   600,  1023, 10, 11,  93,  0,   838,   584,    0, 10,  6,   0}, // m=(3)*11*{31} m/phim(m)=1.7    C=24  D=2 E=1
  {  2,  1200,  1705, 20, 11, 155,  0,   156,   936,    0, 10,  6,   0}, // m=(5)*11*{31} m/phim(m)=1.42   C=34  D=2 E=2
  {  2, 12800, 17425, 40, 41, 425,  0,  5951,  8078,    0, 40, -8,   0}, // m=(5^2)*{17}*41 m/phim(m)=1.36 C=93  D=3 E=3
  {  2, 15004, 15709, 22, 23, 683,  0,  4099, 13663,    0, 22, 31,   0}, // m=23*(683) m/phim(m)=1.04      C=73  D=2 E=1
  {  2, 16384, 21845, 16, 17,   5,257,  8996, 17477, 21591, 16, 4, -16}, // m=5*17*(257) :-( m/phim(m)=1.33 C=65 D=4 E=4
  {  2, 18000, 18631, 25, 31, 601,  0, 15627,  1334,    0, 30, 24,   0}, // m=31*(601) m/phim(m)=1.03      C=77  D=2 E=0
  {  2, 18816, 24295, 28, 43, 565,  0, 16386, 16427,    0, 42, 16,   0}, // m=(5)*43*{113} m/phim(m)=1.29  C=84  D=2 E=2
  {  2, 21168, 27305, 28, 43, 635,  0, 10796, 26059,    0, 42, 18,   0}, // m=(5)*43*{127} m/phim(m)=1.28  C=86  D=2 E=2
  {  2, 23040, 28679, 24, 17,  7, 241, 15184,  4098,28204, 16,  6, -10}, // m=7*17*(241) m/phim(m)=1.24    C=63  D=4 E=3
  {  2, 24000, 31775, 20, 41, 775,  0,  6976, 24806,    0, 40, 30,   0}, // m=(5^2)*{31}*41 m/phim(m)=1.32 C=88  D=2 E=2
  {  2, 26400, 27311, 55, 31, 881,  0, 21145,  1830,    0, 30, 16,   0}, // m=31*(881) m/phim(m)=1.03      C=99  D=2 E=0
  {  2, 31104, 35113, 36, 37, 949,  0, 16134,  8548,    0, 36, 24,   0}, // m=(13)*37*{73} m/phim(m)=1.12  C=94  D=2 E=2
  {  2, 34848, 45655, 44, 23, 1985, 0, 33746, 27831,    0, 22, 36,   0}, // m=(5)*23*{397} m/phim(m)=1.31  C=100 D=2 E=2
  {  2, 42336, 42799, 21, 127, 337, 0, 25276, 40133,    0,126, 16,   0}, // m=127*(337) m/phim(m)=1.01     C=161 D=2 E=0
  {  2, 45360, 46063, 45, 73, 631,  0, 35337, 20222,    0, 72, 14,   0}, // m=73*(631) m/phim(m)=1.01      C=129 D=2 E=0
  {  2, 46080, 53261, 24, 17, 13, 241, 43863, 28680,15913, 16, 12, -10}, // m=13*17*(241) m/phim(m)=1.15   C=69  D=4 E=3
  {  2, 49500, 49981, 30, 151, 331, 0,  6952, 28540,    0,150, 11,   0}, // m=151*(331) m/phim(m)=1        C=189 D=2 E=1
  {  2, 54000, 55831, 25, 31, 1801, 0, 19812, 50593,    0, 30, 72,   0}, // m=31*(1801) m/phim(m)=1.03     C=125 D=2 E=0
  {  2, 60016, 60787, 22, 89, 683,  0,  2050, 58741,    0, 88, 31,   0}, // m=89*(683) m/phim(m)=1.01      C=139 D=2 E=1

  { 17,   576,  1365, 12,  7,   3, 65,   976,   911,  463,  6,  2,   4}, // m=3*(5)*7*{13} m/phim(m)=2.36  C=22  D=3
  { 17, 18000, 21917, 30, 101, 217, 0,  5860,  5455,    0, 100, 6,  0}, // m=(7)*{31}*101 m/phim(m)=1.21  C=134 D=2 
  { 17, 30000, 34441, 30, 101, 341, 0,  2729, 31715,    0, 100, 10,  0}, // m=(11)*{31}*101 m/phim(m)=1.14 C=138 D=2
  { 17, 40000, 45551, 40, 101, 451, 0, 19394,  7677,    0, 100, 10,  0}, // m=(11)*{41}*101 m/phim(m)=1.13 C=148 D=2
  { 17, 46656, 52429, 36, 109, 481, 0, 46658,  5778,    0, 108, 12,  0}, // m=(13)*{37}*109 m/phim(m)=1.12 C=154 D=2
  { 17, 54208, 59363, 44, 23, 2581, 0, 25811,  5199,    0, 22, 56,   0}, // m=23*(29)*{89} m/phim(m)=1.09  C=120 D=2
  { 17, 70000, 78881, 10, 101, 781, 0, 67167, 58581,    0, 100, 70,  0}, // m=(11)*{71}*101 m/phim(m)=1.12 C=178 D=2

  {127,   576,  1365, 12,  7,   3, 65,   976,   911,  463,  6,  2,   4}, // m=3*(5)*7*{13} m/phim(m)=2.36   C=22  D=3
  {127,  1200,  1925, 20,  11, 175, 0,  1751,   199,    0, 10, 6,    0}, //  m=(5^2)*{7}*11 m/phim(m)=1.6   C=34 D=2
  {127,  2160,  2821, 30,  13, 217, 0,   652,   222,    0, 12, 6,    0}, // m=(7)*13*{31} m/phim(m)=1.3     C=46 D=2

  {127, 18816, 24295, 28, 43, 565,  0, 16386, 16427,    0, 42, 16,   0}, // m=(5)*43*{113} m/phim(m)=1.29   C=84  D=2
  {127, 26112, 30277, 24, 17, 1781, 0, 14249, 10694,    0, 16, 68,   0}, // m=(13)*17*{137} m/phim(m)=1.15  C=106 D=2
  {127, 39168, 62335, 24, 7,  5, 1781, 17811, 37402,49876,  6,  4,  68}, // m=5*7*(13)*{137} m/phim(m)=1.59 C=100 D=3
  {127, 49392, 61103, 28, 43, 1421, 0,  1422, 14234,    0, 42, 42,   0}, // m=(7^2)*{29}*43 m/phim(m)=1.23  C=110 D=2
  {127, 54400, 61787, 40, 41, 1507, 0, 30141, 46782,    0, 40, 34,   0}, // m=(11)*41*{137} m/phim(m)=1.13  C=112 D=2
  {127, 72000, 77531, 30, 61, 1271, 0,  7627, 34344,    0, 60, 40,   0}  // m=(31)*{41}*61 m/phim(m)=1.07   C=128 D=2
};
#define num_mValues (sizeof(mValues)/(13*sizeof(long)))


void TestIt(long idx, long p, long r, long L, long c, long B)
{
  Vec<long> mvec;
  vector<long> gens;
  vector<long> ords;

  long phim = mValues[idx][1];
  long m = mValues[idx][2];
  assert(GCD(p, m) == 1);

  append(mvec, mValues[idx][4]);
  append(mvec, mValues[idx][5]);
  if (mValues[idx][6]>1) append(mvec, mValues[idx][6]);
  gens.push_back(mValues[idx][7]);
  gens.push_back(mValues[idx][8]);
  if (mValues[idx][9]>1) gens.push_back(mValues[idx][9]);
  ords.push_back(mValues[idx][10]);
  ords.push_back(mValues[idx][11]);
  if (abs(mValues[idx][12])>1) ords.push_back(mValues[idx][12]);

  cout << "*** TestIt";
  if (DoubleCRT::dryRun) cout << " (dry run)";
  cout << ": p=" << p
       << ", r=" << r
       << ", L=" << L
       << ", B=" << B
       << ", c=" << c
       << ", m=" << m
       << " (=" << mvec << "), gens="<<gens<<", ords="<<ords
       << endl;

  setTimersOn();
  cout << "Computing key-independent tables..." << std::flush;
  double t = -GetTime();
  FHEcontext context(m, p, r, gens, ords);
  context.bitsPerLevel = B;
  buildModChain(context, L, c);
  context.makeBootstrappable(mvec);
  t += GetTime();
  cout << " done in "<<t<<" seconds\n";
  context.zMStar.printout();
  long nPrimes = context.numPrimes();
  IndexSet allPrimes(0,nPrimes-1);
  double bitsize = context.logOfProduct(allPrimes)/log(2.0);
  cout << "  "<<nPrimes<<" primes in chain, total bitsize="
       << ceil(bitsize) << ", secparam="
       << (7.2*phim/bitsize -110) << endl;

  long p2r = context.alMod.getPPowR();

  for (long numkey=0; numkey<3; numkey++) { // test with 3 keys

  t = -GetTime();
  cout << "Generating keys, " << std::flush;
  FHESecKey secretKey(context);
  FHEPubKey& publicKey = secretKey;
  secretKey.GenSecKey(64);      // A Hamming-weight-64 secret key
  addSome1DMatrices(secretKey); // compute key-switching matrices that we need
  addFrbMatrices(secretKey);
  cout << "computing key-dependent tables..." << std::flush;
  secretKey.genRecryptData();
  t += GetTime();
  cout << " done in "<<t<<" seconds\n";

  zz_p::init(p2r);
  zz_pX poly_p = random_zz_pX(context.zMStar.getPhiM());
  PowerfulConversion pConv(context.rcData.p2dConv->getIndexTranslation());
  HyperCube<zz_p> powerful(pConv.getShortSig());
  pConv.polyToPowerful(powerful, poly_p);
  ZZX ptxt_poly = conv<ZZX>(poly_p);
  PolyRed(ptxt_poly, p2r, true); // reduce to the symmetric interval

#ifdef DEBUG_PRINTOUT
  dbgKey = &secretKey; // debugging key and ea
  dbgEa = context.rcData.ea; // EA for plaintext space p^{e+r-e'}
  dbg_ptxt = ptxt_poly;
  context.rcData.p2dConv->ZZXtoPowerful(ptxt_pwr, dbg_ptxt);
  vecRed(ptxt_pwr, ptxt_pwr, p2r, true);
#endif

  ZZX poly2;
  Ctxt c1(publicKey);

  secretKey.Encrypt(c1,ptxt_poly,p2r);
  for (long num=0; num<2; num++) { // recrypt twice, each time test the result
    publicKey.reCrypt(c1);
    secretKey.Decrypt(poly2,c1);

    if (ptxt_poly == poly2) cout << "  *** reCryption succeeds!!\n";
    else if (!DoubleCRT::dryRun) { // bootsrtapping error
      conv(poly_p,poly2);
      HyperCube<zz_p> powerful2(pConv.getShortSig());
      cout << "\ndecryption error, encrypted ";
      printVec(cout, powerful.getData())<<endl;

      pConv.polyToPowerful(powerful2, poly_p);
      cout << "                after reCrypt ";
      printVec(cout, powerful2.getData())<<endl;
      long numDiff = 0;
      for (long i=0; i<powerful.getSize(); i++) 
	if (powerful[i] != powerful2[i]) {
          numDiff++;
	  cout << i << ": " << powerful[i] << " != " << powerful2[i]<<", ";
	  if (numDiff >5) break;
        }
      cout << endl<< endl;
      printAllTimers();
      exit(0);
    }

    decryptAndPrint(cout, c1, secretKey, *context.ea);
    cout << endl;
  }
  }
  printAllTimers();
  resetAllTimers();
#if (defined(__unix__) || defined(__unix) || defined(unix))
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
    cout << "  rusage.ru_maxrss="<<rusage.ru_maxrss << endl;
#endif
}

/********************************************************************
 ********************************************************************/
void usage(char *prog) 
{
  cerr << "Usage: "<<prog<<" [ optional parameters ]\n";
  cerr << "  optional parameters have the form 'attr1=val1 attr2=val2 ...'\n";
  cerr << "  p,r determines plaintext space mod p^r [default=2^1]\n";
  cerr << "  B is the number of bit per level [default="
       << FHE_pSize << "]\n";
  cerr << "  L is # of primes in the chain [default=20]\n";
  cerr << "  c is the number of digits for key-switching [default=3]\n";
  cerr << "  N is a lower bound on phi(m) [default=0]\n";
  cerr << "  dry=1 for a dry run [default=0]\n";
  exit(0);
}

int main(int argc, char *argv[]) 
{
  argmap_t argmap;
  argmap["p"] = "2";
  argmap["r"] = "1";
  argmap["c"] = "3";
  argmap["B"] = "0";
  argmap["L"] = "15";
  argmap["N"] = "0";
  argmap["dry"] = "0";

  // get parameters from the command line
  if (!parseArgs(argc, argv, argmap)) usage(argv[0]);

  long p = atoi(argmap["p"]);
  long r = atoi(argmap["r"]);
  long c = atoi(argmap["c"]);
  long L =  atoi(argmap["L"]);
  long N =  atoi(argmap["N"]);
  long B =  atoi(argmap["B"]);
  if (B<=0) B=FHE_pSize;
  if (B>NTL_SP_NBITS/2) B = NTL_SP_NBITS/2;

  DoubleCRT::dryRun = (atoi(argmap["dry"]) != 0);
  for (long i=0; i<(long)num_mValues; i++)
    if (mValues[i][0]==p && mValues[i][1]>=N) {
      TestIt(i,p,r,L,c,B);
      break;
    }
  return 0;
}




/************************************/
/*********** Utilities **************/
/************************************/

template<class T> ostream& printVec(ostream& s, const Vec<T>& v, long nCoeffs)
{
  long d = v.length();
  if (d<nCoeffs) return s << v; // just print the whole thing

  // otherwise print only 1st nCoeffs coefficiants
  s << '[';
  for (long i=0; i<nCoeffs-2; i++) s << v[i] << ' ';
  s << "... " << v[d-2] << ' ' << v[d-1] << ']';
  return s;
}

ostream& printZZX(ostream& s, const ZZX& poly, long nCoeffs)
{
  return printVec(s, poly.rep, nCoeffs);
  /*  long d = deg(poly);
  if (d<nCoeffs) return s << poly; // just print the whole thing

  // otherwise print only 1st nCoeffs coefficiants
  s << '[';
  for (long i=0; i<nCoeffs-2; i++) s << poly[i] << ' ';
  s << "... " << poly[d-1] << ' ' << poly[d] << ']';
  return s; */
}

void baseRep(Vec<long>& rep, long nDigits, ZZ num, long base)
{
  rep.SetLength(nDigits);
  for (long j=0; j<nDigits; j++) {
    rep[j] = rem(num, base);
    if (rep[j] > base/2)         rep[j] -= base;
    else if (rep[j] < -(base/2)) rep[j] += base;
    num = (num - rep[j]) / base;
  }
}

void decryptAndPrint(ostream& s, const Ctxt& ctxt, const FHESecKey& sk,
		     const EncryptedArray& ea, long flags)
{
  const FHEcontext& context = ctxt.getContext();
  xdouble noiseEst = sqrt(ctxt.getNoiseVar());
  xdouble modulus = xexp(context.logOfProduct(ctxt.getPrimeSet()));
  vector<ZZX> ptxt;
  ZZX p, pp;
  sk.Decrypt(p, ctxt, pp);

  s << "plaintext space mod "<<ctxt.getPtxtSpace()
       << ", level="<<ctxt.findBaseLevel()
       << ", \n           |noise|=q*" << (coeffsL2Norm(pp)/modulus)
       << ", |noiseEst|=q*" << (noiseEst/modulus)
       <<endl;

  if (flags & FLAG_PRINT_ZZX) {
    s << "   before mod-p reduction=";
    printZZX(s,pp) <<endl;
  }
  if (flags & FLAG_PRINT_POLY) {
    s << "   after mod-p reduction=";
    printZZX(s,p) <<endl;
  }
  if (flags & FLAG_PRINT_VEC) {
    ea.decode(ptxt, p);
    if (ea.getAlMod().getTag() == PA_zz_p_tag
	&& ctxt.getPtxtSpace() != ea.getAlMod().getPPowR()) {
      long g = GCD(ctxt.getPtxtSpace(), ea.getAlMod().getPPowR());
      for (long i=0; i<ea.size(); i++)
	PolyRed(ptxt[i], g, true);
    }
    s << "   decoded to ";
    if (deg(p) < 40) // just pring the whole thing
      s << ptxt << endl;
    else if (ptxt.size()==1) // a single slot
      printZZX(s, ptxt[0]) <<endl;
    else { // print first and last slots
      printZZX(s, ptxt[0],20) << "--";
      printZZX(s, ptxt[ptxt.size()-1], 20) <<endl;      
    }
  }
}