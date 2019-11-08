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
		tmp_vec.reserve(n * n);

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
		partial_costs = std::vector<float>(n * n, 0);
	}

	void init_random()
	{
		const int size = static_cast<int>(current_permutation.size());

		for (int i = 0; i < size; ++i)
			current_permutation[i] = i;

		for (int i = size - 1; i >= 0; --i)
			std::swap(current_permutation[i], current_permutation[rng(0, i)]);
	}

	std::vector<float> init_heuristic()
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

		return { init_obj_func(current_permutation) };
	}

	void gen_swap(const std::vector<int>& perm, std::vector<int>& out, int i, int j)
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
				partial_costs[i * size + j] = a.data[i * a.w + j] * b.data[perm[i] * b.w + perm[j]];
			}
		}

		return obj_func(partial_costs);
	}

	std::vector<float> greedy(const std::vector<int>& perm)
	{
		std::vector<int> best(perm);
		std::vector<int> neighbour(perm);
		std::vector<float> current_partials(partial_costs);
		std::vector<float> scores; scores.reserve(100000);

		const int size = static_cast<int>(current_permutation.size());
		float best_obj_func = init_obj_func(perm);
		bool found = false;

		do {
			found = false;

			for (int i = 0; (i < size - 1) && !found; ++i) {
				for (int j = i + 1; (j < size) && !found; ++j)
				{
					gen_swap(best, neighbour, i, j);

					std::memcpy(current_partials.data(), partial_costs.data(), sizeof(float) * partial_costs.size());
					recalculate_obj(neighbour, current_partials, i, j);

					const float current_obj_func = obj_func(current_partials);

					if (current_obj_func < best_obj_func) {
						best_obj_func = current_obj_func;
						std::memcpy(best.data(), neighbour.data(), sizeof(int) * neighbour.size());
						std::memcpy(partial_costs.data(), current_partials.data(), sizeof(float) * partial_costs.size());
						found = true;
						swaps++;
					}

					scores.push_back(best_obj_func);
				}
			}
		} while (found);

		std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
		return scores;
	}

	std::vector<float> steepest(const std::vector<int>& perm)
	{
		std::vector<int> best(perm);
		std::vector<int> neighbour(perm);
		std::vector<float> current_partials(partial_costs);
		std::vector<float> scores; scores.reserve(100000);

		const int size = static_cast<int>(current_permutation.size());
		float best_obj_func = init_obj_func(perm);
		bool found;

		do {
			found = false;
			for (int i = 0; i < size - 1; ++i) {
				for (int j = i + 1; j < size; ++j)
				{
					gen_swap(best, neighbour, i, j);

					std::memcpy(current_partials.data(), partial_costs.data(), sizeof(float) * partial_costs.size());
					recalculate_obj(neighbour, current_partials, i, j);

					const float current_obj_func = obj_func(current_partials);

					if (current_obj_func < best_obj_func) {
						best_obj_func = current_obj_func;
						std::memcpy(best.data(), neighbour.data(), sizeof(int) * neighbour.size());
						std::memcpy(partial_costs.data(), current_partials.data(), sizeof(float) * partial_costs.size());
						found = true;
						swaps++;
					}

					scores.push_back(best_obj_func);
				}
			}
		} while (found);

		std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
		return scores;
	}

	std::vector<float> random(int time) {
		std::vector<int> best(current_permutation);
		std::vector<float> scores; scores.reserve(100000);

		const auto time0 = high_resolution_clock::now();
		init_random();
		std::memcpy(best.data(), current_permutation.data(), sizeof(int) * current_permutation.size());
		float best_obj_func = init_obj_func(current_permutation);

		while (std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - time0).count() < time) {
			init_random();
			float current_obj_func = init_obj_func(current_permutation);

			if (current_obj_func < best_obj_func)
			{
				best_obj_func = current_obj_func;
				std::memcpy(best.data(), current_permutation.data(), sizeof(int) * current_permutation.size());
				swaps++;
			}
			scores.push_back(best_obj_func);
		}

		std::memcpy(current_permutation.data(), best.data(), sizeof(int) * best.size());
		return scores;
	}

	std::vector<int> getCurrentPerm() {
		return current_permutation;
	}

	int GetSwaps()
	{
		const int tmp = swaps;
		swaps = 0;
		return tmp;
	}

