// based on C++17 Complete by N. Jossutis

#include <vector>
#include <iostream>
#include <numeric>     // for transform_reduce()
#include <execution>   // for the execution policy
#include <filesystem>  // filesystem library
#include <chrono>
#include <fstream>

void PrintTiming(const char* title, const std::chrono::time_point<std::chrono::steady_clock>& start)
{
	const auto end = std::chrono::steady_clock::now();
	std::cout << title << ": " << std::chrono::duration <double, std::milli>(end - start).count() << " ms\n";
}

template<typename Policy>
std::vector<std::filesystem::path> GatherAllTextFiles(const std::filesystem::path& root, Policy pol)
{
	std::vector<std::filesystem::path> paths;
	std::vector<std::filesystem::path> output;
	try {
		auto start = std::chrono::steady_clock::now();
		std::filesystem::recursive_directory_iterator dirpos{ root };
		std::copy(begin(dirpos), end(dirpos),
			std::back_inserter(paths));
		PrintTiming("gathering all the paths", start);
		std::cout << "number of all files: " << std::size(paths) << "\n";

		std::mutex mut;

		start = std::chrono::steady_clock::now();
		// we have all files now... so filter them out (possibly in a parallel way, as std::copy_if is not there yet
		std::for_each(pol, std::begin(paths), std::end(paths), [&output, &mut](const std::filesystem::path& p) {
			if (std::filesystem::is_regular_file(p) && p.has_extension())
			{
				auto ext = p.extension();
				if (ext == std::string(".txt"))
				{
					std::unique_lock<std::mutex> lock(mut);
					output.push_back(p);
				}
			}
		});
		PrintTiming("filtering only TXT files", start);
	}
	catch (const std::exception& e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		return {};
	}

	return output;
}

template<>
std::vector<std::filesystem::path> GatherAllTextFiles<std::execution::sequenced_policy>(const std::filesystem::path& root, std::execution::sequenced_policy)
{
	std::vector<std::filesystem::path> paths;
	try {
		auto start = std::chrono::steady_clock::now();
		std::filesystem::recursive_directory_iterator dirpos{ root };
		std::copy_if(begin(dirpos), end(dirpos),
			std::back_inserter(paths), [](const std::filesystem::path& p) {
			if (std::filesystem::is_regular_file(p) && p.has_extension())
			{
				auto ext = p.extension();
				return ext == std::string(".txt");
			}

			return false;
		});
		PrintTiming("filtering only TXT files sequential", start);
	}
	catch (const std::exception& e) {
		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		return {};
	}

	return paths;
}

struct FileAndWordCount
{
	std::filesystem::path path;
	uint32_t wordCount;
};

int countWords(std::istream& in) {
	int count = 0;
	for (std::string word; in >> word; ++count) {}
	return count;
}

int main(int argc, char* argv[])
{
	// root directory is passed as command line argument:
	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <path> <parallel:1|0>\n";
		return EXIT_FAILURE;
	}
	std::filesystem::path root{ argv[1] };

	int executionPolicyMode = atoi(argv[2]);
	std::cout << "Using " << (executionPolicyMode ? "PAR" : "SEQ") << " Policy\n";

	std::vector<std::filesystem::path> paths;
	if (executionPolicyMode)
		paths = GatherAllTextFiles(root, std::execution::par);
	else
		paths = GatherAllTextFiles(root, std::execution::seq);

	std::cout << "number of files: " << std::size(paths) << "\n";

	//for (const auto& p : paths)
	//	std::cout << p.filename() << "\n";

	// transform into pairs:
	std::vector<FileAndWordCount> filesWithWordCount;
	std::transform(std::begin(paths), std::end(paths), std::back_inserter(filesWithWordCount), [](const std::filesystem::path& p) {
		return FileAndWordCount{ p, 0 };
	});

	// accumulate size of all regular files:
	auto start = std::chrono::steady_clock::now();
	uintmax_t FinalSize = 0;
	if (executionPolicyMode)
	{
		std::for_each(std::execution::par,                 // parallel execution
			filesWithWordCount.begin(), filesWithWordCount.end(), [](FileAndWordCount& p) { //  file size if regular file
			std::ifstream file(p.path);
			p.wordCount = countWords(file);
		});

		/*FinalSize = std::transform_reduce(
			std::execution::par,                 // parallel execution
			filesWithWordCount.begin(), filesWithWordCount.end(),        // range
			std::uintmax_t{ 0 },                   // initial value
			std::plus<>(),                       // accumulate ...
			[](FileAndWordCount& p) { //  file size if regular file
			std::ifstream file(p.path);
			p.wordCount = countWords(file);
			return p.wordCount;
		});*/
	}
	else
	{
		std::for_each(std::execution::seq,                 // parallel execution
			filesWithWordCount.begin(), filesWithWordCount.end(), [](FileAndWordCount& p) { //  file size if regular file
			std::ifstream file(p.path);
			p.wordCount = countWords(file);
		});
		
		/*FinalSize = std::transform_reduce(
			std::execution::seq,                 // parallel execution
			filesWithWordCount.begin(), filesWithWordCount.end(),        // range
			std::uintmax_t{ 0 },                   // initial value
			std::plus<>(),                       // accumulate ...
			[](FileAndWordCount& p) { //  file size if regular file
			std::ifstream file(p.path);
			p.wordCount = countWords(file);
			return p.wordCount;
		});*/

	}
	PrintTiming("computing the sizes", start);
	std::cout << "word count " << paths.size()
		<< " regular files: " << FinalSize << "\n";

	if (argc > 3)
	{
		for (const auto& p : filesWithWordCount)
			std::cout << p.path << ", words: " << p.wordCount << "\n";
	}
}

