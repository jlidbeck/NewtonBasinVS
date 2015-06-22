#include "stdafx.h"
#include "Formulas.h"
#include "SketchFormulas.h"
#include "glColor.h"
#include "random.h"
#include "ComplexColorMap.h"
#include "ColorSet.h"

using namespace std;


// Basin Formulas

////////////////////////////////////////////////////////////////

Json::Value BasinFormula::getJSONValue() const 
{
	Json::Value root = ComplexColorMap::getJSONValue();
	root["iterations"] = m_nIterations;
	root["bailout"] = m_fBailout;
	root["colormap"] = m_pColorMap ? m_pColorMap->getJSONValue() : NULL;
	return root;
}

void BasinFormula::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	ComplexColorMap::fromJSONValue(root, factory);
	m_nIterations = root["iterations"].asInt();
	m_fBailout = root["bailout"].asDouble();

	if(root["colormap"].isObject())
	{
		m_pColorMap = factory.createFromJSON(root["colormap"]);
		ASSERT(m_pColorMap->instanceOf("ComplexColorMap"));
	}
	else
	{
		m_pColorMap = NULL;
	}
}

std::string BasinFormula::getRequiredMemberClass(std::string path) const
{
	if(!path.compare("[colormap]")) {
		return "ComplexColorMap";
	}
	if(!path.substr(0, strlen("[colormap]")).compare("[colormap]")) {
		return m_pColorMap->getRequiredMemberClass(path.substr(strlen("[colormap]"), path.length()-strlen("[colormap]")).c_str());
	}
	return "";
}

COLORREF BasinFormula::getIterColor(Complex z, int iter) const
{
	GLcolor c;
	int root = whichRoot(z);
	// good standby: basin rings are hued
	if(!c.SetHSV(arg(z)*360/6.283, 0.8, tanh(0.1*iter)))
		return 0;
	return c;
}

/*void UnitBasinFormula::setDefaults()
{
	// default values
	m_nRoots= 3;
	m_fCutoff = 0.00001;
	m_nIterations = 20;
	m_nRootFlags = -1;

	ColorSet *pTempColorset = new ColorSet();
	pTempColorset->fromHues(BASIC_HUE_SET, 32, 0.9, 1.0);
	pTempColorset->offsetHues(30);	// orange
	pTempColorset->setInstanceName("tempColorSet");
	m_pColorMap = ptrtype(new HarlequinColorMap(5, 7, HarlequinColorMap::SPOTS, ptrtype(pTempColorset)));
}*/

int UnitBasinFormula::whichRoot(Complex z) const
{
	if(!isFinite(z))
		return 0;
	double a = arg(z)/6.28318530718;
	if(a<0)
		++a;		// if a is very close to zero, this might cause a roundoff to 1: watch for that
	ASSERT(a>=0 && a<=1);
	int n = (int)(0.5 + a*m_nRoots);
	if(n==m_nRoots) n=0;
	ASSERT(n>=0 && n<m_nRoots);
	return n;
}

COLORREF TracedUnitBasinFormula::getColor(Complex z)
{
	static CArray<Complex> s_v;
	if(s_v.GetSize() != m_nIterations)
		s_v.SetSize(m_nIterations);
	Complex zn1, zstart(z);
	Complex zm3(z), zm2(z), zm1(z);
	double q = (1-1.0/m_nRoots);
	int iter;
	for(iter=0; iter<m_nIterations; ++iter)
	{
		zm3 = zm2;
		zm2 = zm1;
		zm1 = z;
		zn1 = pow(zm1, m_nRoots-1);
		z = zm1*q + 1/(m_nRoots*zn1);
		double dist=normal(z-zm1);
		if(dist < m_fBailout) break;
	}

	if(m_nRootFlags > 0)
	{
		int n = whichRoot(z);
		if(!(m_nRootFlags & (1<<n)))
			return 0;
	}

	GLcolor c;
	// good standby: basin rings are hued
	if(!c.SetHSV(arg(z*(zm2-zm3))*360/6.283, 0.8, tanh(0.1*iter)))
	// basin rings are busily hued
//	if(!c.SetHSV(arg(conj(zm2-zm3)*(z-zm1))*360/6.283, 0.8, tanh(0.1*iter)))
	// hue=direction of landing. try this with wide cutoff ~0.1
//	if(!c.SetHSV((znext-z).arg()*360/6.283, 0.8, tanh(0.1*iter)))
	// with tight cutoff, bright colors--but try with wide (~0.2) for singed colors
		return 0;
	return c;
}



Json::Value UnitBasinFormula::getJSONValue() const 
{
	Json::Value root = BasinFormula::getJSONValue();
	root["NumRoots"] = m_nRoots;
	root["RootFlags"] = m_nRootFlags;
	return root;
}