private:
	Mat a, b;
	PermGen rng;

	std::vector<int> current_permutation;
	std::vector<float> partial_costs;
	int swaps = 0;
};

std::tuple<std::vector<int>, std::vector<int>, std::vector<std::vector<float>>, std::vector<int>, std::vector<int>> time_count(QAP& instance, int algorithm, int time_random, int how_many_times)
{
	int licznik = 0;
	auto time0 = high_resolution_clock::now();
	high_resolution_clock::time_point start;

	std::vector<int> init_perm;
	std::vector<int> best_perm;
	std::vector<int> swaps;
	std::vector<int> time_counts;
	std::vector<float> scores;
	std::vector<std::vector<float>> runs;

	swaps.reserve(10000);
	time_counts.reserve(10000);
	scores.reserve(10000);
	runs.reserve(how_many_times);

	do {
		instance.init_random();
		init_perm = instance.getCurrentPerm();
		start = high_resolution_clock::now();

		switch (algorithm) {
		case 1:
			scores = instance.random(time_random);
			break;

		case 2:
			scores = instance.init_heuristic();
			break;

		case 3:
			scores = instance.greedy(instance.getCurrentPerm());
			break;

		case 4:
			scores = instance.steepest(instance.getCurrentPerm());
			break;
		}

		runs.push_back(std::move(scores));
		licznik++;
		time_counts.push_back((high_resolution_clock::now() - start).count());
		swaps.push_back(instance.GetSwaps());

		best_perm = instance.getCurrentPerm();

	} while (licznik < how_many_times || std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - time0).count() < 100);

	return std::make_tuple(init_perm, time_counts, runs, swaps, best_perm);
}

std::vector<float> stats(std::vector<int> v) {
	float mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
	int min = *std::min_element(v.begin(), v.end());
	int max = *std::max_element(v.begin(), v.end());
	std::nth_element(v.begin(), v.begin() + static_cast<int>(v.size() / 2) + 1, v.end());
	float median = static_cast<float>(v[static_cast<int>(v.size() / 2)]);
	if (v.size() % 2 == 1) {
		median += v[static_cast<int>(v.size() / 2) + 1];
		median /= 2;
	}

	std::vector<float> stats = { mean, median, static_cast<float>(min), static_cast<float>(max) };

	return stats;
}


int main(int argc, char** argv)
{
	std::string path = argv[1];
	int algorithm = std::stoi(argv[2]);
	int time_random = std::stoi(argv[3]);
	int how_many_times = std::stoi(argv[4]);

	QAP instance(path);
	auto tup = time_count(instance, algorithm, time_random, how_many_times);
	//auto&& [init_perm, times, scores, swaps, best_perm] = time_count(instance, algorithm, time_random);
	std::vector<int> init_perm = std::get<0>(tup);
	std::vector<int> times = std::get<1>(tup);
	std::vector<std::vector<float>> scores = std::get<2>(tup);
	std::vector<int> swaps = std::get<3>(tup);
	std::vector<int> best_perm = std::get<4>(tup);

	//print scores
	std::cout << scores.size() << "\n";

	for (const auto& s : scores) {
		for (const auto& f : s) {
			std::cout << f << " ";
		}
		std::cout << "\n";
	}


	std::cout << "times:\n";

	for (int i = 0; i < times.size(); i++) {
		std::cout << std::fixed << times[i] << "\n";
	}

	std::cout << "initial permutation:\n";
	for (auto i : init_perm)
		std::cout << i << " ";

	std::cout << "\n";
	std::cout << "initial permutation score:\n";
	instance.init_obj_func(init_perm);
	std::cout << std::fixed << instance.init_obj_func(init_perm) << "\n";

	std::cout << "best permutation:\n";

	for (auto i : best_perm)
		std::cout << i << " ";

	std::cout << "\n";

	//auto stats_time = stats(times);

	/*for (auto& x : stats_time)
		std::cout << x << "\n";*/

	return 0;
}


