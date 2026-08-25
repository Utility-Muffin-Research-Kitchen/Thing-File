#include "i18n.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdlib.h>

int main()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("thing-file-i18n-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directories(root)) return 1;

    const std::filesystem::path table_dir =
        root / "Thing-File/i18n";
    std::filesystem::create_directories(table_dir);
    std::ofstream(table_dir / "zh_CN.tsv")
        << "Copy\t复制\n"
        << "Copy\t拷贝\n"
        << "Move\t移动\n";

    setenv("UMRK_LANGUAGE", "zh_CN", 1);
    setenv("USERDATA_PATH", root.c_str(), 1);
    unsetenv("THING_FILE_I18N_DIR");
    i18n::init("");

    const bool ok = std::strcmp(i18n::t("Copy"), "Copy") == 0 &&
                    std::strcmp(i18n::t("Move"), "移动") == 0;
    std::filesystem::remove_all(root);
    if (!ok) {
        std::cerr << "duplicate key did not fall back to English\n";
        return 1;
    }
    std::cout << "PASS i18n-runtime-test\n";
    return 0;
}