void UnitBasinFormula::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	BasinFormula::fromJSONValue(root, factory);
	m_nRoots = root["NumRoots"].asInt();
	m_nRootFlags = root["RootFlags"].asInt();
}

COLORREF UnitBasinFormula::getColor(Complex z)
{
	Complex zn1, znext, zstart(z);
	double q = (1-1.0/m_nRoots);
	int iter;
	for(iter=0; iter<m_nIterations; ++iter)
	{
		// Z <= Z - (Z^n - 1) / (nZ^(n-1))
		Complex zn1 = pow(z, (iter%8+2)-1);
		znext = z*q + 1/((iter%8+2)*zn1);

		// TODO:
		// Z <= Z - (kZ^n - 1) / (knZ^(n-1))
		//    = (knZ^n - kZ^n + 1) / (knZ^(n-1))
		//    = ((n-1)(Z^n) + 1/k) / (nZ^(n-1))

		double dist = normal(z-znext);
		if(dist < m_fBailout) 
			break;
		z = znext;
	}

	GLcolor c;

	int root = whichRoot(z);

	if(m_nRootFlags <= 0)
	{
		return getIterColor(z, iter);

		// classic, color-coded root with dark basins
		if(!c.SetHSV(360/m_nRoots*root, 1, tanh(0.1*iter)))
			return 0;
		return c;
	}

	if(!(m_nRootFlags & (1<<root)))
		return 0;

	// interesting variation of the above. with low iter, iteration limits are rainbow-blurred.
	//if(!c.SetHSV(z.arg()*360/6.283, 0.8, tanh(0.1*iter)))
	//	return 0;
	//return c;

	// busy binary pattern, using hue=direction of landing. 
	// try this with wide cutoff 0.1-0.2 for broader bands and darker colors.
	// this needs to be generalized to an orbit trap.
	//if(!c.SetHSV((znext-z).arg()*360/6.283, 0.8, tanh(0.1*iter)))
	//	return 0;
	//return c;

	// broad bands, interesting texture, monochromatic when zoomed in
	//if(!c.SetHSV((z-zstart).arg()*360/6.283, 0.8, tanh(0.1*iter)))
	//	return 0;
	//return c;

	// z-start: gives nice stable map in basins
	// this works best with a very small cutoff.
	// low iterations are nice
	// high iterations approximate N different offsets of the pattern, each barely distorted
	//return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(z-zstart);

	// if this works at all, it would need a very large cutoff,
	// and a colormap with detail around the origin
	//return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(znext-z);

	// your basic final-iteration colormap. use a map that's interesting near the roots
	return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(z);

}


// defaults
SineBasinFormula::SineBasinFormula() :
	BasinFormula()
{
	m_nIterations = 10;
	m_pColorMap = ptrtype(new BlackHoleColorMap());
}

Json::Value SineBasinFormula::getJSONValue() const 
{
	Json::Value root = BasinFormula::getJSONValue();
	root["bailout"] = m_fBailout;
	return root;
}

void SineBasinFormula::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	BasinFormula::fromJSONValue(root, factory);
	m_fBailout = root["bailout"].asDouble();
}

COLORREF SineBasinFormula::getColor(Complex z)
{
	Complex znext;
	int iter = 0;
	for(; iter<m_nIterations; ++iter)
	{
		znext = z - tan(z);
		double dist = normal(znext-z);
		z = znext;
		if(dist<m_fBailout) break;
	}

	return getIterColor(z, iter);

	return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(sin(z));

//	return RootsColorFn(z, whichRoot(z));
};

int SineBasinFormula::whichRoot(Complex z) const
{
	double f = (z.real()/3.141593);
	if(f<0) return (int)(f-0.5);
	return (int)(f+0.5);
}

COLORREF SineBasinFormula::getIterColor(Complex z, int iter) const
{
	GLcolor c;
	int root = whichRoot(z);
	// good standby: basin rings are hued
	if(!c.SetHSV(root*13.0, 0.8, tanh(0.1*iter)))
		return 0;
	return c;
}



/*void RingSineBasinFormula::setDefaults()
{
	// default settings
	m_nIterations=1;
	m_fRoots = 5;

	ColorSet *pTempColorset = new ColorSet();
	pTempColorset->fromHues(BASIC_HUE_SET, 32, 0.9, 1.0);
	pTempColorset->offsetHues(90);
	pTempColorset->setInstanceName("tempColorSet2");
	m_pColorMap = ptrtype(
		//new HarlequinColorMap(7, 18, HarlequinColorMap::SPOTS, ptrtype(pTempColorset)));
		new BlackHoleColorMap());
		//new LogRingsColorMap(m_fRoots));
}*/

Json::Value RingSineBasinFormula::getJSONValue() const 
{
	Json::Value root = BasinFormula::getJSONValue();
	root["roots"] = m_fRoots;
	return root;
}

