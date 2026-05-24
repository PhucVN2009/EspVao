#include <link.h>

namespace RemapTools {
    class ProcMapInfo {
    private:
        uintptr_t start;
        uintptr_t end;
        uintptr_t offset;
        uint8_t perms;
        ino_t inode;
        char* dev;
        char* path;

        friend class Remapper;
    };

    class Remapper {
    private:
        static int iteratePhdrCallback(struct dl_phdr_info* info, size_t size, void* data);
        static std::vector<ProcMapInfo> ListModulesWithNameImpl(std::string name);
        static void RemapLibraryImpl(const char* libName);

    public:
        Remapper();
        ~Remapper();

        static void Remap(const char* libname);
    };

    Remapper::Remapper() {
    }

    Remapper::~Remapper() {
    }

    std::vector<ProcMapInfo> Remapper::ListModulesWithNameImpl(std::string name) {
        std::vector<ProcMapInfo> returnVal;

        char buffer[512];
        FILE *fp = fopen(oxorany("/proc/self/maps"), oxorany("re"));
        if (fp != nullptr) {
            while (fgets(buffer, sizeof(buffer), fp)) {
                if (strstr(buffer, name.c_str())) {
                    ProcMapInfo info{};
                    char perms[10];
                    char path[255];
                    char dev[25];

                    sscanf(buffer, oxorany("%lx-%lx %s %ld %s %ld %s"), &info.start, &info.end, perms, &info.offset, dev, &info.inode, path);

                    //Process Perms
                    if (strchr(perms, 'r')) info.perms |= PROT_READ;
                    if (strchr(perms, 'w')) info.perms |= PROT_WRITE;
                    if (strchr(perms, 'x')) info.perms |= PROT_EXEC;
                    if (strchr(perms, 'r')) info.perms |= PROT_READ;

                    //Set all other information
                    info.dev = dev;
                    info.path = path;

                    returnVal.push_back(info);
                }
            }
        }
        return returnVal;
    }

    void Remapper::RemapLibraryImpl(const char* libName) {
        dl_iterate_phdr(Remapper::iteratePhdrCallback, const_cast<char*>(libName));
    }

    int Remapper::iteratePhdrCallback(struct dl_phdr_info* info, size_t size, void* data) {
        const char* libraryName = static_cast<const char*>(data);
        if (info->dlpi_name && strstr(info->dlpi_name, libraryName)) {
            std::vector<ProcMapInfo> maps = Remapper().ListModulesWithNameImpl(libraryName);
            for (ProcMapInfo info : maps) {
                void *address = (void *)info.start;
                size_t size = info.end - info.start;
                void *map = mmap(0, size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

                if ((info.perms & PROT_READ) == 0) {
                    mprotect(address, size, PROT_READ);
                }

                if (map == nullptr) {
                    perror(oxorany("mmap failed"));
                    return 1;
                }

                std::memmove(map, address, size);
                mremap(map, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, info.start);

                mprotect((void *)info.start, size, info.perms);
            }
        }
        return 0;
    }

    void Remapper::Remap(const char* libname) {
        RemapLibraryImpl(libname);
    }
}