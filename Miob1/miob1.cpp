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
			const int size = static_cast<int>(current_permutation.size());
			std::vector<int> as(size), bs(size);

			for (int i = 0; i < size; ++i)
			{
				as[i] = i;
				bs[i] = i;
			}

			std::sort(as.begin(), as.end(), [&](const auto& left, const auto& right) { return a.data[left] > a.data[right]; });
			std::sort(bs.begin(), bs.end(), [&](const auto& left, const auto& right) { return b.data[left] < b.data[right]; });

			for (int i = 0; i < size; ++i)
				current_permutation[as[i]] = bs[i];
		}

		std::vector<int> gen_2opt(std::vector<int> perm, int i, int j)
		{
			std::vector<int> ret(perm);
			std::swap(ret[i], ret[j]);
			return ret;
		}

		float obj_func(std::vector<int> permutation)
		{
			const int size = static_cast<int>(current_permutation.size());
			float sum = 0;
			for (int i = 0; i < size; ++i) {
				for (int j = 0; j < size; ++j)
				{
					sum += a.data[i * a.w + j] * b.data[permutation[i] * b.w + permutation[j]];
				}
			}
			return sum;
		}

		float obj_func_neighbour()
		{

		}

		std::vector<int> greedy(std::vector<int> perm)
		{
			const int size = static_cast<int>(current_permutation.size());
			std::vector<int> best(perm);
			std::vector<int> neighbour;
			float current_obj_func = obj_func(perm);
			float best_obj_func = current_obj_func;
			bool found;

			do {
				found = false;

				for (int i = 0; (i < size - 1) && !found; ++i) {
					for (int j = i + 1; (j < size) && !found; ++j)
					{
						neighbour = gen_2opt(best, i, j);
						current_obj_func = obj_func(neighbour);
						if (current_obj_func < best_obj_func) {
							best_obj_func = current_obj_func;
							best = neighbour;
							found = true;
						}
					}
				}
			} while (found == true);

			return best;
		}

		std::vector<int> steepest(std::vector<int> perm)
		{
			std::vector<int> best(perm);
			std::vector<int> neighbour;
			float current_obj_func = obj_func(perm);
			float best_obj_func = current_obj_func;
			bool found;
			const int size = static_cast<int>(current_permutation.size());

			do {
				found = 0;
				for (int i = 0; i < size - 1; ++i) {
					for (int j = i + 1; j < size; ++j)
					{
						neighbour = gen_2opt(best, i, j);
						current_obj_func = obj_func(neighbour);
						if (current_obj_func < best_obj_func) {
							best_obj_func = current_obj_func;
							best = neighbour;
							found = 1;
						}
					}
				}
			} while (found == 1);

			return best;
		}

	private:
		Mat a, b;
		PermGen rng;
		
		std::vector<int> current_permutation;
		std::vector<float> partial_costs;
};

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


