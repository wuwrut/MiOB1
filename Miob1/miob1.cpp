#include <random>
#include <iostream>
#include <chrono>
#include <numeric>
#include <unordered_set>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

using namespace std::chrono;

class PermGen
{
	public:
		PermGen()
		{
			unsigned char tmp[std::mt19937::state_size];
			std::random_device rand;
			std::generate(std::begin(tmp), std::end(tmp), std::ref(rand));
			std::seed_seq seq(std::begin(tmp), std::end(tmp));
			rng = std::mt19937(seq);
		}

		int operator()(int start, int end)
		{
			std::uniform_int_distribution<> dist(start, end);
			return dist(rng);
		}

		std::mt19937 rng;
};

struct Mat
{
	std::vector<float> data;
	int w = 0;
	int h = 0;
};

class QAP
{
	public:
		QAP(const std::string& file_name)
		{
			std::ifstream file(file_name);

			int n;
			file >> n;

			std::vector<float> tmp_vec;
			tmp_vec.reserve(n*n);

			for (int i = 0; i < n * n; ++i)
			{
				int tmp;
				file >> tmp;
				tmp_vec.push_back(tmp);
			}

			a = { tmp_vec, n, n };

			tmp_vec.clear();

			for (int i = 0; i < n * n; ++i)
			{
				int tmp;
				file >> tmp;
				tmp_vec.push_back(tmp);
			}

			b = { tmp_vec, n, n };

			current_permutation = std::vector<int>(n, 0);
			partial_costs = std::vector<float>(n, 0);
		}

		void init_random()
		{
			const int size = static_cast<int>(current_permutation.size());

			for (int i = 0; i < size; ++i)
				current_permutation[i] = i;

			for (int i = size - 1; i >= 0; --i)
				std::swap(current_permutation[i], current_permutation[rng(0, i)]);
		}

		void init_heuristic()
		{

		}

		void alg1()
		{
			
		}

		std::vector<int> 2opt(std::vector<int> perm, int a, int b)
		{
			
		}

	private:
		Mat a, b;
		PermGen rng;
		
		std::vector<int> current_permutation;
		std::vector<float> partial_costs;
};

std::pair<std::vector<int>, std::vector<int>> minmax_elem_masked(const Mat& m, const std::unordered_set<int>& mask)
{
	std::vector<float> max_v(m.h, std::numeric_limits<float>::min());
	std::vector<float> min_v(m.h, std::numeric_limits<float>::max());
	std::vector<int> max_e(m.h, -1);
	std::vector<int> min_e(m.h, -1);

	for (int i = 0; i < m.data.size(); ++i)
	{
		if (mask.find(i) != mask.end())
			continue;

		const int row = i / m.h;

		if (m.data[i] > max_v[row])
		{
			max_v[row] = m.data[i];
			max_e[row] = i;
		}

		if (m.data[i] < min_v[row])
		{
			min_v[row] = m.data[i];
			min_e[row] = i;
		}
	}

	return { min_e, max_e };
}

//TODO: pass one matrix transposed (a, b.t) or (a.t, b)
std::vector<std::pair<int, int>> qap_start_heuristic(const Mat& a, const Mat& b)
{
	const int size = a.w; //whatever, we assume that they are rectangles
	std::vector<std::pair<int, int>> ret(size);
	std::unordered_set<int> usedA;
	std::unordered_set<int> usedB;

	for (int i = 0; i < a.h; i++) {
		usedA.insert(i + i * a.w);
		usedB.insert(i + i * a.w);
	}

	for (int i = 0; i < size; ++i)
	{
		auto&& [a_min_rows, a_max_rows] = minmax_elem_masked(a, usedA);
		auto&& [b_min_rows, b_max_rows] = minmax_elem_masked(b, usedB);

		std::vector<float> ab, ba;

		for (int j = 0; j < a_max_rows.size(); ++j)
			ab.push_back(a.data[a_max_rows[j]] * b.data[b_min_rows[j]]);

		for (int j = 0; j < b_max_rows.size(); ++j)
			ba.push_back(b.data[b_max_rows[j]] * a.data[a_min_rows[j]]);

		auto min_ab = std::min_element(ab.cbegin(), ab.cend());
		auto min_ba = std::min_element(ba.cbegin(), ba.cend());

		if (*min_ab < *min_ba)
		{
			const auto id = std::distance(ab.cbegin(), min_ab);
			usedA.insert(a_max_rows[id]);
			usedB.insert(b_min_rows[id]);
		}

		else
		{
			const auto id = std::distance(ba.cbegin(), min_ba);
			usedA.insert(a_min_rows[id]);
			usedB.insert(b_max_rows[id]);
		}
	}

	return ret;
}

float time_count()
{
	int licznik = 0; 
	auto time0 = high_resolution_clock::now();

	do{
		// algorytm();
		licznik++;
	} while ( licznik < 10 || (high_resolution_clock::now() - time0).count() < 100 );

	return (float)((high_resolution_clock::now() - time0).count() / licznik);
}


int main()
{
	QAP test("test.txt");

	std::cout << time_count() << "\n";
	return 0;
}


