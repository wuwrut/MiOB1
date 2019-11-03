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
			partial_costs = std::vector<float>(n*n, 0);
		}

		void init_random()
		{
			const int size = static_cast<int>(current_permutation.size());

			for (int i = 0; i < size; ++i)
				current_permutation[i] = i;

			for (int i = size - 1; i >= 0; --i)
				std::swap(current_permutation[i], current_permutation[rng(0, i)]);
		}

		float init_heuristic()
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

			return init_obj_func(current_permutation);
		}

		void gen_2opt(const std::vector<int>& perm, std::vector<int>& out, int i, int j)
		{
			std::memcpy(out.data(), perm.data(), sizeof(int) * perm.size());
			std::swap(out[i], out[j]);
		}

		float obj_func(const std::vector<float>& costs)
		{
			float sum = 0;

			for (const auto part : costs)
				sum += part;

			return sum;
		}

		void recalculate_obj(const std::vector<int>& perm, std::vector<float>& costs, int i, int j)
		{
			const int size = perm.size();
			
			//recalculate i, j rows
			for (int k = 0; k < size; ++k)
				costs[i * size + k] = a.data[i * a.w + k] * b.data[perm[i] * b.w + perm[k]];

			for (int k = 0; k < size; ++k)
				costs[j * size + k] = a.data[j * a.w + k] * b.data[perm[j] * b.w + perm[k]];

			//recalculate i, j columns
			for (int k = 0; k < size; ++k)
				costs[k * size + i] = a.data[k * a.w + i] * b.data[perm[k] * b.w + perm[i]];

			for (int k = 0; k < size; ++k)
				costs[k * size + j] = a.data[k * a.w + j] * b.data[perm[k] * b.w + perm[j]];
		}

		float init_obj_func(const std::vector<int>& perm)
		{
			const int size = static_cast<int>(current_permutation.size());

			for (int i = 0; i < size; ++i) {
				for (int j = 0; j < size; ++j) {
					const float part = a.data[i * a.w + j] * b.data[perm[i] * b.w + perm[j]];

					/*if (part != partial_costs[i * size + j])
						std::cout << "i: " << i << " j: " << j << " partial: " << partial_costs[i * size + j] << " real: " << part << "\n";*/

					partial_costs[i * size + j] = part;
				}
			}

			return obj_func(partial_costs);
		}

		float greedy(const std::vector<int>& perm)
		{
			std::vector<int> best(perm);
			std::vector<int> neighbour(perm);
			std::vector<float> current_partials(partial_costs);
			
			const int size = static_cast<int>(current_permutation.size());
			float best_obj_func = init_obj_func(perm);
			bool found = false;

			do {
				found = false;

				for (int i = 0; (i < size - 1) && !found; ++i) {
					for (int j = i + 1; (j < size) && !found; ++j)
					{
						gen_2opt(best, neighbour, i, j);

						std::memcpy(current_partials.data(), partial_costs.data(), sizeof(float) * partial_costs.size());
						recalculate_obj(neighbour, current_partials, i, j);

						const float current_obj_func = obj_func(current_partials);

						if (current_obj_func < best_obj_func) {
							best_obj_func = current_obj_func;
							std::memcpy(best.data(), neighbour.data(), sizeof(int) * neighbour.size());
							std::memcpy(partial_costs.data(), current_partials.data(), sizeof(float) * partial_costs.size());
							found = true;
						}
					}
				}
			} while (found);

			std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
			return best_obj_func;
		}

		float steepest(const std::vector<int>& perm)
		{
			std::vector<int> best(perm);
			std::vector<int> neighbour(perm);
			std::vector<float> current_partials(partial_costs);

			const int size = static_cast<int>(current_permutation.size());
			float best_obj_func = init_obj_func(perm);
			bool found;

			do {
				found = false;
				for (int i = 0; i < size - 1; ++i) {
					for (int j = i + 1; j < size; ++j)
					{
						gen_2opt(best, neighbour, i, j);

						std::memcpy(current_partials.data(), partial_costs.data(), sizeof(float) * partial_costs.size());
						recalculate_obj(neighbour, current_partials, i, j);

						const float current_obj_func = obj_func(current_partials);

						if (current_obj_func < best_obj_func) {
							best_obj_func = current_obj_func;
							std::memcpy(best.data(), neighbour.data(), sizeof(int) * neighbour.size());
							std::memcpy(partial_costs.data(), current_partials.data(), sizeof(float) * partial_costs.size());
							found = true;
						}
					}
				}
			} while (found);

			std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
			return best_obj_func;
		}

		float random(int time) {
			std::vector<int> best;
			auto time0 = high_resolution_clock::now();
			init_random();
			std::memcpy(best.data(), current_permutation.data(), sizeof(int) * current_permutation.size());
			float best_obj_func = init_obj_func(current_permutation);

			while ((high_resolution_clock::now() - time0).count() < time){
				init_random();
				float current_obj_func = init_obj_func(current_permutation);
				if (current_obj_func < best_obj_func)
				{
					best_obj_func = current_obj_func;
					std::memcpy(best.data(), current_permutation.data(), sizeof(int) * current_permutation.size());
				}
			}

			std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
			return best_obj_func;
		}

		std::vector<int> getCurrentPerm(){
			return current_permutation;
		}

	private:
		Mat a, b;
		PermGen rng;
		
		std::vector<int> current_permutation;
		std::vector<float> partial_costs;
};

