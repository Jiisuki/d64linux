#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>

namespace d64
{
    //   Bytes:$00-01: Track/Sector location of the first directory sector (should
    //                 be set to 18/1 but it doesn't matter, and don't trust  what
    //                 is there, always go to 18/1 for first directory entry)
    //             02: Disk DOS version type (see note below)
    //                   $41 ("A")
    //             03: Unused
    //          04-8F: BAM entries for each track, in groups  of  four  bytes  per
    //                 track, starting on track 1 (see below for more details)
    //          90-9F: Disk Name (padded with $A0)
    //          A0-A1: Filled with $A0
    //          A2-A3: Disk ID
    //             A4: Usually $A0
    //          A5-A6: DOS type, usually "2A"
    //          A7-AA: Filled with $A0
    //          AB-FF: Normally unused ($00), except for 40 track extended format,
    //                 see the following two entries:
    //          AC-BF: DOLPHIN DOS track 36-40 BAM entries (only for 40 track)
    //          C0-D3: SPEED DOS track 36-40 BAM entries (only for 40 track)
    //
    // Note: The BAM entries for SPEED, DOLPHIN and  ProLogic  DOS  use  the  same layout as standard BAM entries.
    //
    // One of the interesting things from the BAM sector is the byte  at  offset $02, the DOS version byte. If it is set to anything other than $41 or  $00,
    // then we have what is called "soft write protection". Any attempt  to  write to the disk will return the "DOS Version" error code 73  ,"CBM  DOS  V  2.6
    // 1541". The 1541 is simply telling  you  that  it  thinks  the  disk  format version is incorrect. This message will normally come  up  when  you  first
    // turn on the 1541 and read the error channel. If you write a $00 or a $41 into 1541 memory location $00FF (for device 0),  then  you  can  circumvent
    // this type of write-protection, and change the DOS version back to  what it should be.
    //
    // The BAM entries require a bit (no pun intended) more of a breakdown. Take the first entry at bytes $04-$07 ($12 $FF $F9 $17). The first byte ($12) is
    // the number of free sectors on that track. Since we are looking at the track 1 entry, this means it has 18 (decimal) free sectors. The next three  bytes
    // represent the bitmap of which sectors are used/free. Since it is 3 bytes (8 bits/byte) we have 24 bits of storage. Remember that at  most,  each  track
    // only has 21 sectors, so there are a few unused bits.
    //
    //    Bytes: 04-07: 12 FF F9 17   Track 1 BAM
    //           08-0B: 15 FF FF FF   Track 2 BAM
    //           0C-0F: 15 FF FF 1F   Track 3 BAM
    //           ...
    //           8C-8F: 11 FF FF 01   Track 35 BAM
    //
    // These entries must be viewed in binary to make any sense. We will use the first entry (track 1) at bytes 04-07:
    //
    //      FF=11111111, F9=11111001, 17=00010111
    //
    // In order to make any sense from the binary notation, flip the bits around.
    //
    //                    111111 11112222
    //         01234567 89012345 67890123
    //         --------------------------
    //         11111111 10011111 11101000
    //         ^                     ^
    //     sector 0              sector 20
    //
    // Since we are on the first track, we have 21 sectors, and only use  up  to the bit 20 position. If a bit is on (1), the  sector  is  free.  Therefore,
    // track 1 has sectors 9,10 and 19 used, all the rest are free.  Any  leftover bits that refer to sectors that don't exist, like bits 21-23 in  the  above
    // example, are set to allocated.
    //
    // Each filetype has its own unique properties, but most follow  one  simple structure. The first file sector is pointed to by the directory and follows
    // a t/s chain, until the track value reaches  $00.  When  this  happens,  the value in the sector link location indicates how much of the sector is used.
    // For example, the following chain indicates a file 6 sectors long, and  ends when we encounter the $00/$34 chain. At this point the last sector occupies
    // from bytes $02-$34.
    //
    //     1       2       3       4       5       6
    //   ----    -----   -----   -----   -----   -----
    //   17/0    17/10   17/20   17/1    17/11    0/52
    //  (11/00) (11/0A) (11/14) (11/01) (11/0B)  (0/34)

    /* 36-40 not always available */
    static constexpr const unsigned tracks[] = { 1, 2,  3,  4,  5,  6,  7,  8,  9,  10,
                                                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                                21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                                31, 32, 33, 34, 35, 36, 37, 38, 39, 40};

