/******************************************************************************
 *                       Code generated with sympy 1.4                        *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                  This file is part of 'PrecipitateAging'                   *
 ******************************************************************************/
#include "parabola625.h"
#include <math.h>

double h(double x)
{

	double h_result;
	h_result = pow(x, 3)*(6.0*pow(x, 2) - 15.0*x + 10.0);
	return h_result;

}

double hprime(double x)
{

	double hprime_result;
	hprime_result = 30.0*pow(x, 2)*pow(1.0 - x, 2);
	return hprime_result;

}

double interface_profile(double z)
{

	double interface_profile_result;
	interface_profile_result = 1.0/2.0 - 1.0/2.0*tanh(z);
	return interface_profile_result;

}

double kT()
{

	double kT_result;
	kT_result = 1.5782883556379999e-20;
	return kT_result;

}

double RT()
{

	double RT_result;
	RT_result = 9504.6886668000006;
	return RT_result;

}

double Vm()
{

	double Vm_result;
	Vm_result = 1.0000000000000001e-5;
	return Vm_result;

}

double xe_gam_Cr()
{

	double xe_gam_Cr_result;
	xe_gam_Cr_result = 0.55855269488699388;
	return xe_gam_Cr_result;

}

double xe_gam_Nb()
{

	double xe_gam_Nb_result;
	xe_gam_Nb_result = 0.010717747618334031;
	return xe_gam_Nb_result;

}

double xe_del_Cr()
{

	double xe_del_Cr_result;
	xe_del_Cr_result = 0.031881757028651196;
	return xe_del_Cr_result;

}

double xe_del_Nb()
{

	double xe_del_Nb_result;
	xe_del_Nb_result = 0.16874796371854489;
	return xe_del_Nb_result;

}

double xe_lav_Cr()
{

	double xe_lav_Cr_result;
	xe_lav_Cr_result = 0.44421217837672827;
	return xe_lav_Cr_result;

}

double xe_lav_Nb()
{

	double xe_lav_Nb_result;
	xe_lav_Nb_result = 0.17170586512707406;
	return xe_lav_Nb_result;

}

double matrix_min_Cr()
{

	double matrix_min_Cr_result;
	matrix_min_Cr_result = 0.27939999999999998;
	return matrix_min_Cr_result;

}

double matrix_max_Cr()
{

	double matrix_max_Cr_result;
	matrix_max_Cr_result = 0.32879999999999998;
	return matrix_max_Cr_result;

}

double matrix_min_Nb()
{

	double matrix_min_Nb_result;
	matrix_min_Nb_result = 0.021499999999999998;
	return matrix_min_Nb_result;

}

double matrix_max_Nb()
{

	double matrix_max_Nb_result;
	matrix_max_Nb_result = 0.0269;
	return matrix_max_Nb_result;

}

double enrich_min_Cr()
{

	double enrich_min_Cr_result;
	enrich_min_Cr_result = 0.24729999999999999;
	return enrich_min_Cr_result;

}

double enrich_max_Cr()
{

	double enrich_max_Cr_result;
	enrich_max_Cr_result = 0.29670000000000002;
	return enrich_max_Cr_result;

}

double enrich_min_Nb()
{

	double enrich_min_Nb_result;
	enrich_min_Nb_result = 0.16589999999999999;
	return enrich_min_Nb_result;

}

double enrich_max_Nb()
{

	double enrich_max_Nb_result;
	enrich_max_Nb_result = 0.1726;
	return enrich_max_Nb_result;

}

double xr_gam_Cr(double P_del, double P_lav)
{

	double xr_gam_Cr_result;
	xr_gam_Cr_result = -9.6984079989082258e-10*P_del + 6.3118903213767551e-10*P_lav + 0.55855269488699388;
	return xr_gam_Cr_result;

}