void RingSineBasinFormula::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	BasinFormula::fromJSONValue(root, factory);
	m_fRoots = root["roots"].asDouble();
}

COLORREF RingSineBasinFormula::getColor(Complex z)
{
				Complex ni(0, m_fRoots);	// 5i
				Complex ini = inv(ni);		// 1/5i == -i/5
				for(int iter=0; iter<m_nIterations; ++iter)
				{
					// Newton's Method for f(z) == sin(ni*ln(z)):
					// z2 <-- z - f(z)/f'(z)
					// == z - sin(ni*ln(z)) / (ni/z * cos(ni*ln(z)))
					// == z - z/ni * tan(ni*ln(z))
					// == z + iz/n * tan(ni*ln(z))
					// == z + iz/n * i * tanh(n*ln(z))
					// == z - z/n * (z^2n-1) / (z^2n+1)
					
					// this appears to be a beautiful mistake:
					//z = z * (1 - tan(ni*ln(z)) * ini);

					// z <-- z - (z^2N
					// z <-- z - (z/N)(z^2N-1)/(z^2N+1)
					Complex z2n = pow(z, 2*m_fRoots);
					z = z - (z/m_fRoots) * (z2n-1) / (z2n+1);
				}
//				z = sin(ni*ln(z));	// use f() map, and highlight the zeroes

				//return HarlequinColorFn(z, 7, 18);

	return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(z);
}

int RingSineBasinFormula::whichRoot(Complex z) const
{
	int n = (int)(0.5 + arg(z) * m_fRoots / 6.283);
	if(n<0) n+=m_fRoots;
	ASSERT(n>=0 && n<m_fRoots);
	return n;
}





COLORREF GridSineBasinFormula::getColor(Complex z)
{
	double _r2 = _r*_r;
	Complex _ir(0, _r);
	double _inv_r2 = 1.0 / (_r2);

	int iter = 0;
	for(; iter<m_nIterations; ++iter)
	{
		// z2 <-- z - f(z)/f'(z)
		//      = z - sin(z)sinh(z) / (cos(z)sinh(z) + sin(z)cosh(z))
		//      = z -       sinh(z) / (cot(z)sinh(z) +       cosh(z))
		//      = z -             1 / (cot(z)        +       coth(z))
		//z = z - sin(z)*sinh(z) / (cos(z)*sinh(z) + sin(z)*cosh(z));

		// f(z) = sin(sinh(z))
		// z-f(z)/f'(z) = z-tan(sinh(z))/cosh(z)
		//z = z - tan(sinh(z)) / cosh(z);

		// f(z) = sin(r^2/z + ir)
		// z <-- z - sin(r^2/z + ir) / (cos(r^2/z + ir) * (-r^2/z^2))
		// z <-- z - z^2 * tan(r^2/z + ir) / (-r^2)
		Complex znext = z + _inv_r2 * sq(z) * tan(_r2 / z + _ir);

		double dist = normal(znext-z);
		z = znext;
		if(dist<m_fBailout) break;
	}

	//z = sin(sinh(z));	// use f() map, and highlight the zeroes

	//return ((ComplexColorMap*)(m_pColorMap.get()))->getColor(z);
	return getIterColor(z, iter);
}

int GridSineBasinFormula::whichRoot(Complex z) const
{
	// this is a stopgap until we get a nice way to enumerate a grid...
	int x = (z.real() / PI);
	int y = (z.imag() / PI);
	return x+y;
}

COLORREF GridSineBasinFormula::getIterColor(Complex z, int iter) const
{
	GLcolor c;
	int root = whichRoot(z);
	// good standby: basin rings are hued
	if(!c.SetHSV(arg(z-Complex(0,_r/2))*360/6.283, 0.8, tanh(0.1*iter)))
		return 0;
	return c;
}




int LnFormula::whichRoot(Complex z) const { return 0; }

COLORREF LnFormula::getColor(Complex c)
{
	// x - (ln(x) / (1/x)) = x - x ln x
	Complex z(c);
	Complex znext;
	for(int i=0; i<m_nIterations; ++i)
	{
		znext = z * (1 - ln(z));
		if(normal(znext-z) < 0.000001)
			return GLcolor::FromHSV(222, 0.8, tanh(0.1+0.1*i));
		if(abs(znext) > 5000.0)
			return GLcolor::FromHSV(360/6.283*arg(c), 0.3, tanh(0.1+0.1*i));
		z = znext;
	}
	return 0;
}

// sketches

Complex RingPath::get(double th, int j) const
{
			// r = e^x, x = -50/2000 to 50/2000, 100 steps
			double r = ::exp(j*m_b + (m_rows-j)*m_a);
			r += m_waveamp*sin(m_wavefreq*th);

			if(m_blur > 0)
				r += m_blur*::randomGaussian();

//			r = 0.8+0.25*sin(2.1*th);
			return m_ctr + r * Complex(cos(th), sin(th));
}