    static constexpr const unsigned sectors[] = {21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
                                                 21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
                                                 19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
                                                 17, 17, 17, 17, 17, 17, 17, 17, 17, 17};

    static constexpr const unsigned offsets[] = {     0,   5376,  10752,  16128,  21504,  26880,  32256,  37632,  43008,  48384,
                                                  53760,  59136,  64512,  69888,  75264,  80640,  86016,  91392,  96256, 101120,
                                                 105984, 110848, 115712, 120576, 125440, 130048, 134656, 139264, 143872, 148480,
                                                 153088, 157440, 161792, 166144, 170496, 174848, 179200, 183552, 187904, 192256};

    static constexpr const unsigned DIR_TRACK = 18;
    static constexpr const unsigned BAM_TRACK = 18;
    static constexpr const unsigned SECTOR_SIZE = 256;
    static constexpr const unsigned DIR_ENTRY_SIZE = 32;
    static constexpr const unsigned NAME_LENGTH = 16;
    static constexpr const unsigned BLOCK_SIZE = 254;

    ///\brief Diskette size type.
    enum class SizeType : unsigned
    {
        Standard = 35,
        Ext1 = 36,
        Ext2 = 37,
        Ext3 = 38,
        Ext4 = 39,
        Ext5 = 40
    };

    ///\brief Data type for a byte.
    using byte = std::uint8_t;

    ///\brief Short for a byte vector.
    using byte_vector = std::vector<byte>;

    ///\brief Short for a fixed size byte array.
    template<std::size_t s>
    using byte_array = std::array<byte, s>;

    ///\brief Converts PetASCII to a normal string for using reading.
    static std::string pet_ascii_to_string(const byte_vector& binary_data)
    {
        std::string str {};
        for (const auto& b : binary_data)
        {
            if (b < 32 || (127 <= b && b < 193) || 219 <= b)
            {
                str.push_back(static_cast<char>(32));
            }
            else if (193 <= b && b < 219)
            {
                str.push_back(static_cast<char>(b - 161));
            }
            else
            {
                str.push_back(static_cast<char>(b));
            }
        }
        return str;
    }