double xr_gam_Nb(double P_del, double P_lav)
{

	double xr_gam_Nb_result;
	xr_gam_Nb_result = 1.0048047396144418e-10*P_del - 1.1106547204101917e-11*P_lav + 0.010717747618334031;
	return xr_gam_Nb_result;

}

double xr_del_Cr(double P_del, double P_lav)
{

	double xr_del_Cr_result;
	xr_del_Cr_result = -6.6194274436412276e-11*P_del - 1.5374687132506466e-10*P_lav + 0.031881757028651196;
	return xr_del_Cr_result;

}

double xr_del_Nb(double P_del, double P_lav)
{

	double xr_del_Nb_result;
	xr_del_Nb_result = -1.9581600075232547e-11*P_del + 3.9815435461767849e-10*P_lav + 0.16874796371854489;
	return xr_del_Nb_result;

}

double xr_lav_Cr(double P_del, double P_lav)
{

	double xr_lav_Cr_result;
	xr_lav_Cr_result = -1.2953044143338369e-9*P_del + 4.3416027339639082e-10*P_lav + 0.44421217837672827;
	return xr_lav_Cr_result;

}

double xr_lav_Nb(double P_del, double P_lav)
{

	double xr_lav_Nb_result;
	xr_lav_Nb_result = 2.6966712244807525e-10*P_del - 6.3173100035954083e-11*P_lav + 0.17170586512707406;
	return xr_lav_Nb_result;

}

double inv_fict_det(double f_del, double f_gam, double f_lav)
{

	double inv_fict_det_result;
	inv_fict_det_result = 10.511929918754895/(0.063619913592430452*pow(f_del, 2) + 0.77472418933378007*f_del*f_gam + 1.0*f_del*f_lav + 0.11406419308318602*pow(f_gam, 2) + 0.29936120577567832*f_gam*f_lav + 0.076367679948657952*pow(f_lav, 2));
	return inv_fict_det_result;

}

double fict_gam_Cr(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_gam_Cr_result;
	fict_gam_Cr_result = 0.081719161132759205*INV_DET*(0.94267830414080844*XCR*f_del + 0.13278315265091145*XCR*f_gam + 0.27475106182917869*XCR*f_lav + 0.48142139214471374*XNB*f_del + 1.0*XNB*f_lav - 0.069926420929843164*pow(f_del, 2) - 0.03219039130480722*f_del*f_gam - 0.028703356204519043*f_del*f_lav - 0.028515077359655715*f_gam*f_lav - 0.24409812910594283*pow(f_lav, 2));
	return fict_gam_Cr_result;

}

double fict_gam_Nb(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_gam_Nb_result;
	fict_gam_Nb_result = -0.010850927847195595*INV_DET*(0.75572285002798056*XCR*f_del - 0.063676254673428662*XCR*f_lav + 0.30738018673156009*XNB*f_del - 1.0*XNB*f_gam - 0.55532654346045196*XNB*f_lav - 0.081941433925801291*pow(f_del, 2) - 0.32945245368491854*f_del*f_gam - 0.38670236006564801*f_del*f_lav + 0.18509555516266776*f_gam*f_lav + 0.11646290035390382*pow(f_lav, 2));
	return fict_gam_Nb_result;

}

double fict_del_Cr(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_del_Cr_result;
	fict_del_Cr_result = 0.046966148206032843*INV_DET*(0.12886223924609808*XCR*f_del - 0.07101626075977073*XCR*f_gam - 0.20028342359736442*XCR*f_lav - 0.83765336993033834*XNB*f_gam - 1.0*XNB*f_lav + 0.12166908885811056*f_del*f_gam + 0.18246772627205363*f_del*f_lav + 0.056009953433358468*pow(f_gam, 2) + 0.31729459226818346*f_gam*f_lav + 0.26560576240348166*pow(f_lav, 2));
	return fict_del_Cr_result;

}

