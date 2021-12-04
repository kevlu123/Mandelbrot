#pragma once
#include <vector>
#include "Task.h"

struct Complex {

	Complex(float re = 0.0f, float im = 0.0f) :
		re(re),
		im(im) {
	}

	float AbsSquared() const {
		return re * re + im * im;
	}

	Complex Squared() const {
		return Complex(
			re * re - im * im,
			re * im * 2
		);
	}

	Complex operator+(const Complex& rhs) const {
		return Complex(
			re + rhs.re,
			im + rhs.im
		);
	}

	float re;
	float im;
};


struct Mandelbrot {

	static int ComputePoint(float x, float y, int maxIter = 100) {
		Complex z;
		Complex c(x, y);
		for (int i = 0; i < maxIter; i++) {
			z = z.Squared() + c;
			if (z.AbsSquared() > 4.0f)
				return i;
		}
		return maxIter;
	}

	static Mandelbrot ComputeArea(float xMin, float xMax, float yMin, float yMax, int xPx, int yPx) {
		float dx = (xMax - xMin) / xPx;
		float dy = (yMax - yMin) / yPx;
		std::vector<int> v;
		v.reserve((size_t)xPx * yPx);

		float fy = yMin;
		for (int y = 0; y < yPx; y++) {
			float fx = xMin;
			for (int x = 0; x < xPx; x++) {
				v.push_back(ComputePoint(fx, fy));
				fx += dx;
			}
			fy += dy;
		}

		Mandelbrot r;
		r.iterCounts = std::move(v);
		r.width = xPx;
		r.height = yPx;
		return r;
	}

	static Task<Mandelbrot> ParallelComputeAreaAsync(float xMin, float xMax, float yMin, float yMax, int xPx, int yPx, int threads) {
		return Task<Mandelbrot>([=] {
			int rowsPerThread = yPx / threads;
			float heightPerThread = (yMax - yMin) / threads;

			std::vector<Task<Mandelbrot>> tasks;
			for (int i = 0; i < threads; i++) {
				float min = yMin + heightPerThread * i;
				float max = min + heightPerThread;
				tasks.emplace_back(ComputeArea, xMin, xMax, min, max, xPx, rowsPerThread);
			}

			Mandelbrot v;
			v.iterCounts.reserve((size_t)xPx * yPx);
			for (auto& task : tasks) {
				std::vector<int> r = std::move(task.GetResult().iterCounts);
				v.iterCounts.insert(v.iterCounts.end(), r.begin(), r.end());
			}

			v.width = xPx;
			v.height = yPx;
			return v;
			});
	}

	std::vector<int> iterCounts;
	int width = 0;
	int height = 0;
};
