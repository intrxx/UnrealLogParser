#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <filesystem>
#include <unordered_set>

#include "../include/cxxopts.hpp"

#include <chrono>

namespace parser_options
{
    bool with_telemetry = false;
    std::string minimal_verbosity;
    std::filesystem::path optional_folder_path;
    std::filesystem::path optional_result_path;
}

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

void normalize_log_line(std::string& out_line)
{
    size_t time_block_end_position = out_line.find(']');
    if (time_block_end_position != std::string::npos)
    {
        out_line = out_line.substr(time_block_end_position + 1);
    }

    size_t id_block_end_position = out_line.find(']');
    if (id_block_end_position != std::string::npos)
    {
        out_line = out_line.substr(id_block_end_position + 1);
    }
}

bool construct_keywords(std::vector<std::string>& out_keywords)
{
    std::transform(parser_options::minimal_verbosity.begin(),
                   parser_options::minimal_verbosity.end(),
                   parser_options::minimal_verbosity.begin(),
                   [](unsigned char c)
        {
            return std::tolower(c);
        });

    static const std::vector<std::pair<std::string, std::string>> verbosity_levels = {
            {"error", "Error:"},
            {"warning", "Warning:"},
            {"display", "Display:"}
    };

    out_keywords.clear();
    for (const auto& [key, value] : verbosity_levels)
    {
        out_keywords.push_back(value);
        if (key == parser_options::minimal_verbosity)
        {
            break;
        }
    }

    return !out_keywords.empty();
}