std::pair<std::vector<int>, std::vector<float>> time_count(QAP& instance, int algorithm, int time_random)
{
	int licznik = 0; 
	auto time0 = high_resolution_clock::now();
    high_resolution_clock::time_point start; 
    std::vector<int> time_counts;
	std::vector<float> scores;

	do{
		instance.init_random();
		start = high_resolution_clock::now();

		switch(algorithm){
			case 1:
				scores.push_back(instance.random(time_random));
				break;

			case 2:
				scores.push_back(instance.init_heuristic());
				break;

			case 3:
				scores.push_back(instance.greedy(instance.getCurrentPerm()));
				break;

			case 4:
				scores.push_back(instance.steepest(instance.getCurrentPerm()));
				break;
		}		

		licznik++;
        time_counts.push_back((high_resolution_clock::now() - start).count());

	} while ( licznik < 10 || (high_resolution_clock::now() - time0).count() < 100 );

	std::pair<std::vector<int>, std::vector<float>> ret = std::make_pair(time_counts, scores);

	return ret;
}

std::vector<float> stats(std::vector<int> v){
	float mean = std::accumulate( v.begin(), v.end(), 0.0)/v.size();
	int min = *std::min_element(v.begin(), v.end());
	int max = *std::max_element(v.begin(), v.end());
	std::nth_element(v.begin(), v.begin() + static_cast<int>(v.size()/2) + 1, v.end());
	float median = static_cast<float>(v[static_cast<int>(v.size()/2)]);
	if (v.size() % 2 == 1){
		median += v[static_cast<int>(v.size()/2)+1];
		median /= 2;
	}

	std::vector<float> stats = { mean, median, static_cast<float>(min), static_cast<float>(max)};

	return stats;
}


int main(int argc, char** argv)
{
	std::string path = argv[1];
	int algorithm = std::stoi(argv[2]);
	int time_random = std::stoi(argv[3]);

	QAP instance(path);
	std::pair<std::vector<int>, std::vector<float>> result;
	result = time_count(instance, algorithm, time_random);

	std::cout << "time\tscore\n";

	for (int i = 0; i < result.first.size(); i++){
		std::cout << result.first[i] << "\t" << result.second[i] << "\n";
	}

	auto perm = instance.getCurrentPerm();

	std::cout << "permutation:\n";

	for (auto i : perm)
		std::cout << i << " ";

	std::cout << "\n";

	auto stats_time = stats(result.first);

	for (auto& x : stats_time)
		std::cout << x << "\n";

	return 0;
}