double fict_del_Nb(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_del_Nb_result;
	fict_del_Nb_result = 0.10453655111851477*INV_DET*(0.078444276479274974*XCR*f_gam + 0.20923881653433052*XCR*f_lav + 0.057895185577072183*XNB*f_del + 0.73691813445330934*XNB*f_gam + 1.0*XNB*f_lav - 0.0085055473679878923*f_del*f_gam - 0.031796338023487768*f_del*f_lav - 0.034197271344486177*pow(f_gam, 2) - 0.24299677574697207*f_gam*f_lav - 0.25292500971253778*pow(f_lav, 2));
	return fict_del_Nb_result;

}

double fict_lav_Cr(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_lav_Cr_result;
	fict_lav_Cr_result = 0.10453655111851476*INV_DET*(1.0*XCR*f_del + 0.057643074984273575*XCR*f_gam + 0.0694958662006743*XCR*f_lav + 0.44927967972452598*XNB*f_del - 0.78172811574884349*XNB*f_gam - 0.081979041619570731*pow(f_del, 2) - 0.12011579223114696*f_del*f_gam - 0.11933127186562477*f_del*f_lav + 0.022291037694796172*pow(f_gam, 2) + 0.19081837052380662*f_gam*f_lav);
	return fict_lav_Cr_result;

}

double fict_lav_Nb(double INV_DET, double XCR, double XNB, double f_del, double f_gam, double f_lav)
{

	double fict_lav_Nb_result;
	fict_lav_Nb_result = -0.02643990820696555*INV_DET*(0.82727610358556924*XCR*f_del + 0.026132709676314116*XCR*f_gam + 0.35577056025509707*XNB*f_del - 0.84918699858043711*XNB*f_gam - 0.27476866079657325*XNB*f_lav - 0.12571448770378987*pow(f_del, 2) - 0.80204383828035919*f_del*f_gam - 1.0*f_del*f_lav - 0.075963142465736502*pow(f_gam, 2) - 0.047796328138628434*f_gam*f_lav);
	return fict_lav_Nb_result;

}

double s_delta()
{

	double s_delta_result;
	s_delta_result = 0.13;
	return s_delta_result;

}

double s_laves()
{

	double s_laves_result;
	s_laves_result = 0.13;
	return s_laves_result;

}

double g_gam(double XCR, double XNB)
{

	double g_gam_result;
	g_gam_result = 2005672715.5837781*pow(XCR - 0.55855269488699388, 2) + (14703563128.545319*XCR - 8212714809.8900261)*(XNB - 0.010717747618334031) + 62431733279.319511*pow(XNB - 0.010717747618334031, 2);
	return g_gam_result;

}

double g_del(double XCR, double XNB)
{

	double g_del_result;
	g_del_result = 15567985511.489613*pow(XCR - 0.031881757028651196, 2) + (17972099186.595295*XCR - 572982099.56185102)*(XNB - 0.16874796371854489) + 13383100915.719385*pow(XNB - 0.16874796371854489, 2);
	return g_del_result;

}

double g_lav(double XCR, double XNB)
{

	double g_lav_result;
	g_lav_result = 6897850581.6836567*pow(XCR - 0.44421217837672827, 2) + (57317655210.986305*XCR - 25461200480.718456)*(XNB - 0.17170586512707406) + 134480681274.87074*pow(XNB - 0.17170586512707406, 2);
	return g_lav_result;

}

double dg_gam_dxCr(double XCR, double XNB)
{

	double dg_gam_dxCr_result;
	dg_gam_dxCr_result = 4011345431.1675563*XCR + 14703563128.545319*XNB - 2398136879.4032598;
	return dg_gam_dxCr_result;

}

double dg_gam_dxNb(double XCR, double XNB)
{

	double dg_gam_dxNb_result;
	dg_gam_dxNb_result = 14703563128.545319*XCR + 124863466558.63902*XNB - 9550969931.2158108;
	return dg_gam_dxNb_result;

}

