#include "window.h"

#include <algorithm>
#include <iostream>

#include "fft.h"
#include "Signal.h"
#include "special.h"
#include "utilities.h"

namespace dsp::window
{
	template<class T>
	std::vector<T> boxcar(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		return std::vector<T>(N, static_cast<T>(1));
	}

	template<class T>
	std::vector<T> triang(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);
		auto n = dsp::arange<T>(1.0, static_cast<T>((M + 1) / 2 + 1));

		std::vector<T> w;
		w.resize(n.size(), 0.0);
		if (M % 2 == 0)
		{
			std::transform(n.begin(), n.end(),
				w.begin(),
				[M](auto n) {return static_cast<T>((2 * n - 1.0) / M); });
			w.insert(w.end(), w.rbegin(), w.rend());
		}
		else
		{
			std::transform(n.begin(), n.end(),
				w.begin(),
				[M](auto n) {return static_cast<T>(2 * n / (M + 1.0)); });
			w.insert(w.end(), w.rbegin() + 1, w.rend());
		}

		return utilities::truncate(w, needs_trunc);
	}

	template<class T>
	std::vector<T> general_cosine(unsigned N, const std::vector<T>& a, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto fac = dsp::linspace<T>(static_cast<T>(-pi), static_cast<T>(pi), M);
		std::vector<T> w(M, 0);
		for (size_t k = 0; k < a.size(); ++k)
		{
			auto ak = a[k];
			std::transform(w.begin(), w.end(),
				fac.begin(),
				w.begin(),
				[k, ak](auto w, auto fac) {return w + ak * std::cos(k * fac); });
		}
		return utilities::truncate(w, needs_trunc);
	}

	template <class T>
	std::vector<T> general_hamming(unsigned N, double alpha, bool sym)
	{
		return general_cosine<T>(N, { static_cast<T>(alpha), static_cast<T>(1.0 - alpha) }, sym);
	}

	template<class T>
	std::vector<T> blackman(unsigned N, bool sym)
	{
		return general_cosine<T>(N, { static_cast<T>(0.42), static_cast<T>(0.50), static_cast<T>(0.08) }, sym);
	}

	template <class T>
	std::vector<T> hamming(unsigned N, bool sym)
	{
		return general_hamming<T>(N, 0.54, sym);
	}

	template <class T>
	std::vector<T> hann(unsigned N, bool sym)
	{
		return general_hamming<T>(N, 0.5, sym);
	}

	template <class T>
	std::vector<T> bartlett(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto n = arange<T>(0.0, static_cast<T>(M));
		auto w(n);

		for (auto& x : w)
		{
			if (x <= (M - 1) / 2.0)
			{
				x = static_cast<T>(2.0 * x / (M - 1));
			}
			else
			{
				x = static_cast<T>(2.0 - 2.0 * x / (M - 1));
			}
		}

		return utilities::truncate(w, needs_trunc);
	}

	template<class T>
	std::vector<T> flattop(unsigned N, bool sym)
	{
		std::vector<T> a{
			static_cast<T>(0.21557895),
			static_cast<T>(0.41663158),
			static_cast<T>(0.277263158),
			static_cast<T>(0.083578947),
			static_cast<T>(0.006947368) };
		return general_cosine<T>(N, a, sym);
	}

	template <class T>
	std::vector<T> parzen(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);
		auto n = arange<T>(-(static_cast<T>(M - 1)) / static_cast<T>(2.0),
			static_cast<T>((M - 1) / 2.0 + 0.5), 1.0);

		auto na = extract<T>([M](auto n) { return n < -static_cast<T>(M - 1) / 4.0; }, n);
		auto nb = extract<T>([M](auto n) {return std::abs(n) <= (M - 1) / 4.0; }, n);
		std::vector<T> wa;
		std::transform(na.begin(), na.end(), std::back_inserter(wa),
			[M](auto n) {return static_cast<T>(2 * std::pow((1 - std::abs(n) / static_cast<T>(M / 2.0)), 3)); });
		std::vector<T> wb;
		std::transform(nb.begin(), nb.end(), std::back_inserter(wb),
			[M](auto n)
			{
				return static_cast<T>(1 - 6 * std::pow(std::abs(n) / (M / 2.0), 2.0) + 6 * std::pow(std::abs(n) / (M / 2.0), 3.0));
			});

		auto w(wa);
		w.insert(w.end(), wb.begin(), wb.end());
		w.insert(w.end(), wa.rbegin(), wa.rend());

		return utilities::truncate(w, needs_trunc);

	}

	template <class T>
	std::vector<T> bohman(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto fac = dsp::abs<T>(dsp::linspace<T>(-1, 1, M));
		fac = { fac.begin() + 1, fac.end() - 1 };
		std::vector<T> w;
		w.resize(fac.size());
		std::transform(fac.begin(), fac.end(), w.begin(),
			[](auto fac)
			{
				return (1 - fac) * static_cast<T>(std::cos(dsp::pi * fac)) + static_cast<T>(1.0 / dsp::pi * std::sin(dsp::pi * fac));
			});
		w.push_back(0.0);
		w.insert(w.begin(), 0.0);

		return utilities::truncate(w, needs_trunc);
	}

	template<class T>
	std::vector<T> blackmanharris(unsigned N, bool sym)
	{
		std::vector<T> a
		{
			static_cast<T>(0.35875),
			static_cast<T>(0.48829),
			static_cast<T>(0.14128),
			static_cast<T>(0.01168)
		};

		return general_cosine<T>(N, a, sym);
	}

	template <class T>
	std::vector<T> nuttall(unsigned N, bool sym)
	{
		std::vector<T> a
		{
			static_cast<T>(0.3635819),
			static_cast<T>(0.4891775),
			static_cast<T>(0.1365995),
			static_cast<T>(0.0106411)
		};

		return general_cosine<T>(N, a, sym);
	}

	template <class T>
	std::vector<T> barthann(unsigned N, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto n = dsp::arange<T>(0, static_cast<T>(M));
		std::vector<T> fac;
		fac.resize(n.size());
		std::transform(n.begin(), n.end(), fac.begin(),
			[M](auto n)
			{
				return static_cast<T>(std::abs(n / (M - 1.0) - 0.5));
			});
		std::vector<T> w;
		w.resize(fac.size());
		std::transform(fac.begin(), fac.end(), w.begin(),
			[](auto fac)
			{
				return static_cast<T>(0.62 - 0.48 * fac + 0.38 * std::cos(2 * dsp::pi * fac));
			});
		return utilities::truncate(w, needs_trunc);
	}

	template <class T>
	std::vector<T> kaiser(unsigned N, double beta, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto n = dsp::arange<T>(0, static_cast<T>(M));
		auto alpha = (M - 1) / 2.0;

		std::vector<T> w;
		w.resize(n.size());
		std::transform(n.begin(), n.end(), w.begin(),
			[alpha, beta](auto n)
			{
				return special::i0(static_cast<T>(beta * std::sqrt(1 - std::pow((n - alpha) / alpha, 2)))) / special::i0(static_cast<T>(beta));
			});

		return utilities::truncate(w, needs_trunc);
	}

	template <class T>
	std::vector<T> gaussian(unsigned N, double std, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto n = arange<T>(0, static_cast<T>(M));
		std::transform(n.begin(), n.end(),
			n.begin(),
			[M](auto n) {return static_cast<T>(n - (static_cast<T>(M) - 1.0) / 2.0); });
		auto sig2 = 2 * std * std;
		std::vector<T> w;
		w.resize(n.size());
		std::transform(n.begin(), n.end(),
			w.begin(),
			[sig2](auto n) {return static_cast<T>(std::exp(-1.0 * n * n / sig2)); });

		return utilities::truncate(w, needs_trunc);
	}

	template <class T>
	std::vector<T> general_gaussian(unsigned N, double p, double sig, bool sym)
	{
		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto n = arange<T>(0, static_cast<T>(M));
		std::transform(n.begin(), n.end(),
			n.begin(),
			[M](auto n) {return static_cast<T>(n - (static_cast<T>(M) - 1.0) / 2.0); });

		std::vector<T> w;
		w.resize(n.size());
		std::transform(n.begin(), n.end(),
			w.begin(),
			[sig, p](auto n) {return static_cast<T>(std::exp(-0.5 * std::pow(std::abs(n / sig), 2 * p))); });

		return utilities::truncate(w, needs_trunc);
	}

	template <class T>
	std::vector<T> chebwin(unsigned N, double at, bool sym)
	{
		if (std::abs(at) < 45)
		{
			std::cout << "This window is not suitable for spectral analysis for attenuation values lower than about 45dB because the equivalent noise bandwidth of a Chebyshev window does not grow monotonically with increasing sidelobe attenuation when the attenuation is smaller than about 45 dB." << std::endl;
		}

		if (N <= 0) return std::vector<T>();

		auto [M, needs_trunc] = utilities::extend(N, sym);

		auto order = M - 1;
		auto beta = std::cosh(1.0 / order * std::acosh(std::pow(10, std::abs(at) / 20)));
		std::vector<T> x;
		x.resize(M);
		for (unsigned k = 0; k < M; ++k)
		{
			x[k] = static_cast<T>(beta * std::cos(pi * k / M));
		}
		// Find the window's DFT coefficients
		// Use analytic definition of Chebyshev polynomial
		std::vector<T> p;
		p.resize(x.size());
		std::transform(x.begin(), x.end(), p.begin(),
			[order, M](double x)
			{
				if (x > 1)
				{
					return std::cosh(order * std::acosh(x));
				}
				if (x < -1)
				{
					return (2 * (M % 2) - 1) * std::cosh(order * std::acosh(-x));
				}
				if (std::abs(x <= 1))
				{
					return std::cos(order * std::acos(x));
				}
				else
				{
					return 0.0;
				}
			}
		);
		// Appropriate IDFT and filling up
		// depending on even / odd M
		std::vector<T> w;
		unsigned n;
		if (M % 2 != 0)
		{
			//TODO: The following line does not seem to work!
			auto W = fft::rfft(p);			
			std::transform(W.begin(), W.end(), std::back_inserter(w), [](auto W) {return W.real(); });
			n = (M + 1) / 2;
			w = { w.begin(), w.begin() + n };
			w.insert(w.begin(), w.rbegin(), w.rend() - 1);
		}
		else
		{
			std::vector<std::complex<T>> q;
			
			for(auto& c : p)
			{
				q.emplace_back(c, 0.0);
			}
			
			for (unsigned k = 0; k < M; ++k)
			{
				q[k] *= std::exp(std::complex<T>(0.0, 1.0 * pi / M * k));
				auto Q = fft::fft(q);
				std::transform(Q.begin(), Q.end(), w.begin(), [](auto Q) {return Q.real(); });
				n = M / 2 + 1;
				w.insert(w.begin(), w.rbegin(), w.rend() -1);
			}
		}
		auto max_w = *std::max_element(w.begin(), w.end());
		std::transform(w.begin(), w.end(), w.begin(), [max_w](auto w) {return w / max_w; });

		return utilities::truncate(w, needs_trunc);
	}


	// Explicit template instantiations
	template std::vector<float> boxcar(unsigned N, bool sym);
	template std::vector<double> boxcar(unsigned N, bool sym);
	template std::vector<long double> boxcar(unsigned N, bool sym);
	template std::vector<float> triang(unsigned N, bool sym);
	template std::vector<double> triang(unsigned N, bool sym);
	template std::vector<long double> triang(unsigned N, bool sym);
	template std::vector<float> general_cosine(unsigned N, const std::vector<float>& a, bool sym);
	template std::vector<double> general_cosine(unsigned N, const std::vector<double>& a, bool sym);
	template std::vector<long double> general_cosine(unsigned N, const std::vector<long double>& a, bool sym);
	template std::vector<float> general_hamming(unsigned N, double alpha, bool sym);
	template std::vector<double> general_hamming(unsigned N, double alpha, bool sym);
	template std::vector<long double> general_hamming(unsigned N, double alpha, bool sym);
	template std::vector<float> blackman(unsigned N, bool sym);
	template std::vector<double> blackman(unsigned N, bool sym);
	template std::vector<long double> blackman(unsigned N, bool sym);
	template std::vector<float> hamming(unsigned N, bool sym);
	template std::vector<double> hamming(unsigned N, bool sym);
	template std::vector<long double> hamming(unsigned N, bool sym);
	template std::vector<float> hann(unsigned N, bool sym);
	template std::vector<double> hann(unsigned N, bool sym);
	template std::vector<long double> hann(unsigned N, bool sym);
	template std::vector<float> bartlett(unsigned N, bool sym);
	template std::vector<double> bartlett(unsigned N, bool sym);
	template std::vector<long double> bartlett(unsigned N, bool sym);
	template std::vector<float> flattop(unsigned N, bool sym);
	template std::vector<double> flattop(unsigned N, bool sym);
	template std::vector<long double> flattop(unsigned N, bool sym);
	template std::vector<float> parzen(unsigned N, bool sym);
	template std::vector<double> parzen(unsigned N, bool sym);
	template std::vector<long double> parzen(unsigned N, bool sym);
	template std::vector<float> bohman(unsigned N, bool sym);
	template std::vector<double> bohman(unsigned N, bool sym);
	template std::vector<long double> bohman(unsigned N, bool sym);
	template std::vector<float> blackmanharris(unsigned N, bool sym);
	template std::vector<double> blackmanharris(unsigned N, bool sym);
	template std::vector<long double> blackmanharris(unsigned N, bool sym);
	template std::vector<float> nuttall(unsigned N, bool sym);
	template std::vector<double> nuttall(unsigned N, bool sym);
	template std::vector<long double> nuttall(unsigned N, bool sym);
	template std::vector<float> barthann(unsigned N, bool sym);
	template std::vector<double> barthann(unsigned N, bool sym);
	template std::vector<long double> barthann(unsigned N, bool sym);
	template std::vector<float> kaiser(unsigned N, double beta, bool sym);
	template std::vector<double> kaiser(unsigned N, double beta, bool sym);
	template std::vector<long double> kaiser(unsigned N, double beta, bool sym);
	template std::vector<float> gaussian(unsigned N, double std, bool sym);
	template std::vector<double> gaussian(unsigned N, double std, bool sym);
	template std::vector<long double> gaussian(unsigned N, double std, bool sym);
	template std::vector<float> general_gaussian(unsigned N, double p, double std, bool sym);
	template std::vector<double> general_gaussian(unsigned N, double p, double std, bool sym);
	template std::vector<long double> general_gaussian(unsigned N, double p, double std, bool sym);
	//template std::vector<float> chebwin(unsigned N, double at, bool sym);
	template std::vector<double> chebwin(unsigned N, double at, bool sym);
	//template std::vector<long double> chebwin(unsigned N, double at, bool sym);
}

