// based on C++17 Complete by Nicolai Josuttis, https://leanpub.com/cpp17

#include <vector>
#include <iostream>
#include <numeric>     // for transform_reduce()
#include <execution>   // for the execution policy
#include <chrono>
#include <cctype>
#include <string_view>

bool is_word_beginning(char left, char right) 
{ 
	return std::isspace(left) && !std::isspace(right); 
}

std::size_t word_count(std::string_view s)
{
	if (s.empty())
		return 0;

	std::size_t wc = (!std::isspace(s.front()) ? 1 : 0);
	wc += std::transform_reduce(std::execution::par_unseq,
		s.begin(),
		s.end() - 1,
		s.begin() + 1,
		std::size_t(0),
		std::plus<std::size_t>(),
		is_word_beginning);

	return wc;
}

void PrintTiming(const char* title, const std::chrono::time_point<std::chrono::steady_clock>& start)
{
	const auto end = std::chrono::steady_clock::now();
	std::cout << title << ": " << std::chrono::duration <double, std::milli>(end - start).count() << " ms\n";
}

int main(int argc, char* argv[])
{
	int executionPolicyMode = atoi(argv[2]);
	std::cout << "Using " << (executionPolicyMode ? "PAR" : "SEQ") << " Policy\n";

	// init list of all file paths in passed file tree:
	
}