double dg_del_dxCr(double XCR, double XNB)
{

	double dg_del_dxCr_result;
	dg_del_dxCr_result = 31135971022.979225*XCR + 17972099186.595295*XNB - 4025424604.4914207;
	return dg_del_dxCr_result;

}

double dg_del_dxNb(double XCR, double XNB)
{

	double dg_del_dxNb_result;
	dg_del_dxNb_result = 17972099186.595295*XCR + 26766201831.43877*XNB - 5089724155.0967312;
	return dg_del_dxNb_result;

}

double dg_lav_dxCr(double XCR, double XNB)
{

	double dg_lav_dxCr_result;
	dg_lav_dxCr_result = 13795701163.367313*XCR + 57317655210.986305*XNB - 15969996041.071507;
	return dg_lav_dxCr_result;

}

double dg_lav_dxNb(double XCR, double XNB)
{

	double dg_lav_dxNb_result;
	dg_lav_dxNb_result = 57317655210.986305*XCR + 268961362549.74149*XNB - 71643443923.07843;
	return dg_lav_dxNb_result;

}

double d2g_gam_dxCrCr()
{

	double d2g_gam_dxCrCr_result;
	d2g_gam_dxCrCr_result = 4011345431.1675563;
	return d2g_gam_dxCrCr_result;

}

double d2g_gam_dxCrNb()
{

	double d2g_gam_dxCrNb_result;
	d2g_gam_dxCrNb_result = 14703563128.545319;
	return d2g_gam_dxCrNb_result;

}

double d2g_gam_dxNbCr()
{

	double d2g_gam_dxNbCr_result;
	d2g_gam_dxNbCr_result = 14703563128.545319;
	return d2g_gam_dxNbCr_result;

}

double d2g_gam_dxNbNb()
{

	double d2g_gam_dxNbNb_result;
	d2g_gam_dxNbNb_result = 124863466558.63902;
	return d2g_gam_dxNbNb_result;

}

double d2g_del_dxCrCr()
{

	double d2g_del_dxCrCr_result;
	d2g_del_dxCrCr_result = 31135971022.979225;
	return d2g_del_dxCrCr_result;

}

double d2g_del_dxCrNb()
{

	double d2g_del_dxCrNb_result;
	d2g_del_dxCrNb_result = 17972099186.595295;
	return d2g_del_dxCrNb_result;

}

double d2g_del_dxNbCr()
{

	double d2g_del_dxNbCr_result;
	d2g_del_dxNbCr_result = 17972099186.595295;
	return d2g_del_dxNbCr_result;

}

double d2g_del_dxNbNb()
{

	double d2g_del_dxNbNb_result;
	d2g_del_dxNbNb_result = 26766201831.43877;
	return d2g_del_dxNbNb_result;

}

double d2g_lav_dxCrCr()
{

	double d2g_lav_dxCrCr_result;
	d2g_lav_dxCrCr_result = 13795701163.367313;
	return d2g_lav_dxCrCr_result;

}

double d2g_lav_dxCrNb()
{

	double d2g_lav_dxCrNb_result;
	d2g_lav_dxCrNb_result = 57317655210.986305;
	return d2g_lav_dxCrNb_result;

}

double d2g_lav_dxNbCr()
{

	double d2g_lav_dxNbCr_result;
	d2g_lav_dxNbCr_result = 57317655210.986305;
	return d2g_lav_dxNbCr_result;

}

double d2g_lav_dxNbNb()
{

	double d2g_lav_dxNbNb_result;
	d2g_lav_dxNbNb_result = 268961362549.74149;
	return d2g_lav_dxNbNb_result;

}

