#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <unordered_set>

#include <chrono>

struct parsed_error
{
public:
    parsed_error() = default;

public:
    std::string error_line;
    uint32_t error_count = 0;
    std::string error_category;
    std::string error_verbosity;
};

struct parsed_error_equal
{
public:
    bool operator()(const std::shared_ptr<parsed_error>& error_a, const std::shared_ptr<parsed_error>& error_b) const
    {
        return !(error_a == nullptr || error_b == nullptr) && error_a->error_line == error_b->error_line;
    }
};

struct parsed_error_hash
{
public:
    std::size_t operator()(const std::shared_ptr<parsed_error>& error) const
    {
        return error == nullptr ? 0 : std::hash<std::string>()(error->error_line);
    }
};

void normalize_log_line(std::string& line)
{
    size_t time_block = line.find(']');
    if (time_block != std::string::npos)
    {
        line = line.substr(time_block + 1);
    }

    size_t id_block = line.find(']');
    if (id_block != std::string::npos)
    {
        line = line.substr(id_block + 1);
    }

    // Trim leading spaces
    size_t start = line.find_first_not_of(" \t");
    if (start != std::string::npos)
    {
        line = line.substr(start);
    }
}

int main()
{
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();

    std::filesystem::path logs_folder_name = "LogsToParse";
    if(!std::filesystem::exists(logs_folder_name))
    {
        std::filesystem::create_directory(logs_folder_name);
        std::cerr << "Directory for logs to parse does not exist, creating: " << logs_folder_name << " now, please try again.";
        return 1;
    }

    std::unordered_set<std::shared_ptr<parsed_error>, parsed_error_hash, parsed_error_equal> parsed_text;
    std::vector<std::string> keywords = {"Error:", "Warning:", "Display:"};
    int error_count = 0;
    int lines_count = 0;

    for(const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(logs_folder_name))
    {
        if(entry.is_regular_file())
        {
            std::ifstream input_stream(entry.path());
            if(!input_stream)
            {
                std::cerr << "Cannot open [" << entry << "] file!";
                return 1;
            }

            std::string line;
            while(getline(input_stream, line))
            {
                lines_count++;
                for(const std::string& keyword : keywords)
                {
                    if(line.find(keyword) != std::string::npos)
                    {
                        normalize_log_line(line);
                        std::shared_ptr<parsed_error> potential_new_error = std::make_shared<parsed_error>();
                        potential_new_error->error_line = line;

                        auto iterator = parsed_text.find(potential_new_error);
                        if(iterator == parsed_text.end())
                        {
                            potential_new_error->error_count = 1;

                            // Naive, category is usually first and verbosity second
                            size_t category_block = line.find(':');
                            if(category_block != std::string::npos)
                            {
                                potential_new_error->error_category = line.substr(0, category_block);
                                size_t error_type_block = line.find(':', category_block + 1);
                                if(category_block != std::string::npos)
                                {
                                    potential_new_error->error_verbosity = line.substr(category_block + 1, error_type_block - category_block - 1);
                                }
                                else
                                {
                                    potential_new_error->error_verbosity = "None";
                                }
                            }
                            else
                            {
                                potential_new_error->error_category = "None";
                            }

                            parsed_text.insert(potential_new_error);
                            error_count++;
                        }
                        else if(iterator != nullptr)
                        {
                            (*iterator)->error_count += 1;
                        }
                    }
                }
            }
        }
    }

    std::ofstream parsed_output_file("ParsingResult.txt");
    if(!parsed_output_file)
    {
        std::cerr << "Failed to open file for writing!";
        return 1;
    }

    parsed_output_file << "Log lines count: " << lines_count << std::endl;
    parsed_output_file << "Errors count: " << error_count << std::endl;

    std::vector<std::shared_ptr<parsed_error>> sorted_parsed_text(parsed_text.begin(), parsed_text.end());
    std::sort(sorted_parsed_text.begin(), sorted_parsed_text.end(),
              [](const std::shared_ptr<parsed_error>& a, const std::shared_ptr<parsed_error>& b)
                    {
                        return a->error_count > b->error_count;
                    });

    for(const std::shared_ptr<parsed_error>& error : sorted_parsed_text)
    {
        parsed_output_file << "\n";
        parsed_output_file << "Original line: " << error->error_line << std::endl;
        parsed_output_file << "Count: " << error->error_count << std::endl;
        parsed_output_file << "Verbosity: " << error->error_verbosity << std::endl;
        parsed_output_file << "Category: " << error->error_category << std::endl;
    }

    std::chrono::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration program_duration = std::chrono::duration_cast<std::chrono::milliseconds >(end - start);
    std::cout << "Execution time: " << program_duration << std::endl;
    return 0;
}
