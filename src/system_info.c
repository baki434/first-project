#include "system_info.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

typedef LONG(WINAPI *rtl_get_version_t)(OSVERSIONINFOW *version);

static const char *get_architecture_name(WORD architecture)
{
    switch (architecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x86_64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    default:
        return "Bilinmiyor";
    }
}

int get_system_info(char *buffer, size_t size)
{
    OSVERSIONINFOW version = {0};
    SYSTEM_INFO system_data;
    MEMORYSTATUSEX memory = {0};
    char processor[256] = "Bilinmiyor";
    DWORD processor_size = sizeof(processor);
    rtl_get_version_t rtl_get_version = NULL;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    int written;

    if (buffer == NULL || size == 0) {
        return 0;
    }

    version.dwOSVersionInfoSize = sizeof(version);
    if (ntdll != NULL) {
        FARPROC function_address = GetProcAddress(ntdll, "RtlGetVersion");

        if (function_address != NULL) {
            memcpy(&rtl_get_version, &function_address,
                   sizeof(rtl_get_version));
        }
    }
    if (rtl_get_version == NULL || rtl_get_version(&version) != 0) {
        if (!GetVersionExW(&version)) {
            return 0;
        }
    }

    GetNativeSystemInfo(&system_data);
    memory.dwLength = sizeof(memory);
    if (!GlobalMemoryStatusEx(&memory)) {
        return 0;
    }
    if (GetEnvironmentVariableA("PROCESSOR_IDENTIFIER", processor,
                                processor_size) == 0) {
        snprintf(processor, sizeof(processor), "%s",
                 get_architecture_name(system_data.wProcessorArchitecture));
    }

    written = snprintf(
        buffer, size,
        "OS: Windows %lu.%lu (Build %lu)\n"
        "CPU: %s, %lu mantiksal cekirdek\n"
        "RAM: %llu MB",
        (unsigned long)version.dwMajorVersion,
        (unsigned long)version.dwMinorVersion,
        (unsigned long)version.dwBuildNumber,
        processor,
        (unsigned long)system_data.dwNumberOfProcessors,
        (unsigned long long)(memory.ullTotalPhys / (1024ULL * 1024ULL)));

    return written >= 0 && (size_t)written < size;
}

#else
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

static void trim_text(char *text)
{
    char *start = text;
    size_t length;

    while (*start == ' ' || *start == '\t') {
        start++;
    }
    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    length = strlen(text);
    while (length > 0 &&
           (text[length - 1] == ' ' || text[length - 1] == '\t' ||
            text[length - 1] == '\r' || text[length - 1] == '\n')) {
        text[--length] = '\0';
    }
}

static void read_cpu_name(char *buffer, size_t size)
{
    FILE *file = fopen("/proc/cpuinfo", "r");
    char line[512];
    char model_name[256] = "Bilinmiyor";

    if (file == NULL) {
        snprintf(buffer, size, "%s", model_name);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *separator = strchr(line, ':');

        if (separator == NULL) {
            continue;
        }
        *separator = '\0';
        trim_text(line);
        trim_text(separator + 1);

        if (strcmp(line, "Model") == 0) {
            snprintf(model_name, sizeof(model_name), "%s", separator + 1);
            break;
        }
        if (strcmp(line, "model name") == 0 &&
            strcmp(model_name, "Bilinmiyor") == 0) {
            snprintf(model_name, sizeof(model_name), "%s", separator + 1);
        }
    }

    fclose(file);
    snprintf(buffer, size, "%s", model_name);
}

int get_system_info(char *buffer, size_t size)
{
    struct utsname operating_system;
    struct sysinfo memory;
    char cpu_name[256];
    long processor_count;
    unsigned long long ram_mb;
    int written;

    if (buffer == NULL || size == 0 || uname(&operating_system) != 0 ||
        sysinfo(&memory) != 0) {
        return 0;
    }

    read_cpu_name(cpu_name, sizeof(cpu_name));
    processor_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (processor_count < 1) {
        processor_count = 1;
    }
    ram_mb = ((unsigned long long)memory.totalram * memory.mem_unit) /
             (1024ULL * 1024ULL);

    written = snprintf(
        buffer, size,
        "OS: %s %s (%s)\n"
        "CPU: %s, %ld mantiksal cekirdek\n"
        "RAM: %llu MB",
        operating_system.sysname,
        operating_system.release,
        operating_system.machine,
        cpu_name,
        processor_count,
        ram_mb);

    return written >= 0 && (size_t)written < size;
}
#endif