    static byte_vector read_file_binary(const std::string& filename)
    {
        auto fs = std::ifstream(filename, std::ios::binary);
        return {(std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>()};
    }

    static void assert_track(unsigned track_number)
    {
        if (0 == track_number)
        {
            throw std::runtime_error("Track number must be non-zero.");
        }
    }

    class DiskSector
    {
    private:
        byte_array<SECTOR_SIZE> data;

    public:
        DiskSector() : data() {}
        ~DiskSector() = default;

        [[nodiscard]] bool free() const
        { return !std::any_of(data.begin(), data.end(), [](const byte& v){ return 0 != v; }); }

        [[nodiscard]] byte_array<SECTOR_SIZE> get_sector_data() const
        { return data; }

        byte& operator[](unsigned index)
        { return data[index]; }

        [[nodiscard]] byte operator[](unsigned index) const
        { return data[index]; }

        [[nodiscard]] byte_vector get_bytes(unsigned offset, unsigned count) const
        { byte_vector vector (count); std::copy(data.begin() + offset, data.begin() + offset + count, vector.begin()); return vector; }

        template<std::size_t s>
        [[nodiscard]] byte_array<s> get_bytes(unsigned offset) const
        { byte_array<s> array {}; std::copy(data.begin() + offset, data.begin() + offset + s, array.begin()); return array; }

        void set_bytes(const byte_vector& vector, unsigned offset)
        { std::copy(vector.begin(), vector.end(), data.begin() + offset); }

        template<std::size_t s>
        void set_bytes(const byte_array<s>& array, unsigned offset)
        { std::copy(array.begin(), array.end(), data.begin() + offset); }
    };

    class DiskTrack
    {
    private:
        std::vector<DiskSector> sector;
        unsigned offset;

    public:
        explicit DiskTrack (unsigned track_number) : sector()
        {
            assert_track(track_number);

            for (auto i = 0; i < sectors[track_number - 1]; i++)
            {
                sector.emplace_back();
            }

            offset = offsets[track_number - 1];
        }

        DiskSector& operator[](unsigned index)
        { return sector[index]; }

        DiskSector operator[](unsigned index) const
        { return sector[index]; }

        [[nodiscard]] unsigned get_offset() const
        { return offset; }

        [[nodiscard]] std::size_t size() const
        { return sector.size(); }
    };

    class Entry
    {
    private:
        std::string title;
        std::string prg_extension;
        byte next_dir_track;
        byte next_dir_sector;
        byte on_track;
        byte on_sector;
        unsigned block_size;
        std::string name;

    public:
        Entry() : title(), prg_extension(), next_dir_track(), next_dir_sector(), on_track(), on_sector(), block_size(), name(NAME_LENGTH, ' ')
        {}

        ~Entry() = default;

        void set_title(const byte_vector& petascii)
        { title = pet_ascii_to_string(petascii); }

        [[nodiscard]] std::string get_title() const
        { return title; }

        void set_name(const std::string& prg_name)
        { name = prg_name; if (name.length() < NAME_LENGTH) { name.append(std::string(NAME_LENGTH - name.length(), ' ')); } }

        [[nodiscard]] byte_array<NAME_LENGTH> get_name() const
        { byte_array<NAME_LENGTH> n {}; std::fill(n.begin(), n.end(), ' '); std::copy(name.begin(), name.end(), n.begin()); return n; }

        void set_prg_extension(const std::string &str)
        { prg_extension = str; }

        [[nodiscard]] std::string get_prg_extension() const
        { return prg_extension; }

        void set_next_dir_track(byte value)
        { next_dir_track = value; }

        [[nodiscard]] byte get_next_dir_track() const
        { return next_dir_track; }

        void set_next_dir_sector(byte value)
        { next_dir_sector = value; }

        [[nodiscard]] byte get_next_dir_sector() const
        { return next_dir_sector; }

        void set_first_track(byte value)
        { on_track = value; }

        [[nodiscard]] byte get_first_track() const
        { return on_track; }

        void set_first_sector(byte value)
        { on_sector = value; }

        [[nodiscard]] byte get_first_sector() const
        { return on_sector; }

        void set_block_size(const byte_vector& bl_bytes)
        { if (bl_bytes.size() < 2) return; block_size = bl_bytes[0] + bl_bytes[1] * SECTOR_SIZE; }

        [[nodiscard]] unsigned get_block_size() const
        { return block_size; }

        [[nodiscard]] byte_array<2> get_block_size_array() const
        { return {static_cast<byte>((block_size / SECTOR_SIZE) & 0xFF), static_cast<byte>(((block_size / SECTOR_SIZE) >> 8u) & 0xFF)}; }
    };

    class Program
    {
    private:
        byte_vector data;
        std::string filename;
        std::string name;

    public:
        Program() : data(), filename(), name()
        {}

        explicit Program(const std::string& file) : filename(file)
        {
            if (16 < file.length())
            {
                name = file.substr(file.length() - 20, 16);
            }
            else
            {
                name = "      ----      ";
            }

            data = read_file_binary(file);
        }

        ~Program() = default;

        [[nodiscard]] byte_vector get_data() const
        { return data; }

        [[nodiscard]] std::size_t size() const
        { return data.size(); }

        [[nodiscard]] std::string get_filename() const
        { return filename; }

        [[nodiscard]] std::string get_name() const
        {
            if (name.length() < NAME_LENGTH)
            {
                std::string str = name;
                str.append(std::string(NAME_LENGTH - name.length(), ' '));
                return str;
            }
            else
            {
                return name;
            }
        }
    };

    class d64
    {
    private:
        std::vector<DiskTrack> image;
        std::string disk_name;
        byte disk_dos;
        byte_array<2> disk_id;
        byte_array<0x8C> disk_bam;
        std::vector<Entry> directory;

        void read_bam()
        {
            // filename for disk in offset+144 to offset+159
            const auto& sector = image[BAM_TRACK - 1][0];

            disk_id = sector.get_bytes<2>(0xA2);
            disk_bam = sector.get_bytes<0x8C>(0x04);
            disk_dos = sector[0x02];
            disk_name = pet_ascii_to_string(sector.get_bytes(0x90, NAME_LENGTH));
        }

        void read_dir()
        {
            unsigned cSector = 1;
            unsigned cTrack = DIR_TRACK - 1;
            unsigned nextTrack = 0;
            unsigned nextSector = 0;
            byte entryFT = 0;

            while (true)
            {
                for (auto k = 0u; k < 8; k++)
                {
                    auto offset = k * 32;
                    Entry new_entry {};
                    new_entry.set_next_dir_track(image[cTrack][cSector][offset]);
                    new_entry.set_next_dir_sector(image[cTrack][cSector][offset + 1]);
                    if (0 == k)
                    {
                        nextTrack = new_entry.get_next_dir_track();
                        nextSector = new_entry.get_next_dir_sector();
                    }
                    entryFT = image[cTrack][cSector][offset + 2];
                    new_entry.set_prg_extension(get_file_type(entryFT));
                    new_entry.set_first_track(image[cTrack][cSector][offset + 3]);
                    new_entry.set_first_sector(image[cTrack][cSector][offset + 4]);
                    new_entry.set_title(image[cTrack][cSector].get_bytes(offset + 5, NAME_LENGTH));
                    new_entry.set_block_size(image[cTrack][cSector].get_bytes(offset + 30, 2));

                    if (0 != entryFT)
                    {
                        directory.push_back(new_entry);
                    }
                }

                if (0 == nextTrack)
                {
                    return;
                }
                else
                {
                    cTrack = nextTrack - 1;
                    cSector = nextSector;
                }
            }
        }

        static std::string get_file_type(byte file_type)
        {
            switch (file_type & 0x07)
            {
                case 0: return "DEL";
                case 1: return "SEQ";
                case 2: return "PRG";
                case 3: return "USR";
                case 4: return "REL";
                default: return "*";
            }
        }

    public:
        d64() : image(), disk_name(), disk_dos(), disk_id(), disk_bam(), directory()
        { format(SizeType::Standard); }

        explicit d64(const std::vector<DiskTrack>& new_image) : image(), disk_name(), disk_dos(), disk_id(), disk_bam(), directory()
        { format(SizeType::Standard); image = new_image; read_bam(); read_dir(); }

        void load(const std::string& filename)
        {
            format(SizeType::Standard);
            auto bin = read_file_binary(filename);

            unsigned curTrack = 0;
            unsigned curSector = 0;
            byte curByte = 0;

            for (const auto& b : bin)
            {
                image[curTrack][curSector][curByte] = b;
                if (0 == ++curByte)
                {
                    /* We spilled over. */
                    if (sectors[curTrack] <= ++curSector)
                    {
                        /* Track filled. */
                        curSector = 0;
                        curTrack++;
                    }
                }
            }

            read_bam();
            read_dir();
        }

        void format(SizeType size_type)
        {
            image.clear();
            for (auto i = 0u; i < static_cast<int>(size_type); i++)
            {
                image.emplace_back(i + 1);
            }
            disk_name = "";
            disk_dos = 0x41u;
            std::fill(disk_id.begin(), disk_id.end(), 0x00);
            std::fill(disk_bam.begin(), disk_bam.end(), 0xFF);
            directory.clear();
        }

        [[nodiscard]] std::vector<bool> track_space_free(unsigned track) const
        {
            assert_track(track);
            std::vector<bool> is_free (sectors[track - 1], false);
            if (track <= image.size())
            {
                for (auto i = 0; i < is_free.size(); i++)
                {
                    is_free[i] = image[track - 1][i].free();
                }
            }
            return is_free;
        }

        [[nodiscard]] std::string get_disk_name() const
        { return disk_name; }

        [[nodiscard]] unsigned number_of_entries() const
        { return directory.size(); }

        [[nodiscard]] std::vector<Entry> get_directory() const
        { return directory; }

        [[nodiscard]] unsigned get_disk_size() const
        { return image.size(); }

        [[nodiscard]] DiskSector read_sector(unsigned track, unsigned sector) const
        { return image[track - 1][sector]; }

        [[nodiscard]] std::vector<DiskTrack> get_disk_image() const
        { return image; }

        void write_disk_byte(unsigned track, unsigned sector, unsigned byte_index, byte b)
        {
            assert_track(track);
            image[track - 1][sector][byte_index] = b;
        }

        void add_prg(const Program& program)
        {
            Entry new_entry {};

            unsigned t = 0;
            unsigned s = 0;
            unsigned nt = 0;
            unsigned ns = 0;

            /* Check for next free track/sector. */
            while (!image[t][s].free())
            {
                if (sectors[t] <= ++s)
                {
                    s = 0;

                    if (image.size() <= ++t)
                    {
                        return; // full disk
                    }

                    if ((DIR_TRACK - 1) == t)
                    {
                        /* Directory. */
                        t++;
                    }
                }
            }
            nt = t;
            ns = s;

            new_entry.set_first_track(t);
            new_entry.set_first_sector(s);
            new_entry.set_name(program.get_name());
            new_entry.set_block_size({static_cast<byte>((program.size() / SECTOR_SIZE) & 0xFF), static_cast<byte>(((program.size() / SECTOR_SIZE) >> 8u) & 0xFF)});
            directory.push_back(new_entry);

            auto prg_data = program.get_data();
            unsigned b = 0;

            while (b < prg_data.size())
            {
                for (auto k = 0u; k < SECTOR_SIZE; k++)
                {
                    if (0 == k)
                    {
                        if (sectors[t] <= ++ns)
                        {
                            if ((DIR_TRACK - 1) == ++nt) // directory!
                            {
                                nt++;
                            }
                            ns = 0;
                        }
                        image[t][s][k] = nt + 1;
                    }
                    else if (1 == k)
                    {
                        image[t][s][k] = ns;
                    }
                    else
                    {
                        image[t][s][k] = prg_data[b];
                        if (prg_data.size() <= ++b)
                        {
                            // we reached the end before sector is finished
                            image[t][s][0] = 0;
                            image[t][s][1] = 0;
                            break;
                        }
                    }

                    if ((SECTOR_SIZE - 1) == k)
                    {
                        s = ns;
                        t = nt;
                    }
                }
            }
        }

        void generate_disk(const std::vector<Program>& programs, const std::string& name)
        {
            format(SizeType::Standard);
            disk_name = name;

            for (const auto& prg : programs)
            {
                add_prg(prg);
            }

            /* write directory */
            unsigned t = DIR_TRACK - 1;
            unsigned s = 1;
            unsigned ns = s;
            unsigned nt = t;

            unsigned offset = 0;

#if 1
            for (const auto& e : directory)
            {
                if (0 == offset)
                {
                    if (sectors[t] <= ++ns)
                    {
                        ns = 0;
                        nt++;
                    }

                    /* todo check last program */
                    if (false)
                    {

                    }
                    else
                    {
                        image[t][s][offset] = nt;
                        image[t][s][offset + 1] = ns;
                    }
                }
                else
                {
                    image[t][s][offset] = 0;
                    image[t][s][offset + 1] = 0;
                }

                image[t][2][offset + 2] = 0x82;
                image[t][s][offset + 3] = e.get_first_track();
                image[t][s][offset + 4] = e.get_first_sector();
                image[t][s].set_bytes(e.get_name(), 5);
                image[t][s].set_bytes(e.get_block_size_array(), 0x1E);

                offset += DIR_ENTRY_SIZE;
                if (SECTOR_SIZE <= offset)
                {
                    offset = 0;
                    t = nt;
                    s = ns;
                }
            }
#else
            for (auto index = 0u; index < programs.size(); index++)
            {
                const auto& prg = programs[index];

                if (0 == offset)
                {
                    if (sectors[t] <= ++ns)
                    {
                        ns = 0;
                        nt++;
                    }

                    if (index == (programs.size() - 1))
                    {
                        image[t][s][offset] = 0;
                        image[t][s][offset + 1] = 0;
                    }
                    else
                    {
                        image[t][s][offset] = nt;
                        image[t][s][offset + 1] = ns;
                    }
                }
                else
                {
                    image[t][s][offset] = 0;
                    image[t][s][offset + 1] = 0;
                }

                image[t][s][offset + 2] = 0x82;
                image[t][s][offset + 3] = directory[index].get_first_track();
                image[t][s][offset + 4] = directory[index].get_first_sector();
                auto prg_name = prg.get_name();
                for (auto k = 0u; k < NAME_LENGTH; k++)
                {
                    image[t][s][offset + 5 + k] = prg_name[k];
                }
                image[t][s][offset + 0x1E] = (prg.size() / SECTOR_SIZE) & 0xFF;
                image[t][s][offset + 0x1F] = (prg.size() / SECTOR_SIZE) >> 8u;

                offset += DIR_ENTRY_SIZE;
                if (255 < offset)
                {
                    offset = 0;
                    t = nt;
                    s = ns;
                }
            }
#endif

            /* Clear directory and read back, to verify it is correct. */
            directory.clear();
            read_bam();
            read_dir();
        }

        void save_disk(const std::string& filename)
        {
            std::ofstream out (filename, std::ios::binary);
            for (const auto& t : image)
            {
                for (auto s = 0u; s < t.size(); s++)
                {
                    auto sector = t[s].get_sector_data();
                    for (const auto& b : sector)
                    {
                        out.put(static_cast<char>(b));
                    }
                }
            }
        }
    };

}
