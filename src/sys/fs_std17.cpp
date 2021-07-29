// implementation of util_sys.hpp using standard library functions from c++17

#ifdef CPP_FILESYSTEM_ENABLED

#include "ogm/sys/util_sys.hpp"
#include "fs_share.hpp"

#include <filesystem>
#include <random>

namespace ogm {

bool create_directory(const std::string& path)
{
    return std::filesystem::create_directory(
        std::filesystem::path(path)
    );
}

bool remove_directory(const std::string& path)
{
    return std::filesystem::remove_all(
        std::filesystem::path(path)
    );
}

std::string get_temp_root()
{
    return std::filesystem::temp_directory_path().string();
}

namespace {
    // https://stackoverflow.com/a/7114482
    typedef std::mt19937 MyRNG;  // the Mersenne Twister with a popular choice of parameters
    unsigned int seed_val = static_cast<unsigned int>(time(nullptr));           // populate somehow
    MyRNG rng{ seed_val };
    std::uniform_int_distribution<uint32_t> uint_dist;
}

std::string create_temp_directory()
{
    std::string root = std::filesystem::temp_directory_path().string();
    static int c = 0;

    while (true)
    {
        std::string subfolder = "ogm-tmp" + std::to_string(uint_dist(rng)) + "-" + std::to_string(c++);
        std::string joined = path_join(root, subfolder);
        if (!path_exists(joined))
        {
            create_directory(joined);
            return joined + std::string(1, PATH_SEPARATOR);
        }
    }
}

void list_paths(const std::string& base, std::vector<std::string>& out)
{
    if (!path_exists(base)) return;
    for (auto& p : std::filesystem::directory_iterator(base))
    {
        out.emplace_back(p.path().string());
    }
}

void list_paths_recursive(const std::string& base, std::vector<std::string>& out)
{
    if (!path_exists(base)) return;
    for (auto& p : std::filesystem::recursive_directory_iterator(base))
    {
        out.emplace_back(p.path().string());
    }
}

}

#endif