int SketchFormula::GetGroup(double th, Complex *pts, GLcolor *cols, int n)
{
	// shorthand the path object
	Path *pPath = ((Path*)(m_pPath.get()));

	ASSERT(pPath->getGroupSize() > 0);
//	ASSERT(n >= m_pPath.getGroupSize());
	// n can be less than the number of points offered...
	// only send as many as user requested.
	// Fewer points can be sent.
	if(n > pPath->getGroupSize())
		n = pPath->getGroupSize();

	Complex c, z;
	GLcolor col(1, 1, 1, 0.05f);

	int j;
		for(j=0; j<n; ++j)
		{
			c = pPath->get(th, j);

			pts[j] = c;
//			col.SetHSV(th*180/3.14159265359, 0.5, 1.0);	// anglewise color
			col.SetHSV(j*360/n, 0.5, 1.0);	// pathwise color for buddhabrot
			cols[j] = col;

		}

		for(j=0; j<n; ++j)
			pts[j] = Fold(pts[j]);
		//Fold(pts, pPath->m_rows+1);

		return n;
}

void SketchFormula::Fold(Complex *pz, int n) const
{
	for(int j=0; j<n; ++j) {
		pz[j] = Fold(pz[j]);
	}
}

////////////////////////////////////

Complex NewtonSketch::Fold(Complex z) const
{
	for(int i=0; i<m_nIterations; ++i) {
		// Newton(z^3 - 1) =
		// z <= z - (z^3 - 1) / (3z^2)
		//    = (3z^3 - z^3 + 1) / (3z^2)
		//    = (2z^3 + 1) / (3z^2)

		z = (z+z + inv(sq(z))) / 3;
	}
	return z;
}

////////////////////////////

int BuddhabrotSketch::GetGroupSize(void) const
{
	int n = ((Path*)(m_pPath.get()))->getGroupSize();
	ASSERT(n >= 1);
//	int i = (m_nMaxIter - m_nMinIter + 1);
	int i = m_nMaxIter;
	ASSERT(i>=1);
	return n*i;
}

int BuddhabrotSketch::GetGroup(double th, Complex *pts, GLcolor *cols, int n)
{
	ColorSet colorset;
	colorset.fromHues(BASIC_HUE_SET, 10, 0.9, 1.0);
	colorset.offsetHues(270);	// ?

	// shorthand path
	Path *pPath = ((Path*)(m_pPath.get()));

	ASSERT(n>=GetGroupSize());
	int count = 0;
	Complex c, z;
	for(int j=0; j<pPath->getGroupSize(); ++j)
	{
		c = pPath->get(th, j);
		z = c;

		int i;
		for(i=0; i<m_nMaxIter; ++i)
		{
			z = (z*z + c);
			if(normal(z) > 4.0)
				break;
		}

		// slight alteration here:
		// we're not going to plot short orbits, only
		// interesting ones whose length is at least MinIter.
		if(i < m_nMaxIter && i >= m_nMinIter)
		{
			// plot this point's orbit, until it hits the bailout value

			int nStopIter = i;

			z = c;
			for(i=0; i<=nStopIter; ++i)		// was i<=m_nMaxIter
			{
				z = (z*z) + c;
				pts[count] = z;
				// color according to...
				switch(m_nColorScheme)
				{
				case 0:
					// abs(c)?
					cols[count].SetHSV(360*log(abs(c)), 0.9, 0.9, m_fBrightness);
					break;
				case 1:
					// iter? (busy)
//					cols[count].SetHSV((i-m_nMinIter)*360.0/(m_nMaxIter-m_nMinIter+1), 0.9, 0.9, m_fBrightness);
					cols[count].SetHSV((i*360.0/m_nMaxIter), 0.9, 0.9, m_fBrightness);
					break;
				case 2:
					// c.arg?
					cols[count].SetHSV(arg(c)*360.0/6.283, 0.9, 0.9, m_fBrightness);
					break;
				case 3:
					// ring index? (this makes bright figures with little or no blur)
					cols[count].SetHSV(j*360.0/pPath->getGroupSize(), 0.9, 0.9, m_fBrightness);
					break;
				case 4:
					// stopiter?
					cols[count].SetHSV((nStopIter-m_nMinIter)*360.0/(m_nMaxIter-m_nMinIter+1), 0.9, 0.9, m_fBrightness);
				}

				++count;
			}
		}
	}

	ASSERT(count <= n);
	return count;
}

Complex BuddhabrotSketch::Fold(Complex c) const
{
	Complex z(c);
//	for(int i=0; i<m_nMinIter; ++i)
//		z = (z*z + c);
	return z;
}