double D_CrCr(double XCR, double XNB, double phi_del, double phi_lav)
{

	double D_CrCr_result;
	D_CrCr_result = 1.0000000000000001e-5*(pow(XCR, 2)*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB) + pow(XCR, 2)*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) + pow(1 - XCR, 2)*(0.00078144028852173851*XCR*(-XCR - XNB + 1) + 9.1962770419395764e-16*XCR + 3.3292577900637949e-17))*(27124625591.811668*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 27124625591.811668*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 4011345431.1675563) + 1.0000000000000001e-5*(3268536058.0499763*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 3268536058.0499763*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 14703563128.545319)*(XCR*XNB*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) - XCR*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*(1 - XNB) + XNB*(1 - XCR)*(-0.00078144028852173851*XCR*(-XCR - XNB + 1) - 9.1962770419395764e-16*XCR - 3.3292577900637949e-17));
	return D_CrCr_result;

}

double D_CrNb(double XCR, double XNB, double phi_del, double phi_lav)
{

	double D_CrNb_result;
	D_CrNb_result = 1.0000000000000001e-5*(pow(XCR, 2)*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB) + pow(XCR, 2)*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) + pow(1 - XCR, 2)*(0.00078144028852173851*XCR*(-XCR - XNB + 1) + 9.1962770419395764e-16*XCR + 3.3292577900637949e-17))*(3268536058.0499763*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 3268536058.0499763*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 14703563128.545319) + 1.0000000000000001e-5*(-98097264727.200256*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) - 98097264727.200256*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 124863466558.63902)*(XCR*XNB*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) - XCR*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*(1 - XNB) + XNB*(1 - XCR)*(-0.00078144028852173851*XCR*(-XCR - XNB + 1) - 9.1962770419395764e-16*XCR - 3.3292577900637949e-17));
	return D_CrNb_result;

}

double D_NbCr(double XCR, double XNB, double phi_del, double phi_lav)
{

	double D_NbCr_result;
	D_NbCr_result = 1.0000000000000001e-5*(pow(XNB, 2)*(0.00078144028852173851*XCR*(-XCR - XNB + 1) + 9.1962770419395764e-16*XCR + 3.3292577900637949e-17) + pow(XNB, 2)*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) + (1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*pow(1 - XNB, 2))*(3268536058.0499763*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 3268536058.0499763*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 14703563128.545319) + 1.0000000000000001e-5*(27124625591.811668*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 27124625591.811668*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 4011345431.1675563)*(XCR*XNB*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) - XCR*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*(1 - XNB) + XNB*(1 - XCR)*(-0.00078144028852173851*XCR*(-XCR - XNB + 1) - 9.1962770419395764e-16*XCR - 3.3292577900637949e-17));
	return D_NbCr_result;

}

double D_NbNb(double XCR, double XNB, double phi_del, double phi_lav)
{

	double D_NbNb_result;
	D_NbNb_result = 1.0000000000000001e-5*(pow(XNB, 2)*(0.00078144028852173851*XCR*(-XCR - XNB + 1) + 9.1962770419395764e-16*XCR + 3.3292577900637949e-17) + pow(XNB, 2)*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) + (1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*pow(1 - XNB, 2))*(-98097264727.200256*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) - 98097264727.200256*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 124863466558.63902) + 1.0000000000000001e-5*(3268536058.0499763*pow(phi_del, 3)*(6.0*pow(phi_del, 2) - 15.0*phi_del + 10.0) + 3268536058.0499763*pow(phi_lav, 3)*(6.0*pow(phi_lav, 2) - 15.0*phi_lav + 10.0) + 14703563128.545319)*(XCR*XNB*(0.00019901718691452399*XCR*(-XCR - XNB + 1) + 9.3553083090251001e-16*XCR - 9.6947358113172418e-18*XNB + 1.7389451192085454e-17) - XCR*(1.6381840639562911e-16 - 1.3858793730332273e-16*XNB)*(1 - XNB) + XNB*(1 - XCR)*(-0.00078144028852173851*XCR*(-XCR - XNB + 1) - 9.1962770419395764e-16*XCR - 3.3292577900637949e-17));
	return D_NbNb_result;

}
