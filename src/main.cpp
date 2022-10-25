#include <iostream>
#include <iomanip>
#include <cmath>
#include <deque>
#include <utility>
#include "../lib/d64.hpp"

void show_compilation_list(const std::vector<d64::Program>& programs);
void show_data(const d64::d64& disk, int track, int sector, bool ascii = false);
void show_bam(const d64::d64& disk);
void show_directory(const d64::d64& disk);

enum class Operations
{
    ShowDirectory,
    ShowPartitioning,
    FormatDisk,
    AddProgram,
    CreateDisk,
};

struct Operation
{
    Operations op;
    std::string arg;
    explicit Operation(Operations perf, std::string argument = {}) : op(perf), arg(std::move(argument)) {}
    ~Operation() = default;
};

static void print_usage()
{
    std::cout << "d64 [options] file" << std::endl << std::endl;
    std::cout << "\t-h\t\tPrint help information" << std::endl;
    std::cout << "\t-d       \tShows disk directory information." << std::endl;
    std::cout << "\t-p       \tShows disk partitioning information." << std::endl;
    std::cout << "\t-f       \tFormats the disk." << std::endl;
    std::cout << "\t-a <prg> \tAdd a program that will be burned to the disk upon save. Only the list of programs will be added." << std::endl;
    std::cout << "\t-o <disk>\tCreates and saves a disk." << std::endl;
    std::cout << std::endl;
    std::cout << "Example to show partitioning and contents of an existing disk:" << std::endl;
    std::cout << "\td64 mydisk.d64 -p -d" << std::endl;
    std::cout << std::endl;
    std::cout << "Example to create a new disk with some programs:" << std::endl;
    std::cout << "\td64 -a program1.prg -a program2.prg -o mydisk.d64" << std::endl;
    std::cout << std::endl;
    std::cout << "Example to create a blank disk:" << std::endl;
    std::cout << "\td64 -f -o mydisk.d64" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Program requires input parameters." << std::endl << std::endl;
        print_usage();
        return 1;
    }

    const auto assert_argument = [](int argc, int i) { if (argc <= (i + 1)) { std::cerr << "Invalid argument." << std::endl; print_usage(); return true; } return false; };

    d64::d64 disk {};
    std::deque<Operation> operations {};

    for (auto i = 0; i < argc; i++)
    {
        if ('-' == argv[i][0])
        {
            switch (argv[i][1])
            {
                case 'o':
                    if (assert_argument(argc, i)) { return 1; }
                    operations.emplace_back(Operations::CreateDisk, argv[i + 1]);
                    i++;
                    break;

                case 'p':
                    operations.emplace_back(Operations::ShowPartitioning);
                    break;

                case 'd':
                    operations.emplace_back(Operations::ShowDirectory);
                    break;

                case 'f':
                    operations.emplace_back(Operations::FormatDisk);
                    break;

                case 'a':
                    if (assert_argument(argc, i)) { return 1; }
                    operations.emplace_back(Operations::AddProgram, argv[i + 1]);
                    i++;
                    break;

                default:
                case 'h':
                    print_usage();
                    return 1;
            }
        }
        else if (0 < i)
        {
            disk.load(argv[i]);
        }
    }

    std::vector<d64::Program> programs {};

    while (!operations.empty())
    {
        auto op = operations.front();
        operations.pop_front();

        switch (op.op)
        {
            case Operations::FormatDisk:
                disk.format(d64::SizeType::Standard);
                break;

            case Operations::ShowPartitioning:
                show_bam(disk);
                break;

            case Operations::ShowDirectory:
                show_directory(disk);
                break;

            case Operations::AddProgram:
                std::cout << "Adding program '" << op.arg << "'" << std::endl;
                programs.emplace_back(op.arg);
                break;

            case Operations::CreateDisk:
                if (programs.empty())
                {
                    std::cout << "\033[031mWarning: No programs specified, creating empty disk.\033[0m" << std::endl;
                }
                else
                {
                    disk.generate_disk(programs, "NULL");
                }
                std::cout << "Saving disk to '" << op.arg << "'" << std::endl;
                disk.save_disk(op.arg);
                break;

            default:
                break;
        }
    }

    return 0;
}

void show_compilation_list(const std::vector<d64::Program>& programs)
{
    for (const auto& prg : programs)
    {
        std::cout << prg.get_name() << "   " << std::to_string(std::ceil(prg.size() / 256)) << " sectors." << std::endl;
    }
}

void show_data(const d64::d64& disk, int track, int sector, bool ascii)
{
    auto disk_sector = disk.read_sector(track + 1, sector);
    auto data = disk_sector.get_sector_data();

    if (ascii)
    {
        for (auto ix = 0; ix < d64::SECTOR_SIZE; ix += 32)
        {
            auto cnt = std::min(32u, d64::SECTOR_SIZE - ix);
            auto line = std::vector<std::uint8_t>(disk_sector.get_bytes(ix, cnt));
            std::cout << d64::pet_ascii_to_string(line) << std::endl;
        }
    }
    else
    {
        for (auto ix = 0; ix < d64::SECTOR_SIZE; ix += 16)
        {
            auto cnt = std::min(16u, d64::SECTOR_SIZE - ix);
            auto line = disk_sector.get_bytes(ix, cnt);
            for (const auto &b: line)
            {
                std::cout << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<unsigned>(b) << " ";
            }
            std::cout << std::endl;
        }
    }
}

void show_bam(const d64::d64& disk)
{
    unsigned data_usage = 0;
    unsigned disk_size_sect = 0;

    for (auto track = 1u; track <= disk.get_disk_size(); track++ )
    {
        auto is_free = disk.track_space_free(track);
        disk_size_sect += is_free.size();

        if (d64::sectors[track - 1] != is_free.size())
        {
            std::cout << "Sector size mismatch.";
        }
        else
        {
            for (const auto& f : is_free)
            {
                if (f)
                {
                    std::cout << "\u25A1 ";
                }
                else
                {
                    std::cout << "\u25A0 ";
                    data_usage++;
                }
            }
        }
        std::cout << std::endl;
    }

    std::cout << std::to_string((100 - (100 * data_usage) / disk_size_sect)) << " % free space." << std::endl;
}

void show_directory(const d64::d64& disk)
{
    auto dir = disk.get_directory();
    for (const auto& d : dir)
    {
        std::cout << d.get_title() << "   " << std::setfill('0') << std::setw(3) << d.get_block_size() << " blocks   " << d.get_prg_extension() << std::endl;
    }
}