bool parse_arguments(int argc, char *argv[])
{
    try
    {
        std::unique_ptr<cxxopts::Options> allocated(new cxxopts::Options(argv[0], " -t -v display -p \\mydirectorytoparse "));
        cxxopts::Options &options = *allocated;
        options.positional_help("[optional args]").show_positional_help();

        options.add_options()
                ("h,help", "Print help")
                ("t,telemetry", "Enable telemetry for execution time.")
                ("v,verbosity", "Minimum log line verbosity to include in result (error|warning|display).", cxxopts::value<std::string>()->default_value("warning"))
                ("p,path", "Optional path to folder which contains logs to parse.", cxxopts::value<std::string>())
                ("r,result", "Optional result path to create the ParsingResult.txt", cxxopts::value<std::string>());


        options.parse_positional({"input", "output", "positional"});
        cxxopts::ParseResult result = options.parse(argc, argv);

        if(result.count("help"))
        {
            std::cout << options.help({"", "Group"}) << std::endl;
            return false;
        }

        if(result.count("t"))
        {
            std::cout << "Parsing with telemetry." << std::endl;
            parser_options::with_telemetry = true;
        }

        parser_options::minimal_verbosity = result["v"].as<std::string>();

        if(result.count("p"))
        {
            parser_options::optional_folder_path = result["p"].as<std::string>();
            if(!std::filesystem::exists(parser_options::optional_folder_path))
            {
                std::cerr << "Provided " << parser_options::optional_folder_path << " [-p] folder path is invalid";
                return false;
            }
            std::cout << "Parsing files from path: " << parser_options::optional_folder_path << std::endl;
        }

        if(result.count("r"))
        {
            parser_options::optional_result_path = result["r"].as<std::string>();
            if(!std::filesystem::exists(parser_options::optional_result_path))
            {
                std::cerr << "Provided " << parser_options::optional_result_path << " [-r] result path is invalid";
                return false;
            }
            std::cout << "Parsing result will be created in: " << parser_options::optional_result_path << std::endl;
        }
    }
    catch (const cxxopts::exceptions::exception& arg_parsing_error)
    {
        std::cout << "Error while trying to parse options: " << arg_parsing_error.what() << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    if(!parse_arguments(argc, argv))
    {
        return 1;
    }

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> start;
    if(parser_options::with_telemetry)
    {
        start = std::chrono::high_resolution_clock::now();
    }

    bool create_default = false;
    std::filesystem::path logs_folder_name = parser_options::optional_folder_path;
    if(!std::filesystem::exists(logs_folder_name))
    {
        create_default = true;
        logs_folder_name = "LogsToParse";
    }

    if(!std::filesystem::exists(logs_folder_name))
    {
        std::string error_string = "Directory for logs to parse does not exist";
        if(create_default)
        {
            std::filesystem::create_directory(logs_folder_name);
            error_string += " creating default folder LogsToParse now, please try again.";
        }
        else
        {
            error_string += " please provide a correct path.";
        }

        std::cerr << error_string;
        return 1;
    }

    std::vector<std::string> keywords;
    if(!construct_keywords(keywords))
    {
        std::cerr << "Failed to construct keywords!";
        return 1;
    }

    uint32_t lines_count = 0;
    uint32_t unique_captured_lines_count = 0;
    std::unordered_set<std::shared_ptr<parsed_error>, parsed_error_hash, parsed_error_equal> parsed_text;
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
                            size_t category_block_end_position = line.find(':');
                            if(category_block_end_position != std::string::npos)
                            {
                                potential_new_error->error_category = line.substr(0, category_block_end_position);
                                size_t error_type_block_end_position = line.find(':', category_block_end_position + 1);
                                if(category_block_end_position != std::string::npos)
                                {
                                    potential_new_error->error_verbosity = line.substr(
                                            category_block_end_position + 1, error_type_block_end_position - category_block_end_position - 1);
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
                            unique_captured_lines_count++;
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

    std::filesystem::path result_path = "ParsingResult.txt";
    if(std::filesystem::exists(parser_options::optional_result_path))
    {
        result_path = parser_options::optional_result_path / result_path;
    }

    std::ofstream parsed_output_file(result_path);
    if(!parsed_output_file)
    {
        std::cerr << "Failed to open file for writing!";
        return 1;
    }

    parsed_output_file << "  PARSING DATA  " << std::endl;
    parsed_output_file << "Log lines count: " << lines_count << std::endl;
    parsed_output_file << "Unique lines captured: " << unique_captured_lines_count << std::endl;

    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>> parsing_end;
    if(parser_options::with_telemetry && start)
    {
        parsing_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration parsing_duration = std::chrono::duration_cast<std::chrono::milliseconds>(*parsing_end - *start);
        std::cout << "Parsing took: " << parsing_duration << std::endl;
        parsed_output_file << "Parsing took: " << parsing_duration << std::endl;
    }

    std::vector<std::shared_ptr<parsed_error>> sorted_parsed_text(parsed_text.begin(), parsed_text.end());
    std::sort(sorted_parsed_text.begin(), sorted_parsed_text.end(),
              [](const std::shared_ptr<parsed_error>& a, const std::shared_ptr<parsed_error>& b)
              {
                  return a->error_count > b->error_count;
              });

    if(parser_options::with_telemetry && parsing_end)
    {
        std::chrono::time_point sorting_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration sorting_duration = std::chrono::duration_cast<std::chrono::milliseconds>(sorting_end - *parsing_end);
        std::cout << "Sorting took: " << sorting_duration << std::endl;
        parsed_output_file << "Sorting took: " << sorting_duration << std::endl;
    }

    for(const std::shared_ptr<parsed_error>& error : sorted_parsed_text)
    {
        parsed_output_file << "\n";
        parsed_output_file << "Original line: " << error->error_line << std::endl;
        parsed_output_file << "Count: " << error->error_count << std::endl;
        parsed_output_file << "Verbosity: " << error->error_verbosity << std::endl;
        parsed_output_file << "Category: " << error->error_category << std::endl;
    }

    if(parser_options::with_telemetry && start)
    {
        std::chrono::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration program_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - *start);
        std::cout << "Execution time: " << program_duration << std::endl;
    }
    return 0;
}
